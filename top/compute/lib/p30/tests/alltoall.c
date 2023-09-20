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
 *  alltoall.c
 *
 * All portals processes send a message to every other one.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>

#include <p30.h>

#define xSTR( x ) #x
#define STR( x ) xSTR( x )

static int alltoall_debug = 0;

int my_rid = -1;

#define NODE_BUFFER	9
#define STATUS_REPORT	7
#define RESTART		5

static void segfault( int sig )
{
	fprintf( stderr, "FATAL: %d: Segfaulted...\n", my_rid );
	exit( EXIT_FAILURE );
}

static void test_fail( ptl_handle_ni_t ni, const char *str, int rc )
{
	fprintf( stderr, "FATAL: %d: %s: %s (errno=%d) (%ld)\n",
		my_rid, str, ptl_err_str[rc], rc, time( NULL ) );
	exit( EXIT_FAILURE );
}

int drain_events( const char *tag, ptl_handle_eq_t eq, int messages, int timeout );


int main( int argc, char *argv[] )
{
	int			rc, i;
	ptl_handle_ni_t		ni;
	ptl_handle_me_t		me;
	ptl_md_t		md_real;
	ptl_handle_md_t		md_in, md_out;
	ptl_handle_eq_t		eq, status_eq, restart_eq;
	ptl_event_t		ev;
	ptl_id_t		nodes;
	ptl_process_id_t	my_id, id;

	int			outgoing[1];
	volatile int		restart;
	int	*volatile incoming;
	int	*volatile status;
	int			failed = 0;
	int			count = 16;

	signal( SIGSEGV, segfault );

	if( (rc = PtlInit()) )
		test_fail( ni, "PtlINit", rc );

	if( (rc = PtlNIInit( PTL_IFACE_DEFAULT, 16, 0, &ni ) ) )
		test_fail( ni, "PtlNIInit: " STR( PTL_IFACE_DEFAULT ), rc );

	PtlNIDebug( ni, PTL_DEBUG_DROP | PTL_DEBUG_THRESHOLD );

	if( (rc = PtlGetId( ni, &my_id, &nodes ) ) )
		test_fail( ni, "PtlGetID", rc );

	my_rid	= my_id.rid;
	id.addr_kind	= PTL_ADDR_GID;
	id.gid	= PTL_ID_ANY;
	id.rid	= PTL_ID_ANY;

	if( argc > 1 )
		count = atoi( argv[1] );

	printf( "%d: gid=%d out of %d nodes\n", my_rid, my_id.gid, nodes );

	if( (rc = PtlMEAttach( ni, NODE_BUFFER, id, 0x0, 0x0, PTL_RETAIN, &me )) )
		test_fail( ni, "PtlMEAttach 1", rc );

	printf( "%d: We have ME %d on index %d\n", my_rid, me, NODE_BUFFER );

	if( (rc = PtlEQAlloc( ni, nodes * 2, &eq ) ) )
		test_fail( ni, "PtlEQAlloc", rc );
	if( (rc = PtlEQAlloc( ni, 2, &restart_eq ) ) )
		test_fail( ni, "PtlEQAlloc: restart_eq", rc );


	/*
	 *  Build the incoming MD and initialize the buffer
	 * such that we can tell if a message has arrived from a
	 * node yet.
	 */

	incoming		= malloc( sizeof(int) * nodes );
	for( i=0 ; i<nodes ; i++ )
		incoming[i] = -1;

	md_real.start		= incoming;
	md_real.length		= sizeof(int) * nodes;
	md_real.threshold	= -1;
	md_real.max_offset      = md_real.length;
	md_real.options		= PTL_MD_OP_PUT
				| PTL_MD_MANAGE_REMOTE
				| 0;
	md_real.user_ptr	= NULL;
	md_real.eventq		= eq;

	if( (rc = PtlMDAttach( me, md_real, PTL_RETAIN, &md_in )) )
		test_fail( ni, "PtlMDAttach", rc );

		
	/*
	 * All nodes need an outgoing-only MD
	 */
	md_real.start		= outgoing;
	md_real.length		= sizeof(int);
	md_real.options		= PTL_MD_OP_PUT;
	md_real.eventq		= PTL_EQ_NONE;
	if( (rc = PtlMDBind( ni, md_real, &md_out )) )
		test_fail( ni, "PtlMDBind", rc );

	/*
	 * Node zero has an additional ME/MD for status reports
	 */
	if( my_rid == 0 ) {
		ptl_handle_me_t status_me;
		ptl_handle_md_t status_md;

		if( (rc = PtlEQAlloc( ni, nodes * 2, &status_eq ) ) )
			test_fail( ni, "PtlEQAlloc: status_eq", rc );

		if( (rc = PtlMEAttach( ni, STATUS_REPORT, id, 0, 0, PTL_RETAIN, &status_me )))
			test_fail( ni, "PtlMEAttach: status_me", rc );

		md_real.length	= sizeof(int) * nodes;
		md_real.start	= status = malloc( md_real.length );
		md_real.options	= PTL_MD_OP_PUT
				| PTL_MD_MANAGE_REMOTE;
		md_real.eventq	= status_eq;
	
		if( (rc = PtlMDAttach( status_me, md_real, PTL_RETAIN, &status_md )))
			test_fail( ni, "PtlMDAttach: status_md", rc );

	}

	/*
	 * All nodes have a restart MD
	 */
	{
		ptl_handle_me_t	restart_me;
		ptl_handle_md_t	restart_md;

		if( (rc = PtlMEAttach( ni, RESTART, id, 0, 0, PTL_RETAIN, &restart_me )))
			test_fail( ni, "PtlMEAttach: restart_me", rc );

		md_real.length	= sizeof(int);
		md_real.start	= (void *) &restart;
		md_real.options	= PTL_MD_OP_PUT
				| PTL_MD_MANAGE_REMOTE;
		md_real.eventq	= restart_eq;

		if( (rc = PtlMDAttach( restart_me, md_real, PTL_RETAIN, &restart_md )))
			test_fail( ni, "PtlMDAttach: restart_md", rc );
	}
		

		
	
	printf( "%d: Setup. Barrier to sync\n", my_rid );
	rc = PtlNIBarrier( ni );
	if( rc )
		test_fail( ni, "PtlNIBarrier", rc );

	printf( "%d: Awake and sending\n", my_rid );

restart:
	id = my_id;
	id.addr_kind	= PTL_ADDR_GID;
	outgoing[0]	= my_rid;

	for( i=0 ; i<nodes ; i++ ) {
		id.nid = i;
		id.rid = i;

		rc = PtlPut( md_out, PTL_ACK_REQ, id,
			NODE_BUFFER, 0, 0, sizeof(int)*my_rid, 0 );

		if( rc )
			test_fail( ni, "PtlMDPut", rc );
	}

/*
	printf( "%d: Sent to %d nodes.  Sleeping now\n", my_rid, nodes );
	sleep(1);
*/

	if( drain_events( "Exchange", eq, nodes, 5 ) != nodes )
		fprintf( stderr, "%d: exchange timed out\n", my_rid );

	for( i=0 ; i<nodes ; i++ ) {
		if( incoming[i] != i ) {
			printf( "%d: Didn't hear from %d (%d)\n",
				my_rid, i, incoming[i] );
			failed = 1;
		}
	}

	/*
	 * Check in with node zero
	 */
	restart = -1;
	outgoing[0] = failed;
	id.rid = 0;
	PtlPut( md_out, PTL_NOACK_REQ, id,
		STATUS_REPORT, 0, 0, sizeof(int) * my_rid, 0 );

	/*
	 * Node zero waits for status reports from all nodes and
	 * prints diagnostics.
	 */
	failed = 0;

	if( my_rid == 0 ) {
/*
		printf( "%d: Waiting for status report (%ld)\n",
			my_rid, time( NULL ) );
*/

		if( drain_events( "Status", status_eq, nodes, 5 ) != nodes )
			fprintf( stderr, "%d: Status timed out\n", my_rid );

		for( i=0 ; i<nodes ; i++ ) {
			if( status[i] < 0 ) {
				printf( "%d: Node %d didn't check in\n",
					my_rid, i );
				failed = 1;
			}

			if( status[i] > 0 ) {
				printf( "%d: Node %d had a failure: %d\n",
					my_rid, i, status[i] );
				failed = 1;
			}

			status[i] = -1;
		}

		printf( "%d: count=%d: %s\n", my_rid, count,
			(failed ? "FAILED!" : "passed") );

		outgoing[0] = !failed;
		if( count-- <= 0 ) {
			printf( "%d: PASSED ALL TESTS\n", my_rid );
			outgoing[0] = 0;
		}

		while( PtlEQGet( status_eq, &ev ) != PTL_EQ_EMPTY )
			/* Nothing */;

		for( i=0 ; i<nodes ; i++ ) {
			id.rid = i;
			PtlPut( md_out, PTL_NOACK_REQ, id, RESTART, 0, 0, 0, 0 );
		}

	}

	/*
	 * Wait for the restart code from the root node.
	 * If it doesn't arrive in five seconds, signal an error
	 * and get out of here.
	 */
	if( drain_events( "Restart", restart_eq, 1, 15 ) != 1 ) {
		fprintf( stderr, "%d: Never received restart code\n", my_rid );
		restart = 0;
	}

	if( alltoall_debug )
		printf( "%d: Restart received ok: %d\n",
			my_rid, restart );

	if( !restart ) {
		ptl_sr_value_t	send_count	= 3,
				send_length	= 9,
				drop_count	= 12,
				msgs_max;

		PtlNIStatus( ni, PTL_SR_SEND_COUNT, &send_count );
		PtlNIStatus( ni, PTL_SR_SEND_LENGTH, &send_length );
		PtlNIStatus( ni, PTL_SR_DROP_COUNT, &drop_count );
		PtlNIStatus( ni, PTL_SR_MSGS_MAX, &msgs_max );

		printf( "%d: sent: %d/%d bytes.  Drop %d. %d msgs used.\n",
			my_rid,
			send_count, send_length, drop_count, msgs_max );

		PtlNIFini( ni );
		return 0;
	}

	goto restart;
}


