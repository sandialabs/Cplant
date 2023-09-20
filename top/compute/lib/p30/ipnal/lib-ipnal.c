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
 * ipnal/lib-ipnal.c
 *
 * This file implements the NAL side of the Portals 3 over TCP
 * (file descriptors, actually).   It is a very simplistic implementation
 * that does not include any packetization.  All messages are sent
 * or received in a synchronous fashion, so it does not show the
 * full procedure for calling lib_finalize().  The message life cycle
 * document describes how to do so.
 *
 * In order to implement the "application bypass" aspect of the API
 * this portion of the ipnal runs in a separate thread (implemented
 * via pthreads) that spends most of its time in a select wait.
 * It can write directly into the application's address space
 * as the messages are pulled in.  While not actually application
 * bypass (since the app is suspended while the other thread runs)
 * it does not require the app to perform any special operations
 * to receive the messages or send the reply.
 * 
 * Communication across the thread boundaries is done via a
 * local pipe.  In response to a api_nal->forward() method call
 * the user level ipnal writes an integer function
 * index, the size of the argument block length, the size of the
 * return block length and then the bytes of the argument length.
 * It then waits for ret_len bytes to be written to the pipe
 * before returning to the Portals 3 API code that it.
 *
 * I do not like the nal->gid2rid() methods.  They are rather hackish
 * and are included soley to support a Portals 2 design that may or
 * may not make sense in the new world of P3.  I have a vision of
 * per-NI addressing instead.
 *
 * The memory model also needs some modifications.  I would like to
 * allow the NAL the opportunity to include a NAL-translated address
 * and a NAL-specific token for every user addresses.  The Portals 3
 * library would never perform any modification to these tokens
 * and will make all requests of the NAL with four components:
 *
 *	- NAL translated address
 *	- NAL specific token
 *	- Offset in bytes
 *	- Obligatory Diversity Component from a protected minority group.
 * 
 * The current model gives the NAL a chance to translate/validate the
 * address and extent but not to store the information with the object
 * that required the translation.  This needs to be changed for efficient
 * address use once the message starts to arrive -- the less time that
 * the NAL needs to compute where to deliver the message the better
 * our overall performance.
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <p30.h>
#include <lib-p30.h>
#include <p30/lib-dispatch.h>
#include <p30/lib-nal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct {
	int	my_nid;
	int	my_rid;
	int	my_gid;
	int	num_nodes;
	int	*to_nodes;
	int	*from_nodes;
	int	*pid_map;
	int	*nid_map;
	int	to_api;
	int	from_api;
	fd_set	select;
} ipnal_data_t;

static ipnal_data_t data;

static void spin_loop( nal_cb_t *nal );
static int open_communication( ipnal_data_t *data );

int ipnal_debug = 0;

static int ipnal_send(
	nal_cb_t	*nal,
	void		*private,
	lib_msg_t	*cookie,
	ptl_hdr_t	*hdr,
	int		nid,
	int		pid,
	int		gid,
	int		rid,
	user_ptr	data,
	size_t		len
)
{
	int fd;

	if( nid < 0 || nid >= nal->ni.num_nodes )
		return -1;

	fd = ((ipnal_data_t*)nal->nal_data)->to_nodes[ rid ];

	if( ipnal_debug )
		nal->cb_printf( nal, "%d: ipnal_send: "
			"%ld bytes from %p -> dst_rid %d on fd %d\n",
			nal->ni.rid, (long)len, data, rid, fd );

	if( ipnal_debug > 1 ) {
		int i;
		char *buf = data;
		nal->cb_printf( nal, "%d: ", nal->ni.rid );
		for( i=0 ; i<len ; i++ )
			printf( "%02x ", buf[i] );
		nal->cb_printf( nal, "\n" );
	}

	if( write( fd, hdr, sizeof( ptl_hdr_t ) ) != sizeof( ptl_hdr_t ) ) {
		perror( "ipnal_send: hdr write" );
		return PTL_SEGV;
	}

	if( write( fd, data, len ) != len ) {
		perror( "ipnal_send: data write" );
		return PTL_SEGV;
	}

	lib_finalize( nal, private, cookie );
		
	return 0;
}


