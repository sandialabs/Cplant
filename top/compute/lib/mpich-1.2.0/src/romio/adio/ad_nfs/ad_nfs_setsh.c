/* 
 *   $Id: ad_nfs_setsh.c,v 1.1 2000/05/10 21:42:38 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

/* set the shared file pointer to "offset" etypes relative to the current 
   view */

void ADIOI_NFS_Set_shared_fp(ADIO_File fd, ADIO_Offset offset, int *error_code)
{
    int err;
    MPI_Comm dupcommself;

    if (fd->shared_fp_fd == ADIO_FILE_NULL) {
	MPI_Comm_dup(MPI_COMM_SELF, &dupcommself);
	fd->shared_fp_fd = ADIO_Open(dupcommself, fd->shared_fp_fname, 
             fd->file_system, ADIO_CREATE | ADIO_RDWR | ADIO_DELETE_ON_CLOSE, 
             0, MPI_BYTE, MPI_BYTE, M_ASYNC, MPI_INFO_NULL, 
             ADIO_PERM_NULL, error_code);
    }

    if (*error_code != MPI_SUCCESS) return;

    ADIOI_WRITE_LOCK(fd->shared_fp_fd, 0, SEEK_SET, sizeof(ADIO_Offset));
    lseek(fd->shared_fp_fd->fd_sys, 0, SEEK_SET);
    err = write(fd->shared_fp_fd->fd_sys, &offset, sizeof(ADIO_Offset));
    ADIOI_UNLOCK(fd->shared_fp_fd, 0, SEEK_SET, sizeof(ADIO_Offset));

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}

