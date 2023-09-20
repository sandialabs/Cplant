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
 *  $Id: typeub.c,v 1.1 1997/10/29 22:45:46 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include <math.h>
#include <stdio.h>
#include "mpi.h"

/*
 *	Trying to manipulate the extent of a datatype with succesive
 *	calls to MPI_Type_struct.  Tests that a MPI_UB buried within
 *      a structure is found.  From kalns@canidae.cps.msu.edu (modified to
 *      fit test structure).
 */
main( argc, argv )
int argc;
char **argv;
{
   MPI_Aint   	extent;
   int		blens[2];
   MPI_Aint	displ[2];
   MPI_Datatype types[2];
   MPI_Datatype type1,type2,type3;
   MPI_Aint     extent1, extent2, extent3;

   MPI_Init( &argc, &argv );

  /*	2 blocks of 1 int each, stride of 4 ; expect extent to be 20
   */
   MPI_Type_vector( 2, 1, 4, MPI_INT, &type1 );
   MPI_Type_commit( &type1 );
   MPI_Type_extent( type1, &extent );
   extent1 = 5 * sizeof(int);
   if (extent != extent1) {
       printf("extent(type1)=%d\n",extent);
       }

   blens[0]=1;
   blens[1]=1;
   displ[0]=0;
   displ[1]=sizeof(int)*4;
   types[0]=type1;
   types[1]=MPI_UB;
   extent2 = displ[1];

  /*	using MPI_UB and Type_struct, monkey with the extent, making it 16
   */
   MPI_Type_struct( 2, blens, displ, types, &type2 );
   MPI_Type_commit( &type2 );
   MPI_Type_extent( type2, &extent );
   if (extent != extent2) {
       printf("extent(type2)=%d\n",extent);
       }

  /*	monkey with the extent again, making it 4
   *  	===> MPICH gives 4	
   *  	===> MPIF gives 16, the old extent	
   */
   displ[1]=sizeof(int);
   types[0]=type2;
   types[1]=MPI_UB;
   extent3 = extent2;

   MPI_Type_struct( 2, blens, displ, types, &type3 );
   MPI_Type_commit( &type3 );

   MPI_Type_extent( type3, &extent );
   if (extent != extent3) {
       printf("extent(type3)=%d\n",extent);
       }

   MPI_Type_free( &type1 );
   MPI_Type_free( &type2 );
   MPI_Type_free( &type3 );
   MPI_Finalize();
   return 0;
}
