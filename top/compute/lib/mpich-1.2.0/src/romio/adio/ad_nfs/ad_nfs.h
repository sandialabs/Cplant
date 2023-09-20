/* 
 *   $Id: ad_nfs.h,v 1.1 2000/05/10 21:42:37 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#ifndef __AD_NFS_INCLUDE
#define __AD_NFS_INCLUDE

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "adio.h"

#ifndef __NO_AIO
#ifdef __AIO_SUN
#include <sys/asynch.h>
#else
#include <aio.h>
#endif
#endif

int ADIOI_NFS_aio(ADIO_File fd, void *buf, int len, ADIO_Offset offset,
                  int wr, void *handle);

#ifdef __SX4
#define lseek llseek
#endif

#endif
