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
 *  coll.c
 *
 * Test collective routines
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>

#include <p30.h>
#include <p30/coll.h>

#define xSTR( x ) #x
#define STR( x ) xSTR( x )

int my_nid = -1;

#define NODE_BUFFER	9
#define STATUS_REPORT	7
#define RESTART		5

static void test_fail( ptl_handle_ni_t ni, const char *str, int rc )
{
	fprintf( stderr, "FATAL: %d: %s: %s (errno=%d) (%ld)\n",
		my_nid, str, ptl_err_str[rc], rc, time( NULL ) );
	exit( EXIT_FAILURE );
}

int drain_events( const char *tag, ptl_handle_eq_t eq, int messages, int timeout );


int main( int argc, char *argv[] )
{
	int			rc;
	ptl_handle_ni_t		ni;
	ptl_id_t		nodes;
	ptl_process_id_t	my_id, id;

	int			count	= 16;
	int			passed	= 0;
	int			value;
	int			root;

	if( (rc = PtlInit()) )
		test_fail( ni, "PtlINit", rc );

	if( (rc = PtlNIInit( PTL_IFACE_DEFAULT, 16, 0, &ni ) ) )
		test_fail( ni, "PtlNIInit: " STR( PTL_IFACE_DEFAULT ), rc );

	if( (rc = PtlGetId( ni, &my_id, &nodes ) ) )
		test_fail( ni, "PtlGetID", rc );

	my_nid	= my_id.rid;
	id.addr_kind	= PTL_ADDR_NID;
	id.nid	= 37; /* Bogus! */
	id.pid	= 14;

	if( my_nid < 0 )
		printf( "ptl_coll_init is %p\n", ptl_coll_init );

	if( argc > 1 )
		count = atoi( argv[1] );

	printf( "%d: gid=%d out of %d nodes\n", my_nid, my_id.gid, nodes );

	sleep(2);

	while( count-- > 0 ) {
		value = 9;
		root = count % nodes;
		if( my_nid == root )
			printf( "%d: Root for this round\n", my_nid );

		PtlReduce_all( ni, root, &value );

		if( my_nid == root ) {
			printf( "%d: pass=%d.  Fan in done?\n",
				my_nid, count );
			value = 17 * count;
		} else
			value = 3; /* Bogus value */

		PtlBroadcast_all( ni, root, &value );
		if( value != 17 * count ) {
			printf( "%d: value %d != %d\n",
				my_nid, 17 * count, value );
			exit( EXIT_FAILURE );
		} else
			passed++;

		if( my_nid == root ) 
			printf( "%d: pass=%d.  Fan out done?\n",
				my_nid, count );
	}

	printf( "%d: Done %d passes\n", my_nid, passed );
	return 0;
}
