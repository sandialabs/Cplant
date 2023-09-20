/* 
 *   $Id: ad_fstype.c,v 1.1 2000/05/10 21:42:46 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#if (defined(__HPUX) || defined(__SPPUX) || defined(__IRIX) || defined(__SOLARIS) || defined(__AIX) || defined(__DEC) || defined(__CRAY))
#include <sys/statvfs.h>
#endif
#ifdef __LINUX
#include <sys/vfs.h>
/* #include <linux/nfs_fs.h> this file is broken in newer versions of linux */
#define NFS_SUPER_MAGIC 0x6969
#endif
#ifdef __FREEBSD
#include <sys/param.h>
#include <sys/mount.h>
#endif
#ifdef __PARAGON
#include <nx.h>
#include <pfs/pfs.h>
#include <sys/mount.h>
#endif
#ifdef __SX4
#include <sys/stat.h>
#endif
#ifdef __PVFS
#include "pvfs_config.h"
#endif

void ADIO_FileSysType(char *filename, int *fstype, int *error_code)
{
    char *dir, *slash;
    int err;
#if (defined(__HPUX) || defined(__SPPUX) || defined(__IRIX) || defined(__SOLARIS) || defined(__AIX) || defined(__DEC) || defined(__CRAY))
    struct statvfs vfsbuf;
#endif
#if (defined(__LINUX) || defined(__FREEBSD))
    struct statfs fsbuf;
#endif
#ifdef __PARAGON
    struct estatfs ebuf;
#endif
#ifdef __SX4
    struct stat sbuf;
#endif

    dir = strdup(filename);
    slash = strrchr(dir, '/');
    if (!slash) strcpy(dir, ".");
    else {
	if (slash == dir) *(dir + 1) = 0;
	else *slash = '\0';
    }

#if (defined(__HPUX) || defined(__SPPUX) || defined(__IRIX) || defined(__SOLARIS) || defined(__AIX) || defined(__DEC) || defined(__CRAY))
    err = statvfs(filename, &vfsbuf);
    if (err && (errno == ENOENT)) err = statvfs(dir, &vfsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	/* printf("%s\n", vfsbuf.f_basetype); */
	if (!strncmp(vfsbuf.f_basetype, "nfs", 3)) *fstype = ADIO_NFS;
	else {
# if (defined(__HPUX) || defined(__SPPUX))
#    ifdef __HFS
	    *fstype = ADIO_HFS;
#    else
            *fstype = ADIO_UFS;
#    endif
# else
	    if (!strncmp(vfsbuf.f_basetype, "xfs", 3)) *fstype = ADIO_XFS;
	    else if (!strncmp(vfsbuf.f_basetype, "piofs", 4)) *fstype = ADIO_PIOFS;
	    else *fstype = ADIO_UFS;
# endif
	}
	*error_code = MPI_SUCCESS;
    }
#elif defined(__LINUX)
    err = statfs(filename, &fsbuf);
    if (err && (errno == ENOENT)) err = statfs(dir, &fsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	/* printf("%d\n", fsbuf.f_type);*/
	if (fsbuf.f_type == NFS_SUPER_MAGIC) *fstype = ADIO_NFS;
#ifdef __PVFS
	else if (fsbuf.f_type == PVFS_SUPER_MAGIC) *fstype = ADIO_PVFS;
#endif
	else *fstype = ADIO_UFS;
	*error_code = MPI_SUCCESS;
    }
#elif (defined(__FREEBSD) && defined(__HAVE_MOUNT_NFS))
    err = statfs(filename, &fsbuf);
    if (err && (errno == ENOENT)) err = statfs(dir, &fsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	if (fsbuf.f_type == MOUNT_NFS) *fstype = ADIO_NFS;
	else *fstype = ADIO_UFS;
	*error_code = MPI_SUCCESS;
    }
#elif defined(__PARAGON)
    err = statpfs(filename, &ebuf, 0, 0);
    if (err && (errno == ENOENT)) err = statpfs(dir, &ebuf, 0, 0);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	if (ebuf.f_type == MOUNT_NFS) *fstype = ADIO_NFS;
	else if (ebuf.f_type == MOUNT_PFS) *fstype = ADIO_PFS;
	else *fstype = ADIO_UFS;
	*error_code = MPI_SUCCESS;
    }
#elif defined(__SX4)
     err = stat (filename, &sbuf);
     if (err && (errno == ENOENT)) err = stat (dir, &sbuf);
     free(dir);
 
     if (err) *error_code = MPI_ERR_UNKNOWN;
     else {
         if (!strcmp(sbuf.st_fstype, "nfs")) *fstype = ADIO_NFS;
         else *fstype = ADIO_SFS;
        *error_code = MPI_SUCCESS;
     }
#else
    /* on other systems, make NFS the default */
    free(dir);
    *fstype = ADIO_NFS;   
    *error_code = MPI_SUCCESS;
#endif
}
