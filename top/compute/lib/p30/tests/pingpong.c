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
#include <signal.h>

#include <p30.h>
#include <p30/internal.h>


static char my_buf[1024];
int			my_nid = -1;
ptl_handle_ni_t		ni;

static void test_fail( const char *str, int rc )
{
	fprintf( stderr, "%d: FAILED: %s: error=%s (%d)\n",
		my_nid, str, ptl_err_str[rc], rc );
	exit(-1);
}

static void timeout( int sig )
{
	printf( "%d: Timedout waiting for events\n", my_nid );
	PtlNIFini( ni );
	sleep(10);
	exit(0);
}

int main( int argc, char *argv[] )
{
	int rc, i;
	ptl_handle_me_t		me;
	ptl_md_t		md_real;
	ptl_handle_md_t		md;
	ptl_handle_eq_t		eq;
	ptl_event_t		ev;
	ptl_id_t		nodes;
	ptl_process_id_t	my_id, id = {
		PTL_ADDR_NID,
		37, 14
	};

	if( (rc=PtlInit()) )
		test_fail( "PtlInit", rc );

	if( (rc=PtlNIInit( PTL_IFACE_DEFAULT, 32, 4, &ni )) )
		test_fail( "PtlNIInit", rc );

	if( (rc=PtlGetId( ni, &my_id, &nodes ) ) )
		test_fail( "PtlGetId", rc );

	my_nid = my_id.rid;
	printf( "%d: gid=%d out of %d nodes\n", my_nid, my_id.gid, nodes );
		
	printf( "%d: going to attach ni=%d rc=%d\n", my_nid, ni, rc );

	if( (rc= PtlMEAttach( ni, 4, id,
		0, 0,
		PTL_RETAIN, &me
	)) )
		test_fail( "PtlMEAttach 1", rc );
	

	printf( "%d: ME %d attached (Should have no next field)\n",
		my_nid, me );

	if( (rc = PtlEQAlloc( ni, 128, &eq )) )
		test_fail( "PtlEQAlloc 0", rc );

	printf( "%d: EQ %d allocated\n", my_nid, eq );

	/*
	 * Fill in the MD and attach it
	 */
	md_real.start		= my_buf;
	md_real.length		= 1024;
	md_real.threshold	= 2;
	md_real.max_offset      = md_real.length;
	md_real.options		= PTL_MD_OP_PUT
				| PTL_MD_OP_GET
				| PTL_MD_TRUNCATE
				| PTL_MD_MANAGE_REMOTE
				| 0;
	md_real.user_ptr	= NULL;
	md_real.eventq		= eq;

	if( (rc = PtlMDAttach( me, md_real, PTL_RETAIN, &md )) )
		test_fail( "PtlMDAttach 0", rc );

	printf( "%d: MD %d attached (sleeping)\n", my_nid, md );

	sleep(1);

	id = my_id;
	id.addr_kind = PTL_ADDR_GID;
	id.rid = ( my_nid + 1 ) % nodes;

	printf( "%d: Sending 1024 bytes to %d\n", my_nid, id.rid );
	if( (rc = PtlPut( md, PTL_ACK_REQ, id, 4, 0, 0, 12, 0 ) ) ) 
		test_fail( "PtlPut", rc );

	
	signal( SIGALRM, timeout );
	alarm( 10 );
	for( i=0 ; i<4 ; i++ ) {
		PtlEQWait( eq, &ev );
		printf( "%d: Event: type=%d\n", my_nid, ev.type );
	}

	if( (rc = PtlEQGet( eq, &ev ) ) )
		test_fail( "PtlEQGet", rc );

	printf( "%d: exiting...\n", my_nid );
	return 0;
}


