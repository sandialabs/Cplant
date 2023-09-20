/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
/* $Id: protocol_switch.h,v 1.15 2002/02/12 18:51:13 pumatst Exp $ */

#ifndef PROTOCOL_SWITCH_H
#define PROTOCOL_SWITCH_H

#define APATH GL_fname
#define BPATH GL_fname2

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include "fdTable.h"

#if __linux__
#include <sys/vfs.h>
#endif

/*
** now defined in fdTable.h
**
**enum{ FYOD_IO_PROTO, YOD_IO_PROTO, ENFS_IO_PROTO, DUMMY_IO_PROTO, 
**    DUMMY1_IO_PROTO, DUMMY2_IO_PROTO, LAST_IO_PROTO };
*/

extern char newname[MAXPATHLEN];
extern char* GL_fname;
extern char* GL_fname2;

/* nonstandard io function */
int binval(int fd, void *p);

int path2io_proto(const char* path);
int path2io_proto2(const char* path1, const char* path2);
int fd2io_proto(int fd);
void io_proto_init(void);

typedef struct {
  int protocol;
} fdMap_t;

typedef struct {
  int (*open) (const char*, int, mode_t);
  int (*creat) (const char*, mode_t);
  int (*access) (const char*, int);
  int (*chdir) (const char*);
  int (*chmod) (const char*, mode_t);
  int (*chown) (const char*, uid_t, gid_t);
  int (*close) (int);
  int (*dup) (int);
  int (*dup2) (int, int);
  int (*fcntl) (int, int, int);
  int (*binval) (int, void*);
  int (*fstat) (int, struct stat*);
  int (*fstatfs) (int, struct statfs*);
  int (*fsync) (int);
  int (*ftruncate) (int, long);
  int (*getdirentries) (int, char*, int, long*);
  int (*ioctl) (int, unsigned long, char*);
  int (*link) (const char*, const char*);
  off_t (*lseek) (int, off_t, int);
  int (*lstat) (const char* , struct stat*);
  int (*mkdir) (const char*, mode_t mode);
#ifdef __osf__
  int (*read) (int, void*, size_t);
#else
  ssize_t (*read) (int, void*, size_t);
#endif
  void * (*mmap) (void *, size_t, int, int, int, off_t);
  int  (*munmap) (void *, size_t);
  int (*rename) (const char*, const char*);
  int (*rmdir) (const char*);
  int (*stat) (const char*, struct stat*);
  int (*statfs) (const char*, struct statfs*);
  int (*symlink) (const char*, const char*);
#ifdef __linux__
  int (*truncate) (const char *path, __off_t length);
#else
  int (*truncate) (const char *path, off_t length);
#endif /* __linux__ */
  char* (*ttyname) (int);
  int (*unlink) (const char*);
#ifdef __osf__
  int     (*write) (int fd, const void *buff, size_t nbytes);
#else
  ssize_t (*write) (int fd, const void *buff, size_t nbytes);
#endif
} io_ops_t;

extern io_ops_t io_ops_yod;
extern io_ops_t io_ops_dummy;
extern io_ops_t io_ops_enfs;

extern fdMap_t fdMap[];
extern io_ops_t *io_ops[]; 

int register_io_proto(int protocol, io_ops_t *ioops, const char* uri);
#endif
