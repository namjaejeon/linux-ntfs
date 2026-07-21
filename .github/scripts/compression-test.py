#!/usr/bin/env python3
"""Exercise NTFS compressed I/O through the system.ntfs_attrib xattr."""

import argparse
import errno
import hashlib
import json
import os
import random
import struct
from pathlib import Path

COMPRESSED = 0x00000800
SPARSE = 0x00000200
XATTR = "system.ntfs_attrib"
XATTR_BE = "system.ntfs_attrib_be"
SUB_BLOCK = 4096
COMPRESSION_UNIT = 16 * SUB_BLOCK


def attrs(path: Path, name: str = XATTR) -> int:
    value = os.getxattr(path, name)
    byte_order = ">I" if name == XATTR_BE else "=I"
    return struct.unpack(byte_order, value)[0]


def set_attrs(path: Path, value: int, name: str = XATTR) -> None:
    byte_order = ">I" if name == XATTR_BE else "=I"
    os.setxattr(path, name, struct.pack(byte_order, value))


def enable_compression(path: Path, name: str = XATTR) -> None:
    path.touch()
    set_attrs(path, attrs(path, name) | COMPRESSED, name)
    actual = attrs(path, name)
    if not actual & COMPRESSED or actual & SPARSE:
        raise AssertionError(f"failed to enable compression on {path}: {actual:#x}")


def expect_setxattr_error(path: Path, value: bytes, expected_errno: int) -> None:
    try:
        os.setxattr(path, XATTR, value)
    except OSError as error:
        if error.errno != expected_errno:
            raise AssertionError(
                f"{path}: expected errno {expected_errno}, got {error.errno}"
            ) from error
    else:
        raise AssertionError(f"{path}: setxattr unexpectedly succeeded")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def deterministic_random(size: int, seed: int) -> bytes:
    return random.Random(seed).randbytes(size)


def write_compressed(path: Path, data: bytes) -> None:
    """Write without O_TRUNC, which compressed files do not support."""
    with path.open("r+b", buffering=0) as output:
        output.write(data)
        os.fsync(output.fileno())


def record_file(manifest: dict, root: Path, name: str, data: bytes) -> None:
    path = root / name
    enable_compression(path)
    write_compressed(path, data)
    actual = path.read_bytes()
    if actual != data:
        raise AssertionError(f"immediate readback mismatch for {name}")
    manifest["files"][name] = {"size": len(data), "sha256": digest(data)}


