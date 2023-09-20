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
/* 	$Id: grouptest.c,v 1.1 1997/10/29 22:51:26 bright Exp $	 */

#ifndef lint
static char vcid[] = "$Id: grouptest.c,v 1.1 1997/10/29 22:51:26 bright Exp $";
#endif /* lint */
#include "mpi.h"

/*
    grouptest -
*/
int main( argc, argv )
int argc;
char **argv;
{
    int rank, size, i;
    MPI_Group group1, group2, group3, groupall, groupunion, groupintersection,
              newgroup;
    MPI_Comm newcomm;
    int ranks1[100], ranks2[100], ranks3[100];
    int nranks1=0, nranks2=0, nranks3=0;
    int grouprank;

    MPI_Init( &argc, &argv );
    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_group( MPI_COMM_WORLD, &groupall );

    /* Divide groups */
    for (i=0; i<size; i++) 
      if ( (i%3)==0 )
	ranks1[nranks1++] = i;
      else if ( (i%3)==1 )
	ranks2[nranks2++] = i;
      else
	ranks3[nranks3++] = i;

    MPI_Group_incl ( groupall, nranks1, ranks1, &group1 );
    MPI_Group_incl ( groupall, nranks2, ranks2, &group2 );
    MPI_Group_incl ( groupall, nranks3, ranks3, &group3 );

    MPI_Group_difference ( groupall, group2, &groupunion );

    MPI_Comm_create ( MPI_COMM_WORLD, group3, &newcomm );
    newgroup = MPI_GROUP_NULL;
    if (newcomm != MPI_COMM_NULL)
    {
	/* If we don't belong to group3, this would fail */
	MPI_Comm_group ( newcomm, &newgroup );
    }

    /* Free the groups */
    MPI_Group_free( &groupall );
    MPI_Group_free( &group1 );
    MPI_Group_free( &group2 );
    MPI_Group_free( &group3 );
    MPI_Group_free( &groupunion );
    if (newgroup != MPI_GROUP_NULL)
    {
	MPI_Group_free( &newgroup );
    }

    /* Free the communicator */
    if (newcomm != MPI_COMM_NULL)
	MPI_Comm_free( &newcomm );
    Test_Waitforall( );
    MPI_Finalize();
    return 0;
}


