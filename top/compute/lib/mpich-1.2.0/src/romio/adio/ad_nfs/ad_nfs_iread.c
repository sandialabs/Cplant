/* 
 *   $Id: ad_nfs_iread.c,v 1.1 2000/05/10 21:42:37 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

void ADIOI_NFS_IreadContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
#ifdef __NO_AIO
    ADIO_Status status;
#else
    int err=-1;
#endif

    (*request) = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;

#ifdef __NO_AIO
    /* HP, FreeBSD, Linux */
    /* no support for nonblocking I/O. Use blocking I/O. */

    ADIOI_NFS_ReadContig(fd, buf, len, file_ptr_type, offset, &status,
                    error_code);  
    (*request)->queued = 0;

#else
    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
        if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;

        err = ADIOI_NFS_aio(fd, buf, len, offset, 0, 
                           &((*request)->handle));

        if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += len;
    }

    (*request)->queued = 1;
    ADIOI_Add_req_to_list(request);

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
#endif

    fd->fp_sys_posn = -1;   /* set it to null. */

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */
}



void ADIOI_NFS_IreadStrided(ADIO_File fd, void *buf, int count, 
		       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Request *request, int
                       *error_code)
{
    ADIO_Status status;

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;
    (*request)->queued = 0;
    (*request)->handle = 0;

/* call the blocking version. It is faster because it does data sieving. */
    ADIOI_NFS_ReadStrided(fd, buf, count, datatype, file_ptr_type, 
                            offset, &status, error_code);  

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */

}
