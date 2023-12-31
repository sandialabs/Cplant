/*
 *  $Id: dup_fn.c,v 1.1 2000/02/18 03:23:53 rbbrigh Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississipi State University.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*D
  
MPI_DUP_FN - A function to simple-mindedly copy attributes  

D*/
int MPIR_dup_fn ( 
	MPI_Comm comm, 
	int keyval, 
	void *extra_state, 
	void *attr_in, 
	void *attr_out, 
	int *flag )
{
  /* No error checking at present */

  /* Set attr_out, the flag and return success */
  (*(void **)attr_out) = attr_in;
  (*flag) = 1;
  return (MPI_SUCCESS);
}
