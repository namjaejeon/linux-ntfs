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
 * ntfs named stream ioctl structure.
 *
 * @stream_offset:	Offset within the named stream.
 * @io_len:		Number of bytes to read/write.
 * @result_size:	Actual bytes transferred.
 * @flags:		Must be zero.
 * @reserved:		Must be zero.
 * @name_len:		Stream name length.
 * @reserved2:		Must be zero.
 * @buffer:		Stream name followed by stream data.
 *
 * The stream name is encoded with the mounted filesystem NLS.  For read and
 * write, @io_len bytes of data follow the name.  For remove, only the name
 * is required and @io_len must be zero.
 */
struct ntfs_stream {
	__u64 stream_offset;
	__u64 io_len;
	__u64 result_size;
	__u32 flags;
	__u32 reserved;
	__u32 name_len;
	__u32 reserved2;
	__u8 buffer[]; /* name + data */
};

struct ntfs_stream_entry {
	__u64 size;
	__u64 alloc_size;
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
 * @flags:		Must be zero.
 * @reserved:		Must be zero.
 * @buffer:		ntfs_stream_entry array.
 */
struct ntfs_list_streams {
	__u64 buffer_size;
	__u64 bytes_returned;
	__u64 stream_count;
	__u32 flags;
	__u32 reserved;
	__u8 buffer[];
};

#define NTFS_IOC_STREAM_READ		_IOWR(NTFS_IOC_MAGIC, 1, struct ntfs_stream)
#define NTFS_IOC_STREAM_WRITE		_IOWR(NTFS_IOC_MAGIC, 2, struct ntfs_stream)
#define NTFS_IOC_STREAM_REMOVE		_IOWR(NTFS_IOC_MAGIC, 3, struct ntfs_stream)
#define NTFS_IOC_LIST_STREAMS		_IOWR(NTFS_IOC_MAGIC, 4, struct ntfs_list_streams)

#endif /* _UAPI_LINUX_NTFS_H */
