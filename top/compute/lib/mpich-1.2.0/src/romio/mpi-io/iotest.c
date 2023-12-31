/* 
 *   $Id: iotest.c,v 1.1 2000/05/10 21:44:00 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPIO_Test = PMPIO_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPIO_Test MPIO_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPIO_Test as PMPIO_Test
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPIO_Test - Test the completion of a nonblocking read or write
                
Input Parameters:
. request - request object (handle)

Output Parameters:
. flag - true if operation completed (logical)
. status - status object (Status)

.N fortran
@*/
int MPIO_Test(MPIO_Request *request, int *flag, MPI_Status *status)
{
    int error_code;
#ifdef MPI_hpux
    int fl_xmpi;

    if (*request != MPIO_REQUEST_NULL) {
	HPMP_IO_WSTART(fl_xmpi, BLKMPIOTEST, TRDTSYSTEM, (*request)->fd);
    }
#endif /* MPI_hpux */

    if (*request == MPIO_REQUEST_NULL) return MPI_SUCCESS;

    if ((*request < (MPIO_Request) 0) || 
	     ((*request)->cookie != ADIOI_REQ_COOKIE)) {
	printf("MPIO_Test: Invalid request object\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    switch ((*request)->optype) {
    case ADIOI_READ:
        *flag = ADIO_ReadDone(request, status, &error_code);
        break;
    case ADIOI_WRITE:
        *flag = ADIO_WriteDone(request, status, &error_code);
        break;
    }

#ifdef MPI_hpux
    HPMP_IO_WEND(fl_xmpi);
#endif /* MPI_hpux */
    return error_code;
}
