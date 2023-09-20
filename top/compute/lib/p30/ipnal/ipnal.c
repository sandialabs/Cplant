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
 * ipnal/ipnal.c
 *
 * User level code for implementing the P30 library over TCP/IP
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <p30.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>	/* sched_yeild */


static int ipnal_debug	= 0;

extern void *ipnal_thread( void *arg );

static int fail_write( const char *err )
{
	fprintf( stderr, "ipnal: failed: %s\n", err );
	return PTL_SEGV;
}
	
static int forward(
	nal_t	*nal,
	int	id,
	void	*args,	size_t args_len,
	void	*ret,	size_t ret_len
)
{
	int to_lib	= ((long int) nal->nal_data) >> 16 & 0xFFFF;
	int from_lib	= ((long int) nal->nal_data) >>  0 & 0xFFFF;

	if( ipnal_debug )
		printf( "calling api %d\n", id );

	if( !write( to_lib, &id, sizeof( id ) ) )
		return fail_write( "send index" );

	if( !write( to_lib, &args_len, sizeof( args_len ) ) )
		return fail_write( "send arg length" );

	if( !write( to_lib, &ret_len, sizeof( ret_len ) ) )
		return fail_write( "send ret length" );

	if( !write( to_lib, args, args_len ) )
		return fail_write( "send args" );

	if( ipnal_debug )
		printf( "api %d called.  Waiting on fd %d in %p [%ld]\n",
			id, from_lib, ret, (long) ret_len );


	if( read( from_lib, ret, ret_len ) != ret_len ) {
		perror( "ipnal: read return block" );
		return PTL_SEGV;
	}

	if( ipnal_debug )
		printf( "return result read: %d\n", *(int*)ret );

	return PTL_OK;
}

static int shutdown(
	nal_t	*nal,
	int	ni
)
{
	int to_lib	= ((long int) nal->nal_data) >> 16 & 0xFFFF;
	int from_lib	= ((long int) nal->nal_data) >>  0 & 0xFFFF;

	close( to_lib );
	close( from_lib );

	return 0;
}

static int validate( nal_t *nal, void *base, size_t extent )
{
	/* We don't do anything... */
	if( ipnal_debug )
		printf( "validating %p of length %ld\n",
			base, (long) extent );
	return 0;
}


/*
 * This speeds the system up by many, many times.
 * It is defined in POSIX, so we should be portable to use it
 * if _POSIX_PRIORITY_SCHEDULING is defined in <unistd.h>.
 */
static void yield( nal_t *nal )
{
#if defined( _POSIX_PRIORITY_SCHEDULING )
		sched_yield();
#else
		/* Do nothing */
#endif
}

static nal_t ipnal = {
	{0},
	NULL,
	NULL,
	forward,
	shutdown,
	validate,
	yield
};


nal_t *PTL_IFACE_IP(
	int		interface,
	ptl_pt_index_t	ptl_size,
	ptl_ac_index_t	ac_size
)
{
	int		to_lib[2],
			from_lib[2];
	size_t		rc = 0xDEAD;
	void		*thread_data;
	pthread_t	t;

	if( __p30_ip_initialized ) {
	    return &ipnal;
	}

	if( pipe( to_lib ) ) {
		perror( "ipnal_init: pipe" );
		return NULL;
	}

	if( pipe( from_lib ) ) {
		perror( "ipnal_init: pipe" );
		return NULL;
	}

	if( ipnal_debug )
		printf( "ipnal_init: to %d from %d\n", to_lib[1], from_lib[0] );

	/*
	 * Grody hack to pack the two fds into a usable void* space
	 * May fail if there are too many fds open.  16 bits should
	 * be plenty on most systems, however.
	 */
	ipnal.nal_data	= (void*) (   to_lib[1] << 16 | from_lib[0] );
	thread_data	= (void*) ( from_lib[1] << 16 | to_lib[0] );
	
	write( to_lib[1], &ptl_size, sizeof( ptl_size ) );
	write( to_lib[1], &ac_size,  sizeof( ac_size  ) );

	if( pthread_create( &t, NULL, ipnal_thread, thread_data ) ) {
		perror( "ipnal_init: pthread_create" );
		return NULL;
	}

	read( from_lib[0], &rc, sizeof( rc ) );

	if( rc )
		return NULL;

        ipnal.timeout = &__p30_ip_timeout;

        __p30_ip_initialized = 1;
	__ip_ni_handle = (interface << 16);

	return &ipnal;
}
