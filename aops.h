/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Defines for NTFS kernel address space operations and page cache
 * handling.
 *
 * Copyright (c) 2001-2004 Anton Altaparmakov
 * Copyright (c) 2002 Richard Russon
 * Copyright (c) 2025 LG Electronics Co., Ltd.
 */

#ifndef _LINUX_NTFS_AOPS_H
#define _LINUX_NTFS_AOPS_H

#include <linux/pagemap.h>
#include <linux/iomap.h>
#include <linux/version.h>

#include "volume.h"
#include "inode.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
void mark_ntfs_record_dirty(struct folio *folio);
#else
void mark_ntfs_record_dirty(struct page *page);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 16, 0)
int ntfs_rw_bdev(struct block_device *bdev, sector_t sector, unsigned int count,
		 char *data, enum req_op op);
#else
int ntfs_dev_read(struct super_block *sb, void *buf, loff_t start, loff_t end);
#endif
int ntfs_dev_write(struct super_block *sb, void *buf, loff_t start,
		   loff_t size);
#endif /* _LINUX_NTFS_AOPS_H */