static int ipnal_recv(
	nal_cb_t	*nal,
	void		*private,
	lib_msg_t	*cookie,
	user_ptr 	data,
	size_t		mlen,
	size_t		rlen
)
{
	ssize_t		i,
			rc = 0,
			read_so_far = 0;
	char		c;

	if( ipnal_debug )
		nal->cb_printf( nal, "%d: ipnal_recv: "
			"%ld bytes -> %p, drop %ld bytes\n",
			nal->ni.nid, (long)mlen, data, (long) (rlen-mlen) );

	while( read_so_far < mlen ) {
		rc = read( (int) private, data, mlen - read_so_far  );
		if( rc < 0 ) {
			nal->cb_printf( nal, "%d: Error on fd %d ",
				nal->ni.nid, (int) private );
			perror( "read" );
			return -1;
		}
				
		read_so_far += rc;
	}

	if( read_so_far != mlen ) {
		nal->cb_printf( nal, "%d: Read %ld bytes instead of %ld\n",
			nal->ni.nid, (long) rc, (long) mlen );
	}

	if( ipnal_debug > 1 ) {
		char *buf = data;
		nal->cb_printf( nal, "%d: ", nal->ni.nid );
		for( i=0 ; i<mlen ; i++ )
			nal->cb_printf( nal, "%02x ", buf[i] );
		nal->cb_printf( nal, "\n" );
	}

	for( i=0 ; i < (rlen-mlen) ; i++ )
		read( (int) private, &c, 1 );

	lib_finalize( nal, private, cookie );

	return rlen;
}


static int ipnal_write(
	nal_cb_t	*nal,
	void		*private,
	user_ptr	dst_addr,
	void		*src_addr,
	size_t		len
)
{
	if( ipnal_debug )
		nal->cb_printf( nal, "%d: ipnal_write: "
			"writing %ld bytes from %p -> %p\n",
			nal->ni.nid, (long)len, src_addr, dst_addr );

	memcpy( dst_addr, src_addr, len );
	return 0;
}

static void *ipnal_malloc(
	nal_cb_t	*nal,
	size_t		len
)
{
	void *buf =  malloc( len );
	if( ipnal_debug )
		nal->cb_printf( nal, "%d: ipnal_thread: allocating %lu -> %p\n",
			nal->ni.rid, (unsigned long) len, buf );

	return buf;
}

static void ipnal_free(
	nal_cb_t	*nal,
	void		*buf
)
{
	if( ipnal_debug )
		nal->cb_printf( nal, "%d: ipnal_thread: freeing %p\n",
			nal->ni.rid, buf );

	free( buf );
}

static void ipnal_invalidate(
	nal_cb_t	*nal,
	void		*base,
	size_t		extent,
	void            *addrkey
)
{
	/* Nothing to do... */
	if( ipnal_debug )
		nal->cb_printf( nal, "%d: invalidating %p : %d\n",
			nal->ni.rid, base, extent );
}


static void ipnal_validate(
	nal_cb_t	*nal,
	void		*base,
	size_t		extent,
	void            *addrkey
)
{
	/* Nothing to do... */
	if( ipnal_debug )
		nal->cb_printf( nal, "%d: validating %p : %d\n",
			nal->ni.rid, base, extent );
}

static void ipnal_printf( 
	nal_cb_t	*nal,
	const char	*fmt,
	...
)
{
	va_list		ap;

	va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end( ap );
}


static void ipnal_cli(
	nal_cb_t	*nal,
	unsigned long	*flags
)
{
}


static void ipnal_sti(
	nal_cb_t	*nal,
	unsigned long	*flags
)
{
}


static int ipnal_nidpid2gidrid( 
	nal_cb_t	*nal,
	ptl_id_t	nid,
	ptl_id_t	pid,
	ptl_id_t	*gid,
	ptl_id_t	*rid
)
{
	nal->cb_printf( nal, "%d: ipnal_nidpid2gidrid() not implemented yet!\n",
		nal->ni.rid );
	return -1;
}


