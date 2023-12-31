/* 
 *   $Id: iread_at.c,v 1.1 2000/05/10 21:44:00 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_iread_at = PMPI_File_iread_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_iread_at MPI_File_iread_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_iread_at as PMPI_File_iread_at
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_iread_at - Nonblocking read using explict offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPIO_Request *request)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size;
    ADIO_Status status;
    ADIO_Offset off;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEIREADAT, TRDTSYSTEM, fh, datatype, count);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_iread_at: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (offset < 0) {
	printf("MPI_File_iread_at: Invalid offset argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_iread_at: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_iread_at: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);

    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_iread_at: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_WRONLY) {
	printf("MPI_File_iread_at: Can't read from a file opened with MPI_MODE_WRONLY\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
	printf("MPI_File_iread_at: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    /* contiguous or strided? */

    if (buftype_is_contig && filetype_is_contig) {
    /* convert count and offset to bytes */
	bufsize = datatype_size * count;
	off = fh->disp + fh->etype_size * offset;
        if (!(fh->atomicity))
	    ADIO_IreadContig(fh, buf, bufsize, ADIO_EXPLICIT_OFFSET,
			off, request, &error_code); 
        else {
            /* to maintain strict atomicity semantics with other concurrent
              operations, lock (exclusive) and call blocking routine */

            *request = ADIOI_Malloc_request();
            (*request)->optype = ADIOI_READ;
            (*request)->fd = fh;
            (*request)->next = ADIO_REQUEST_NULL;
            (*request)->queued = 0;
	    (*request)->handle = 0;

            if ((fh->file_system != ADIO_PIOFS) && 
              (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS))
                ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);

            ADIO_ReadContig(fh, buf, bufsize, ADIO_EXPLICIT_OFFSET, off, 
                    &status, &error_code);  

            if ((fh->file_system != ADIO_PIOFS) && 
               (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS))
                ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);

            fh->async_count++;
            /* status info. must be linked to the request structure, so that it
               can be accessed later from a wait */
        }
    }
    else ADIO_IreadStrided(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			  offset, request, &error_code); 

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}
