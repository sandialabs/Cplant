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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_NODES 16
char my_node[ 8 ];
char max_nodes[ 8 ];
char to_nodes[ MAX_NODES ];
char from_nodes[ MAX_NODES ];

int fds[MAX_NODES][MAX_NODES][2];
static int debug = 0;

int main( int argc, char *argv[] )
{
	int num_nodes, x, y;

	if( argc < 3 ) {
		fprintf(stderr, "Usage: %s num_nodes command...\n", argv[0] );
		exit( EXIT_FAILURE );
	}

	sprintf( max_nodes, "%d", MAX_NODES );
	num_nodes = atoi( argv[1] );
	setenv( "PTL_MY_NODE", "0", 1 );
	setenv( "PTL_MY_GID", "113", 1 );
	setenv( "PTL_NUM_NODES", argv[1], 1 );
	setenv( "PTL_MAX_NODES", max_nodes, 1 );

	if( num_nodes < 1 || num_nodes > MAX_NODES ) {
		fprintf( stderr, "%s: Max nodes is %d right now\n",
			argv[0], MAX_NODES );
		exit( EXIT_FAILURE );
	}

	for( x=0 ; x < num_nodes ; x++ ) {
		for( y=0 ; y<num_nodes ; y++ ) {

			pipe( fds[x][y] );
			if( debug )
				printf( "%d -> %d: write %d / read %d\n",
					x, y, 
					fds[x][y][1], fds[x][y][0]
				);
		}
	}

	for( x=0 ; x < num_nodes ; x++ ) {
		if( fork() ) continue;
		
		sprintf( my_node, "%d", x );
		setenv( "PTL_MY_RANK", my_node, 1 );

		for( y=0 ; y < num_nodes ; y++ ) {
			to_nodes[y]	= fds[x][y][1];
			from_nodes[y]	= fds[y][x][0];
		}

		to_nodes[num_nodes] = from_nodes[num_nodes] = 0;

		setenv( "PTL_TO_NODES", to_nodes, 1 );
		setenv( "PTL_FROM_NODES", from_nodes, 1 );

		if( debug )
			printf( "%d: execing %s\n", x, argv[2] );

		execv( argv[2], &argv[2] );
		fprintf( stderr, "%d: Didn't exec()?\n", x );

		exit( EXIT_FAILURE );
	}

	return 0;
}