def prepare(root: Path, manifest_path: Path) -> None:
    manifest = {"files": {}, "must_uncompressed": []}

    # Both compressible and incompressible streams exercise LZNT1 and the
    # uncompressed fallback around 4 KiB sub-block and 64 KiB unit boundaries.
    boundary_sizes = (
        0,
        1,
        SUB_BLOCK - 1,
        SUB_BLOCK,
        SUB_BLOCK + 1,
        COMPRESSION_UNIT - 1,
        COMPRESSION_UNIT,
        COMPRESSION_UNIT + 1,
        2 * COMPRESSION_UNIT + 257,
    )
    for size in boundary_sizes:
        record_file(manifest, root, f"repeated-{size}", b"NTFS" * (size // 4) + b"NTFS"[: size % 4])
        if size:
            record_file(manifest, root, f"random-{size}", deterministic_random(size, size))

    # A large zero extent should take the all-hole compressed-unit path.
    record_file(manifest, root, "zeros-262144", bytes(4 * COMPRESSION_UNIT))

    # Unaligned writes cross both sub-block and compression-unit boundaries.
    name = "unaligned-pwrite"
    path = root / name
    enable_compression(path)
    expected = bytearray(b"a" * (2 * COMPRESSION_UNIT + 257))
    with path.open("r+b", buffering=0) as output:
        output.write(expected)
        patches = (
            (1, b"begin"),
            (SUB_BLOCK - 2, b"sub-block-crossing"),
            (COMPRESSION_UNIT - 3, b"unit-crossing"),
            (COMPRESSION_UNIT + 17, deterministic_random(8193, 7)),
        )
        for offset, value in patches:
            os.pwrite(output.fileno(), value, offset)
            expected[offset : offset + len(value)] = value
        os.fsync(output.fileno())
    if path.read_bytes() != expected:
        raise AssertionError("unaligned pwrite readback mismatch")
    manifest["files"][name] = {"size": len(expected), "sha256": digest(expected)}

    # Explicitly create holes before, between, and after initialized data.
    name = "holes"
    path = root / name
    enable_compression(path)
    expected = bytearray(4 * COMPRESSION_UNIT + 123)
    with path.open("r+b", buffering=0) as output:
        for offset, value in (
            (0, b"head"),
            (COMPRESSION_UNIT + 31, b"middle"),
            (4 * COMPRESSION_UNIT + 119, b"tail"),
        ):
            os.pwrite(output.fileno(), value, offset)
            expected[offset : offset + len(value)] = value
        os.fsync(output.fileno())
    if path.read_bytes() != expected:
        raise AssertionError("sparse compressed file readback mismatch")
    manifest["files"][name] = {"size": len(expected), "sha256": digest(expected)}

    # ATTR_SIZE changes are currently unsupported for compressed files.  In
    # particular, opening one with O_TRUNC must fail without losing its data.
    name = "reject-truncate"
    path = root / name
    enable_compression(path)
    expected = deterministic_random(3 * COMPRESSION_UNIT, 11)
    write_compressed(path, expected)
    for new_size in (COMPRESSION_UNIT + 17, 4 * COMPRESSION_UNIT):
        try:
            os.truncate(path, new_size)
        except OSError as error:
            if error.errno != errno.EOPNOTSUPP:
                raise AssertionError(
                    f"truncate to {new_size}: expected EOPNOTSUPP, "
                    f"got errno {error.errno}"
                ) from error
        else:
            raise AssertionError(f"truncate to {new_size} unexpectedly succeeded")
    try:
        path.open("wb").close()
    except OSError as error:
        if error.errno != errno.EOPNOTSUPP:
            raise AssertionError(
                f"O_TRUNC: expected EOPNOTSUPP, got errno {error.errno}"
            ) from error
    else:
        raise AssertionError("O_TRUNC unexpectedly succeeded")
    if path.read_bytes() != expected:
        raise AssertionError("rejected truncate changed compressed data")
    manifest["files"][name] = {"size": len(expected), "sha256": digest(expected)}

    # Replacing an allocated, incompressible unit with zeroes exercises the
    # compressed-unit hole-punch/update path.
    name = "zero-overwrite"
    path = root / name
    enable_compression(path)
    expected = bytearray(deterministic_random(3 * COMPRESSION_UNIT, 13))
    with path.open("r+b", buffering=0) as output:
        output.write(expected)
        os.pwrite(output.fileno(), bytes(COMPRESSION_UNIT), COMPRESSION_UNIT)
        expected[COMPRESSION_UNIT : 2 * COMPRESSION_UNIT] = bytes(COMPRESSION_UNIT)
        os.fsync(output.fileno())
    if path.read_bytes() != expected:
        raise AssertionError("zero overwrite readback mismatch")
    manifest["files"][name] = {"size": len(expected), "sha256": digest(expected)}

    # The big-endian compatibility xattr reaches the same ntfs_setxattr path.
    # Keep this an attribute-only case; compressed I/O patterns above all use
    # the canonical little-endian interface under test.
    name = "big-endian-xattr"
    path = root / name
    enable_compression(path, XATTR_BE)
    manifest["files"][name] = {"size": 0, "sha256": digest(b"")}

    # Invalid transitions must fail without changing file state.
    path = root / "reject-nonempty"
    path.write_bytes(b"not empty")
    expect_setxattr_error(path, struct.pack("=I", COMPRESSED), errno.EOPNOTSUPP)
    manifest["must_uncompressed"].append(path.name)

    path = root / "reject-compressed-sparse"
    path.touch()
    expect_setxattr_error(
        path, struct.pack("=I", COMPRESSED | SPARSE), errno.EOPNOTSUPP
    )
    manifest["must_uncompressed"].append(path.name)

    path = root / "reject-invalid-xattr-size"
    path.touch()
    expect_setxattr_error(path, b"\x00\x08", errno.EINVAL)
    manifest["must_uncompressed"].append(path.name)

    path = root / "reject-clear-nonempty"
    enable_compression(path)
    write_compressed(path, b"compressed data")
    expect_setxattr_error(path, struct.pack("=I", 0), errno.EOPNOTSUPP)
    manifest["files"][path.name] = {
        "size": path.stat().st_size,
        "sha256": digest(path.read_bytes()),
    }

    # Empty files may toggle compression in either direction.
    path = root / "toggle-empty"
    enable_compression(path)
    set_attrs(path, attrs(path) & ~COMPRESSED)
    if attrs(path) & COMPRESSED:
        raise AssertionError("compression remained set after clearing an empty file")
    set_attrs(path, attrs(path) | COMPRESSED)
    manifest["files"][path.name] = {"size": 0, "sha256": digest(b"")}

    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True))


