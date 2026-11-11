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

#define NTFS_IOC_MAGIC	0xEF

/*
 * Flags for ntfs_stream.flags and ntfs_list_streams.flags.
 */
#define NTFS_STREAM_FL_UTF16		0x1

struct ntfs_stream {
	__aligned_u64 stream_offset;
	__aligned_u64 io_len;
	__aligned_u64 bytes_returned;
	__u32 name_len;
	__u32 flags;
	__aligned_u64 reserved;
	__u8 buffer[];
};

struct ntfs_stream_entry {
	__aligned_u64 next_entry_off;
	__aligned_u64 size;
	__aligned_u64 alloc_size;
	__u32 name_len;
	__u32 name_offset;
	__u32 reserved;
	__u8 name[];
};

struct ntfs_list_streams {
	__aligned_u64 buffer_size;
	__aligned_u64 bytes_returned;
	__aligned_u64 stream_count;
	__u32 flags;
	__u32 reserved;
	__u8 buffer[];
};

#define NTFS_IOC_STREAM_READ \
	_IOWR(NTFS_IOC_MAGIC, 1, struct ntfs_stream)
#define NTFS_IOC_STREAM_WRITE \
	_IOWR(NTFS_IOC_MAGIC, 2, struct ntfs_stream)
#define NTFS_IOC_STREAM_REMOVE \
	_IOWR(NTFS_IOC_MAGIC, 3, struct ntfs_stream)
#define NTFS_IOC_LIST_STREAMS \
	_IOWR(NTFS_IOC_MAGIC, 4, struct ntfs_list_streams)

#endif /* _UAPI_LINUX_NTFS_H */
