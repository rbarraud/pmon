/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by Patrik Lindergren.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* $Id: diskfs.h,v 1.1.1.1 2006/06/29 06:43:25 cpu Exp $ */

#ifndef __DISKFS_H__
#define __DISKFS_H__

#include <sys/queue.h>

typedef struct DiskFileSystem {
	char *fsname;
	int (*open) __P((int, const char *, int, int));
	int (*read) __P((int , void *, size_t));
	int (*write) __P((int , const void *, size_t));
	off_t (*lseek) __P((int , off_t, int));
	int (*is_fstype) __P((int, off_t));
	int (*close) __P((int));
	int (*ioctl) __P((int , unsigned long , ...));
	int (*open_dir) __P((int, const char*));
	SLIST_ENTRY(DiskFileSystem)	i_next;
} DiskFileSystem;

typedef struct DiskPartitionTable {
	struct DiskPartitionTable* Next;
	struct DiskPartitionTable* logical;
	unsigned char bootflag;
	unsigned char tag;
	unsigned char id;
	unsigned int sec_begin;
	unsigned int size;
	unsigned int sec_end;
	DiskFileSystem* fs;
}DiskPartitionTable;

typedef struct DeviceDisk {
	struct DeviceDisk* Next;
	char device_name[20];
	DiskPartitionTable* part;
}DeviceDisk;
typedef struct DiskFile {
	char *devname;
	DiskFileSystem *dfs;
	DiskPartitionTable* part;
} DiskFile;


int diskfs_init(DiskFileSystem *fs);

#endif /* __DISKFS_H__ */
