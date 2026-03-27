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
 * @offset:		Start offset in stream.
 * @length:		Byte length to read/write.
 * @flags:		flags.
 * @reserved:		Reserved.
 * @name_len:		Stream name length.
 * @data_size:		Stream size.
 * @name:		Stream name.
 * @data:		named stream data.
 */
struct ntfs_stream_req {
	__u64 offset;
	__u64 length;
	__u32 flags;
	__u64 result_size;
	__u32 reserved;
	__u32 name_len;
	const char __user *name;
	void __user *data;
};

/*
 * Flags for NTFS_IOC_STREAM.
 */
enum {
	NTFS_STREAM_READ	= 0,
	NTFS_STREAM_WRITE,
	NTFS_STREAM_GET_SIZE,
	NTFS_STREAM_REMOVE
};

/*
 * ntfs list streams ioctl structure.
 *
 * @buffer_size:	user buffer size(in).
 * @bytes_returned:	acutal bytes written(out).
 * @stream_count:	number of streams(out).
 * @buffer:		ntfs_stream_entry array.
 */
struct ntfs_list_streams {
	u64 buffer_size;
	u64 bytes_returned;
	u64 stream_count;
	char buffer[];
};

#define NTFS_IOC_STREAM			_IOWR(NTFS_IOC_MAGIC, 1, struct ntfs_stream_req)
#define NTFS_IOC_LIST_STREAMS		_IOWR(NTFS_IOC_MAGIC, 2, struct ntfs_list_streams)

#endif /* _UAPI_LINUX_NTFS_H */
