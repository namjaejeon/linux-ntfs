# SPDX-License-Identifier: GPL-2.0

ifneq ($(KERNELRELEASE),)
obj-$(CONFIG_NTFS_FS) += ntfs.o

ntfs-y := aops.o attrib.o collate.o dir.o file.o index.o inode.o \
	  mft.o mst.o namei.o runlist.o super.o unistr.o attrlist.o ea.o \
	  upcase.o bitmap.o lcnalloc.o logfile.o reparse.o compress.o \
	  iomap.o debug.o sysctl.o quota.o object_id.o bdev-io.o

ccflags-$(CONFIG_NTFS_DEBUG) += -DDEBUG
#ccflags-y += -DCONFIG_NTFS_FS_POSIX_ACL
else
# Called from external kernel module build

KERNELRELEASE	?= $(shell uname -r)
KDIR	?= /lib/modules/${KERNELRELEASE}/build
MDIR	?= /lib/modules/${KERNELRELEASE}
PWD	:= $(shell pwd)

export CONFIG_NTFS_FS := m

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) -C $(KDIR) M=$(PWD) help

install: ntfs.ko
	rm -f ${MDIR}/kernel/fs/ntfs/ntfs.ko
	install -m644 -b -D ntfs.ko ${MDIR}/kernel/fs/ntfs/ntfs.ko
	depmod -aq

uninstall:
	rm -rf ${MDIR}/kernel/fs/ntfs
	depmod -aq

endif

.PHONY : all clean install uninstall