static int ipnal_gidrid2nidpid(
	nal_cb_t	*nal,
	ptl_id_t	gid,
	ptl_id_t	rid,
	ptl_id_t	*nid,
	ptl_id_t	*pid
)
{
	ipnal_data_t	*data = nal->nal_data;

	if( gid != nal->ni.gid || rid < 0 || rid >= nal->ni.num_nodes )
		return -1;

	if( data->nid_map ) {
		*nid = data->nid_map[ rid ];
		*pid = data->pid_map[ rid ];
	} else {
		*nid = *pid = 0;
	}

	return 0;
}


static int ipnal_dist(
	nal_cb_t        *nal,
	ptl_id_t         nid,
	unsigned long   *dist
)
{
	/* network distance doesn't mean much for this nal */
	if ( nal->ni.nid == nid ) {
		*dist = 0;
	} else {
		*dist = 1;
	}

	return 0;
}
	

static nal_cb_t static_cb = {
	{0},		/* NI data */
	&data,		/* NAL private data */
	ipnal_send,
	ipnal_recv,
	ipnal_write,
	ipnal_malloc,
	ipnal_free,
	ipnal_invalidate,
	ipnal_validate,
	ipnal_printf,
	ipnal_cli,
	ipnal_sti,
	ipnal_gidrid2nidpid,
	ipnal_nidpid2gidrid,
	ipnal_dist
};

void * ipnal_thread( void *arg )
{
	ssize_t		rc;
	ptl_pt_index_t	ptl_size;
	ptl_ac_index_t	ac_size;

	data.to_api	= ((long int) arg) >> 16 & 0xFFFF;
	data.from_api	= ((long int) arg) >>  0 & 0xFFFF;


	if( ipnal_debug )
		printf( "ipnal_thread: Starting up.  to %d from %d\n",
			data.to_api, data.from_api );

	if( open_communication( &data ) ) {
		rc = PTL_NAL_FAILED;
		write( data.to_api, &rc, sizeof( rc ) );
		return NULL;
	}

	if( ipnal_debug )
		printf( "ipnal_thread: Communication paths are open\n" );

	
	/*
	 * First step is to read the portal table size and access control
	 * list size from the user side and then initialize the library.
	 */
	if( !(rc = read( data.from_api, &ptl_size, sizeof( ptl_size ))) )
		fprintf( stderr, "ipnal_thread: Read %ld bytes\n", (long) rc );

	if( !(rc = read( data.from_api, &ac_size, sizeof( ac_size ))) )
		fprintf( stderr, "ipnal_thread: Read %ld bytes\n", (long) rc );

	if( ipnal_debug )
		printf( "%d: ipnal_thread: Setting up table %d / %d\n",
			data.my_rid, ptl_size, ac_size );

	rc = lib_init( &static_cb,
		data.my_nid, getpid(), data.my_rid, data.my_gid, data.num_nodes,
		ptl_size, ac_size);

	if( ipnal_debug )
		printf( "%d: ipnal_thread: Library initialized\n",
			static_cb.ni.rid );

	/*
	 * Whatever the initialization returned is passed back to the
	 * user level code for further interpretation.  We just exit if
	 * it is non-zero since something went wrong.
	 */
	write( data.to_api, &rc, sizeof( rc ) );

	if( !rc )
		spin_loop( &static_cb );
	else
		fprintf( stderr, "%d: ipnal_thread: init failed\n",
			static_cb.ni.rid );

	return NULL;
}

