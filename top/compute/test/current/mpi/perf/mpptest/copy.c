/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
/*
 *  $Id: copy.c,v 1.1 1997/10/29 20:31:39 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mpi.h"
extern int __NUMNODES, __MYPROCID;static MPI_Status _mpi_status;static int _n, _MPILEN;

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* 
   Test of single process memcpy.
   ctx is ignored for this test.
*/
double memcpy_rate(reps,len,ctx)
int      reps,len;
void     *ctx;
{
  double elapsed_time;
  int  i,msg_id,myproc;
  char *sbuffer,*rbuffer;
  double t0, t1;

  sbuffer = (char *)malloc(len);
  rbuffer = (char *)malloc(len);

  myproc       = __MYPROCID;
  elapsed_time = 0;
  *(&t0)=MPI_Wtime();
  for(i=0;i<reps;i++){
      memcpy( rbuffer, sbuffer, len );
  }
  *(&t1)=MPI_Wtime();
  elapsed_time = *(&t1 )-*(&t0);

  free(sbuffer);
  free(rbuffer);
  return(elapsed_time);
}
