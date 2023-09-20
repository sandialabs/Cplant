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
 *  $Id: util.c,v 1.1 1997/10/29 20:32:00 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */

#include <stdio.h>

#include "mpi.h"
extern int __NUMNODES, __MYPROCID;static MPI_Status _mpi_status;static int _n, _MPILEN;

/*
   Utility programs for mpptest
 */

/* 
    T1 is time to send len1,
    T2 is time to send len2, 
    len is lenght we'd like the number of repititions for
    reps (input) is the default to use
 */
int GetRepititions( T1, T2, Len1, Len2, len, reps )
double T1, T2;
int    Len1, Len2, len, reps;
{
if (__MYPROCID == 0) {
    if (T1 > 0 && T2 > 0) 
	reps = ComputeGoodReps( T1, Len1, T2, Len2, len );
    }
    MPI_Bcast(&reps, sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD );
return reps;
}