static void spin_loop( nal_cb_t *nal )
{
	int	index, i;
	size_t	arg_len,
		ret_len;
	char	arg_block[ 256 ];
	char	ret_block[ 128 ];
	ipnal_data_t	*data	= (ipnal_data_t*) nal->nal_data;

	int	rc;
	int	to_api		= data->to_api;
	int	from_api	= data->from_api;

	printf( "%d: In spin_loop\n",
		nal->ni.rid );

	while(1) {
		fd_set		fds;
		ptl_hdr_t	hdr;

		fds = data->select;
		FD_SET( from_api, &fds );

		rc = select( 256, &fds, NULL, NULL, NULL );
		if( rc == 0 )
			continue;
		if( rc  < 0 ) {
			perror( "select" );
			lib_fini( nal );
			return;
		}

		if( FD_ISSET( from_api, &fds ) ) {
			if( read( from_api, &index, sizeof( index ) ) == 0) {
				lib_fini( nal );
				return;
			}
			if( ipnal_debug )
				printf( "%d: ipnal_thread: api call %d\n",
					nal->ni.rid, index );

			read( from_api, &arg_len, sizeof( arg_len ) );
			read( from_api, &ret_len, sizeof( ret_len ) );
			read( from_api, arg_block, arg_len );

			/* We don't need any private data */
			lib_dispatch( nal, NULL, index, arg_block, ret_block );

			if( ret_len != write( to_api, ret_block, ret_len ) ) {
				perror( "ipnal_thread: write return" );
				lib_fini( nal );
				return;
			}
		}

		for( i=3 ; i<128 ; i++ ) {
			if( i == from_api || ! FD_ISSET( i, &fds ) )
				continue;

			if( ipnal_debug )
				printf( "%d: ipnal_thread: fd %d is writing\n",
					nal->ni.rid, i );
			read( i, &hdr, sizeof( ptl_hdr_t ) );
			lib_parse( nal, &hdr, (void*) i );
		}
	}

}


static int open_communication( ipnal_data_t *data )
{
	int		i;
	fd_set		fds;
	const unsigned char	*to_node_s;
	const unsigned char	*from_node_s;


	to_node_s = (const unsigned char *) getenv( "PTL_TO_NODES" );
	from_node_s = (const unsigned char *) getenv( "PTL_FROM_NODES" );

	if( getenv( "PTL_MY_NODE" ) == NULL ) {
		fprintf( stderr,
			"\n\n\a-------------\n"
			"You should start this with the fork command to\n"
			"ensure that all of the environment variables are\n"
			"properly initialized\n"
			"\n"
			"	PTL_MY_RANK	Rank id of this process\n"
			"	PTL_MY_NODE	Node id of this process\n"
			"	PTL_NUM_NODES	Number of nodes in this group\n"
			"	PTL_TO_NODES	List of file descriptors\n"
			"	PTL_FROM_NODES	   for communication\n"
			"\n"
			"\a-------------\n\n\n"
		);
		return -1;
	}

	data->my_rid	= atoi( getenv( "PTL_MY_RANK" ) );
	data->my_gid	= atoi( getenv( "PTL_MY_GID" ) );
	data->my_nid	= atoi( getenv( "PTL_MY_NODE" ) );
	data->num_nodes	= atoi( getenv( "PTL_NUM_NODES" ) );
	data->to_nodes	= malloc( sizeof( int ) * data->num_nodes );
	data->from_nodes= malloc( sizeof( int ) * data->num_nodes );
	data->nid_map	= (int *) getenv( "PTL_NIDMAP" );
	data->pid_map	= (int *) getenv( "PTL_PIDMAP" );
	
	if( ipnal_debug )
		printf( "ipnal_thread: I am %d/%d\n",
			data->my_rid, data->num_nodes );

	if( !data->to_nodes || !data->from_nodes ) {
		fprintf( stderr, "ipnal_thread: %d: Failed allocation\n",
			data->my_rid );
		return -1;
	}
	
	for( i=0 ; i< data->num_nodes ; i++ ) {
		data->to_nodes[i]	= (int) to_node_s[i];
		data->from_nodes[i]	= (int) from_node_s[i];

		if( ipnal_debug )
			printf( "%d: -> %d = %d  <- = %d\n",
				data->my_rid, i,
				to_node_s[i], from_node_s[i] );

		FD_SET( data->from_nodes[i], &fds );
	}

	data->select = fds;

	return 0;
}
