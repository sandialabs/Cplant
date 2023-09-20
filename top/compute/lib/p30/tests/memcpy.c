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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>

#define MAX_INDEX	1024
#define DEVICE		"/dev/memcpy"

static int buf[ MAX_INDEX ];

int main( int argc, char *argv[] )
{
	int fd;
	ssize_t	rc;

	printf( "Memcpy test starting up: pid=%d\n", (int) getpid() );
	fd = open( DEVICE, O_RDWR );
	if( fd < 0 ) {
		perror( "open: " DEVICE );
		return EXIT_FAILURE;
	}

	printf( "Have fd %d for /dev/memcpy\n", fd );

	rc = read( fd, buf, sizeof( buf ) );
	if( rc < 0 ) {
		perror( "read: " DEVICE );
		return EXIT_FAILURE;
	}

	printf( "Read into buffer %p, now to watch values\n", buf );

	while( 1 ) {
		int index = buf[0];
		if( index && index < MAX_INDEX ) {
			int value = buf[ index ];
			if( value != index + 0x89ABCDEF ) {
				printf( "buf[%d] = %08x != %08x\n",
					index,
					value,
					buf[ index ]
				);
				return EXIT_FAILURE;
			}
		}

#		if 0
			sched_yield();
#		endif
	}

	return EXIT_SUCCESS;
}
