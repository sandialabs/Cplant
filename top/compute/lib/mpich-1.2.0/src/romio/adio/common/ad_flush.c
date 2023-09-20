/* 
 *   $Id: ad_flush.c,v 1.1 2000/05/10 21:42:46 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"

void ADIOI_GEN_Flush(ADIO_File fd, int *error_code)
{
    int err;

    err = fsync(fd->fd_sys);

    *error_code = (err == 0) ? MPI_SUCCESS : MPI_ERR_UNKNOWN;
}