int drain_events( const char *tag, ptl_handle_eq_t eq, int messages, int timeout )
{
	int		rc;
	ptl_event_t	ev;
	int		drain_count = 0;

	while( drain_count < messages ) {
		rc = PtlEQWait_timeout( eq, &ev, timeout );

		if( rc == PTL_EQ_EMPTY )
			return drain_count;

		drain_count++;
	}

	return drain_count;
}

		
#if 0
static jmp_buf	drain_buf;

static void time_out( int sig )
{
	longjmp( drain_buf, -1 );
}

int old_drain_events( const char *tag, ptl_handle_eq_t eq, int messages, int timeout )
{
	ptl_event_t	ev;
	int		rc;
	static void	*prev;
	static int	drain_count = 0;

	prev = signal( SIGALRM, time_out );
	drain_count = 0;

	if( setjmp( drain_buf ) ) {
		alarm(0);
		signal( SIGALRM, prev );
		fprintf( stderr, "%d: Timed out waiting for %s events\n",
			my_rid, tag );
		return drain_count;
	}

	alarm( timeout );

	while( drain_count < messages ) {
		rc = PtlEQWait( eq, &ev );

		if( rc == PTL_EQ_EMPTY )
			continue;

		if( alltoall_debug )
			printf( "%d: PtlEQGet %s: %s (%d)\n",
				my_rid, tag, ptl_err_str[rc], rc );

		if( rc == PTL_EQ_DROPPED )
			fprintf( stderr, "%d: %s: LOST EVENTS\n", my_rid, tag );
		if( ev.type == PTL_EVENT_PUT )
			drain_count++;
	}

	alarm(0);
	signal( SIGALRM, prev );

	return drain_count;
}

#endif
