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
#include <sys/time.h>

#include <p30.h>


int		nodes;
int		rank = -1;
ptl_process_id_t my_id;

static unsigned char	buf[ 1<<20 ];
static long int		loops	= 1024;
static long int		length	= 1024;

static void test_fail( const char *str, int rc )
{
	fprintf( stderr, "%d: %s: %s (%d)\n",
		rank,
		str,
		ptl_err_str[rc],
		rc );
	exit( EXIT_FAILURE );
}

static struct timeval start_tv, stop_tv;

static inline void start( void ) {
	gettimeofday( &start_tv, NULL );
}

static inline long int stop( void )
{
	long int diff;
	gettimeofday( &stop_tv, NULL );

	if( stop_tv.tv_sec != start_tv.tv_sec )
		printf( "%d: start=%ld:%ld stop=%ld:%ld\n",
			rank,
			start_tv.tv_sec, start_tv.tv_usec,
			stop_tv.tv_sec, stop_tv.tv_usec );

	/* Force evaluation order so that diff is never negative */
	diff = 1000000 * ( stop_tv.tv_sec - start_tv.tv_sec );
	diff += stop_tv.tv_usec;
	diff -= start_tv.tv_usec;

	return diff;
}


static void sender( ptl_handle_ni_t ni )
{
	int		rc,
			i,
			j;
	ptl_handle_md_t	md_h;
	ptl_handle_eq_t	eq;
	ptl_event_t	ev;
	ptl_md_t	md;
	ptl_process_id_t	dest = my_id;
	unsigned long int	diff,
				avg = 200,
				min = (unsigned long int) -1,
				max = 0;

	rc = PtlEQAlloc( ni, 16, &eq );
	if( rc )

		test_fail( "sender: EQAlloc", rc );

	dest.addr_kind= PTL_ADDR_GID;
	dest.rid = (dest.rid + 1) % nodes;

	md.start	= buf;
	md.length	= sizeof( buf );
	md.length	= length;
	md.threshold	= PTL_MD_THRESH_INF;
	md.max_offset   = md.length;
	md.options	= 0;
	md.user_ptr	= NULL;
	md.eventq	= eq;

	if( (rc = PtlMDBind( ni, md, &md_h ) ) )
		test_fail( "sender: PtlMDBind", rc );

	printf( "%d: doing %ld loops of %ld bytes each\n",
		rank, loops, length );

	sleep( 2 );

	for( j=0 ; j<loops ; j++ ) {
		for( i=0 ; i<sizeof( buf ); i++ )
			buf[i] = (j + i) % 256 ;
		buf[sizeof(buf) - 4] = 0xAB;	/* Error */

		start();
		PtlPut( md_h, PTL_ACK_REQ, dest, 4, 0, 0, 0, 0 );
		while( 1 ) {
			if( PtlEQGet( eq, &ev ) == PTL_EQ_EMPTY )
				continue;

			if( ev.type == PTL_EVENT_ACK )
				break;
		}

		diff = stop();

		if( 1 || diff < 10 * avg ) {
			avg += diff;
			if( diff > max )
				max = diff;
			if( diff < min )
				min = diff;

/*
			printf( "%d: msg %d: Done %ld usec avg=%ld/%ld/%ld\n",
				rank, j, diff, min, avg / (j+1), max );
*/
		} else
			printf( "%d: %ld usec >> avg=%ld\n",
				rank, diff, avg );
	}

	printf( "%d: loops=%ld time = %ld/%ld/%ld len=%ld mbs=%ld\n",
		rank, loops,
		min, avg / loops, max,
		length,
		(long) ((length * loops) / avg)
	);
}

static void receive( ptl_handle_ni_t ni )
{
	int		i;	/* Bug in gcc?  remove initializer to get error */
	int		rc;
	ptl_handle_me_t	me;
	ptl_handle_md_t	md_h;
	ptl_md_t	md;
	ptl_process_id_t	src = my_id;

	src.addr_kind = PTL_ADDR_GID;
	src.rid = ( src.rid + 1 ) % nodes;

	md.start	= buf;
	md.length	= sizeof( buf );
	md.threshold	= PTL_MD_THRESH_INF;
	md.max_offset   = md.length;
	md.options	= PTL_MD_TRUNCATE
			| PTL_MD_MANAGE_REMOTE
			| PTL_MD_OP_PUT
			;
	md.eventq	= PTL_EQ_NONE;
	md.user_ptr	= NULL;
	

	if( (rc = PtlMEAttach( ni, 4, src, 0, 0, PTL_RETAIN, &me )) )
		test_fail( "recv: PtlMEAttach", rc );

	if( (rc = PtlMDAttach( me, md, PTL_RETAIN, &md_h )) )
		test_fail( "recv: PtlMDattach", rc );

	printf( "%d: Waiting for bytes\n", rank );
	pause();

	printf( "%d: Done..\n", rank );
	for( i=0 ; i<sizeof(buf) ; i++ ) {
		if( buf[i] != i % 256 ) {
			fprintf( stderr, "%d: buf[%d] = %02x != %02x\n",
				rank, i, (unsigned int) (buf[i]), i % 256 );
			break;
		}
	}

	raise( SIGSEGV );
}

	
int main( int argc, char *argv[] )
{
	int		rc;
	ptl_handle_ni_t	ni;
	ptl_sr_value_t	send_count,
			send_length,
			drop_count;

	printf( "argc=%d\n", argc );
	if( argc > 1 )
		loops = atoi( argv[1] );
	if( argc > 2 )
		length = atoi( argv[2] );

	if( (rc = PtlInit()) )
		test_fail( "PtlInit", rc );

	if( (rc = PtlNIInit( PTL_IFACE_DEFAULT, 16, 0, &ni )) )
		test_fail( "PtlNIInit", rc );

	if( (rc = PtlGetId( ni, &my_id, &nodes )) )
		test_fail( "PtlGetID", rc );

	rank = my_id.rid;
	
	if( rank == 0 ) {
		sender( ni );
	} else {
		receive( ni );
	}

	PtlNIStatus( ni, PTL_SR_SEND_COUNT, &send_count );
	PtlNIStatus( ni, PTL_SR_SEND_LENGTH, &send_length );
	PtlNIStatus( ni, PTL_SR_DROP_COUNT, &drop_count );

	printf( "%d: sent %d msg %d bytes.  dropped %d\n",
		rank, send_count, send_length, drop_count );

	PtlNIFini( ni );
	PtlFini();

	return 0;
}
