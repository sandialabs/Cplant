/* 
 *   $Id: ioreq_c2f.c,v 1.1 2000/05/10 21:44:00 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPIO_Request_c2f = PMPIO_Request_c2f
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPIO_Request_c2f MPIO_Request_c2f
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPIO_Request_c2f as PMPIO_Request_c2f
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif
#include "adio_extern.h"

/*@
    MPIO_Request_c2f - Translates a C I/O-request handle to a 
                       Fortran I/O-request handle

Input Parameters:
. request - C I/O-request handle (handle)

Return Value:
  Fortran I/O-request handle (integer)
@*/
MPI_Fint MPIO_Request_c2f(MPIO_Request request)
{
#ifndef __INT_LT_POINTER
    return (MPI_Fint) request;
#else
    int i;

    if ((request <= (MPIO_Request) 0) || (request->cookie != ADIOI_REQ_COOKIE))
	return (MPI_Fint) 0;
    if (!ADIOI_Reqtable) {
	ADIOI_Reqtable_max = 1024;
	ADIOI_Reqtable = (MPIO_Request *)
	    ADIOI_Malloc(ADIOI_Reqtable_max*sizeof(MPIO_Request)); 
        ADIOI_Reqtable_ptr = 0;  /* 0 can't be used though, because 
                                  MPIO_REQUEST_NULL=0 */
	for (i=0; i<ADIOI_Reqtable_max; i++) ADIOI_Reqtable[i] = MPIO_REQUEST_NULL;
    }
    if (ADIOI_Reqtable_ptr == ADIOI_Reqtable_max-1) {
	ADIOI_Reqtable = (MPIO_Request *) ADIOI_Realloc(ADIOI_Reqtable, 
                           (ADIOI_Reqtable_max+1024)*sizeof(MPIO_Request));
	for (i=ADIOI_Reqtable_max; i<ADIOI_Reqtable_max+1024; i++) 
	    ADIOI_Reqtable[i] = MPIO_REQUEST_NULL;
	ADIOI_Reqtable_max += 1024;
    }
    ADIOI_Reqtable_ptr++;
    ADIOI_Reqtable[ADIOI_Reqtable_ptr] = request;
    return (MPI_Fint) ADIOI_Reqtable_ptr;
#endif
}
