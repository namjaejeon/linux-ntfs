#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
IMAGE=$(mktemp --suffix=.ntfs-compression.img)
MOUNTPOINT=$(mktemp -d)
MANIFEST=$(mktemp)
LOOP_DEVICE=""

cleanup()
{
	set +e
	if mountpoint -q "$MOUNTPOINT"; then
		umount "$MOUNTPOINT"
	fi
	if [[ -n "$LOOP_DEVICE" ]]; then
		losetup -d "$LOOP_DEVICE"
	fi
	rm -f "$IMAGE" "$MANIFEST"
	rmdir "$MOUNTPOINT"
}
trap cleanup EXIT

truncate -s 1G "$IMAGE"
LOOP_DEVICE=$(losetup --find --show "$IMAGE")
mkfs.ntfs -F "$LOOP_DEVICE" >/dev/null
mount -t ntfs "$LOOP_DEVICE" "$MOUNTPOINT"

python3 "$SCRIPT_DIR/compression-test.py" prepare "$MOUNTPOINT" "$MANIFEST"
sync
umount "$MOUNTPOINT"
mount -t ntfs "$LOOP_DEVICE" "$MOUNTPOINT"

python3 "$SCRIPT_DIR/compression-test.py" mutate "$MOUNTPOINT" "$MANIFEST"
sync
umount "$MOUNTPOINT"
mount -t ntfs "$LOOP_DEVICE" "$MOUNTPOINT"

python3 "$SCRIPT_DIR/compression-test.py" verify "$MOUNTPOINT" "$MANIFEST"
sync
umount "$MOUNTPOINT"
ntfsck -n "$LOOP_DEVICE"