def verify(root: Path, manifest_path: Path) -> dict:
    manifest = json.loads(manifest_path.read_text())
    for name, expected in manifest["files"].items():
        path = root / name
        data = path.read_bytes()
        actual_digest = digest(data)
        if len(data) != expected["size"] or actual_digest != expected["sha256"]:
            raise AssertionError(
                f"persistent readback mismatch for {name}: "
                f"size {len(data)} (expected {expected['size']}), "
                f"sha256 {actual_digest} (expected {expected['sha256']})"
            )
        actual_attrs = attrs(path)
        if not actual_attrs & COMPRESSED or actual_attrs & SPARSE:
            raise AssertionError(f"bad persistent attributes for {name}: {actual_attrs:#x}")

    for name in manifest["must_uncompressed"]:
        actual_attrs = attrs(root / name)
        if actual_attrs & (COMPRESSED | SPARSE):
            raise AssertionError(f"failed xattr operation changed {name}: {actual_attrs:#x}")

    # Confirm that the highly compressible streams are not merely flagged but
    # actually consume less disk space than their logical lengths.
    for name in ("repeated-131329", "zeros-262144"):
        stat = (root / name).stat()
        if stat.st_blocks * 512 >= stat.st_size:
            raise AssertionError(f"{name} was not stored compressed/sparse")
    return manifest


def mutate(root: Path, manifest_path: Path) -> None:
    manifest = verify(root, manifest_path)

    # Modify data loaded from disk after a remount, then verify it after another
    # remount. This covers read-modify-write of existing compression units.
    for name, patches, suffix in (
        (
            "random-65537",
            ((SUB_BLOCK - 1, b"S" * 33), (COMPRESSION_UNIT - 8, b"boundary" * 5)),
            b"appended-after-remount",
        ),
        (
            "repeated-131329",
            ((COMPRESSION_UNIT + 3, deterministic_random(5000, 29)),),
            b"tail",
        ),
    ):
        path = root / name
        expected = bytearray(path.read_bytes())
        with path.open("r+b", buffering=0) as output:
            for offset, value in patches:
                os.pwrite(output.fileno(), value, offset)
                expected[offset : offset + len(value)] = value
            output.seek(0, os.SEEK_END)
            output.write(suffix)
            expected.extend(suffix)
            os.fsync(output.fileno())
        if path.read_bytes() != expected:
            raise AssertionError(f"post-remount mutation mismatch for {name}")
        manifest["files"][name] = {
            "size": len(expected),
            "sha256": digest(expected),
        }

    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("prepare", "mutate", "verify"))
    parser.add_argument("mountpoint", type=Path)
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()

    if args.mode == "prepare":
        prepare(args.mountpoint, args.manifest)
    elif args.mode == "mutate":
        mutate(args.mountpoint, args.manifest)
    else:
        verify(args.mountpoint, args.manifest)

    print(f"compression {args.mode} phase passed")


if __name__ == "__main__":
    main()
