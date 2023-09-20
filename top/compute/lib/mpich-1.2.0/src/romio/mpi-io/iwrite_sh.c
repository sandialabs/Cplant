/* 
 *   $Id: iwrite_sh.c,v 1.1 2000/05/10 21:44:01 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_iwrite_shared = PMPI_File_iwrite_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_iwrite_shared MPI_File_iwrite_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_iwrite_shared as PMPI_File_iwrite_shared
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_iwrite_shared - Nonblocking write using shared file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. request - request object (handle)

.N fortran
@*/
int MPI_File_iwrite_shared(MPI_File fh, void *buf, int count, 
                       MPI_Datatype datatype, MPIO_Request *request)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size, incr;
    ADIO_Status status;
    ADIO_Offset off, shared_fp;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_iwrite_shared: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_iwrite_shared: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_iwrite_shared: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);

    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_iwrite_shared: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((fh->file_system == ADIO_PIOFS) || (fh->file_system == ADIO_PVFS)) {
	printf("MPI_File_iwrite_shared: Shared file pointer not supported on PIOFS and PVFS\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    incr = (count*datatype_size)/fh->etype_size;
    ADIO_Get_shared_fp(fh, incr, &shared_fp, &error_code);
    if (error_code != MPI_SUCCESS) {
	printf("MPI_File_iwrite_shared: Could not access shared file pointer!\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* contiguous or strided? */
    if (buftype_is_contig && filetype_is_contig) {
    /* convert sizes to bytes */
	bufsize = datatype_size * count;
	off = fh->disp + fh->etype_size * shared_fp;
        if (!(fh->atomicity))
	    ADIO_IwriteContig(fh, buf, bufsize, ADIO_EXPLICIT_OFFSET,
		            off, request, &error_code); 
	else {
            /* to maintain strict atomicity semantics with other concurrent
              operations, lock (exclusive) and call blocking routine */

            *request = ADIOI_Malloc_request();
            (*request)->optype = ADIOI_WRITE;
            (*request)->fd = fh;
            (*request)->next = ADIO_REQUEST_NULL;
            (*request)->queued = 0;
	    (*request)->handle = 0;

            if (fh->file_system != ADIO_NFS)
                ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);

            ADIO_WriteContig(fh, buf, bufsize, ADIO_EXPLICIT_OFFSET, off, 
                    &status, &error_code);  

            if (fh->file_system != ADIO_NFS)
                ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);

            fh->async_count++;
            /* status info. must be linked to the request structure, so that it
               can be accessed later from a wait */
	}
    }
    else
	ADIO_IwriteStrided(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
		 	    shared_fp, request, &error_code); 

    return error_code;
}
