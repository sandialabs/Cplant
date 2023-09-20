/* 
 *   $Id: read_at.c,v 1.1 2000/05/10 21:44:01 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_read_at = PMPI_File_read_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_read_at MPI_File_read_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_read_at as PMPI_File_read_at
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */


/*@
    MPI_File_read_at - Read using explict offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf,
		    int count, MPI_Datatype datatype, MPI_Status *status)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size;
    ADIO_Offset off;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEREADAT, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_read_at: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (offset < 0) {
	printf("MPI_File_read_at: Invalid offset argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_read_at: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_read_at: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);
    if (count*datatype_size == 0) {
#ifdef MPI_hpux
	HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
	return MPI_SUCCESS;
    }

    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_read_at: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_WRONLY) {
	printf("MPI_File_read_at: Can't read from a file opened with MPI_MODE_WRONLY\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
	printf("MPI_File_read_at: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    /* contiguous or strided? */

    if (buftype_is_contig && filetype_is_contig) {
    /* convert count and offset to bytes */
	bufsize = datatype_size * count;
	off = fh->disp + fh->etype_size * offset;

        /* if atomic mode requested, lock (exclusive) the region, because there
           could be a concurrent noncontiguous request. Locking doesn't 
           work on PIOFS and PVFS, and on NFS it is done in the ADIO_ReadContig.*/

        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
            (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS))
            ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);

	ADIO_ReadContig(fh, buf, bufsize, ADIO_EXPLICIT_OFFSET,
			off, status, &error_code); 

        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
            (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS))
            ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);
    }
    else ADIO_ReadStrided(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			  offset, status, &error_code); 
    /* For strided and atomic mode, locking is done in ADIO_ReadStrided */


#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}

