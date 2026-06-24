/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 2024 LG Electronics Co., Ltd.
 */

#ifndef _UAPI_LINUX_NTFS_H
#define _UAPI_LINUX_NTFS_H
#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * Shutdown the filesystem.
 */
#define FS_IOC_SHUTDOWN _IOR('X', 125, __u32)

/*
 * Flags for FS_IOC_SHUTDOWN
 */
#define FS_SHUTDOWN_FLAGS_DEFAULT	0x0
#define FS_SHUTDOWN_FLAGS_LOGFLUSH	0x1	/* flush log but not data*/
#define FS_SHUTDOWN_FLAGS_NOLOGFLUSH	0x2	/* don't flush log nor data */

#define NTFS_IOC_MAGIC	'N'

/*
 * Flags for ntfs_stream.flags and ntfs_list_streams.flags.
 *
 * NTFS_STREAM_FL_UTF16: the stream name is raw UTF-16LE, exactly as stored on
 * disk, instead of being encoded with the mounted filesystem NLS.  This allows
 * lossless round-tripping of stream names whose characters are not
 * representable in the mount NLS (e.g. for Wine, which works in UTF-16
 * natively).  When set, the name length fields count bytes of UTF-16LE and
 * must therefore be even.
 */
#define NTFS_STREAM_FL_UTF16	0x1

/*
 * ntfs named stream ioctl structure.
 *
 * @stream_offset:	Offset within the named stream.
 * @io_len:		Number of bytes to read/write.
 * @result_size:	Actual bytes transferred (out).
 * @flags:		Bit mask of NTFS_STREAM_FL_* flags; other bits must be
 *			zero.
 * @reserved:		Must be zero.
 * @name_len:		Stream name length in bytes, not including any
 *			terminating NUL (which must not be present).  When
 *			NTFS_STREAM_FL_UTF16 is set this is a UTF-16LE byte
 *			count and must be even.
 * @reserved2:		Must be zero.
 * @buffer:		Stream name followed by stream data.
 *
 * The stream name is the bare stream name, without the leading ':' or the
 * trailing ":$DATA" decoration used by Windows.  By default it is encoded with
 * the mounted filesystem NLS; if NTFS_STREAM_FL_UTF16 is set in @flags it is
 * raw UTF-16LE instead.  For read and write, @io_len bytes of data follow the
 * name.  For remove, only the name is required and @io_len must be zero.
 *
 * All 64-bit fields use __aligned_u64 so the layout is identical for 32-bit
 * and 64-bit userspace and no compat translation is required.
 */
struct ntfs_stream {
	__aligned_u64 stream_offset;
	__aligned_u64 io_len;
	__aligned_u64 result_size;
	__u32 flags;
	__u32 reserved;
	__u32 name_len;
	__u32 reserved2;
	__u8 buffer[]; /* name + data */
};

/*
 * Single stream entry returned by NTFS_IOC_LIST_STREAMS.
 *
 * @next_entry_off:	Byte offset from the start of this entry to the next
 *			entry, or zero for the last entry.  Always a multiple
 *			of 8.  Consumers must use this to advance instead of
 *			computing the stride themselves, so the entry can grow
 *			new fields without breaking the ABI.
 * @size:		Stream data size in bytes.
 * @alloc_size:		Bytes allocated for the stream (cluster aligned for
 *			non-resident streams).
 * @name_len:		Stream name length in bytes, not including a NUL
 *			terminator (none is stored).  Counts UTF-16LE bytes
 *			when NTFS_STREAM_FL_UTF16 was requested.
 * @reserved:		Must be zero.
 * @name:		Bare stream name; NLS encoded, or raw UTF-16LE when
 *			NTFS_STREAM_FL_UTF16 was requested (see struct
 *			ntfs_stream).
 */
struct ntfs_stream_entry {
	__aligned_u64 next_entry_off;
	__aligned_u64 size;
	__aligned_u64 alloc_size;
	__u32 name_len;
	__u32 reserved;
	__u8 name[];
};

/*
 * ntfs list streams ioctl structure.
 *
 * @buffer_size:	user buffer size(in).
 * @bytes_returned:	actual bytes written or required(out).
 * @stream_count:	number of streams(out).
 * @flags:		Bit mask of NTFS_STREAM_FL_* flags controlling the
 *			encoding of the returned names; other bits must be zero.
 * @reserved:		Must be zero.
 * @buffer:		ntfs_stream_entry array.
 *
 * The variable-length entries follow the header in @buffer, each aligned on
 * an 8-byte boundary and chained via ntfs_stream_entry.next_entry_off.  If
 * @buffer_size is too small, no data is copied, @stream_count and
 * @bytes_returned report the required values and the ioctl fails with
 * -ENOSPC.
 */
struct ntfs_list_streams {
	__aligned_u64 buffer_size;
	__aligned_u64 bytes_returned;
	__aligned_u64 stream_count;
	__u32 flags;
	__u32 reserved;
	__u8 buffer[];
};

#define NTFS_IOC_STREAM_READ		_IOWR(NTFS_IOC_MAGIC, 1, struct ntfs_stream)
#define NTFS_IOC_STREAM_WRITE		_IOWR(NTFS_IOC_MAGIC, 2, struct ntfs_stream)
#define NTFS_IOC_STREAM_REMOVE		_IOWR(NTFS_IOC_MAGIC, 3, struct ntfs_stream)
#define NTFS_IOC_LIST_STREAMS		_IOWR(NTFS_IOC_MAGIC, 4, struct ntfs_list_streams)

#endif /* _UAPI_LINUX_NTFS_H */
