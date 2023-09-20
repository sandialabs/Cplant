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
**  This Portals 3.0 program tests the PtlNIDist() function 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p30.h"

static ptl_id_t	rank;

int main( int argc, char *argv[] )
{
    ptl_process_id_t	my_id,id;
    ptl_id_t		size;
    ptl_handle_ni_t	ni_handle;
    unsigned long       distance;
    int                 i,j;
    int                 rc;

    /* initialize library */
    if ( (rc = PtlInit()) != PTL_OK ) {
	fprintf(stderr,"PtlInit() failed rc = %d: %s\n",rc,ptl_err_str[rc]);
	exit(1);
    }

    /* Initialize the Interface */
    if ( (rc = PtlNIInit( PTL_IFACE_DEFAULT, 32, 2, &ni_handle )) != PTL_OK)   {
	fprintf(stderr,"PtlNIInit() failed rc = %d: %s\n",rc,ptl_err_str[rc]);
	exit(1);
    }

    /* get my id and size */
    if ( (rc = PtlGetId( ni_handle, &my_id, &size )) != PTL_OK ) {
	fprintf(stderr,"PtlGetId() failed rc = %d: %s\n",rc,ptl_err_str[rc]);
	exit(1);
    }

    /* get my rank id */
    rank = my_id.rid;

    id.addr_kind = PTL_ADDR_GID;
    id.gid       = my_id.gid;

    for ( i=0; i<size; i++ ) {
	if ( i == rank ) {
	    for ( j=0; j<size; j++ ) {
		id.rid = j;
		if ( PtlNIDist( ni_handle, id, &distance ) != PTL_OK ) {
		    fprintf(stderr,"PtlNIDist() failed\n");
		    return 1;
		}
		printf("from %d to %ld => %d\n",i,j,distance );
	    }
	} else {
	    PtlNIBarrier( ni_handle );
	}
    }

    id.addr_kind = PTL_ADDR_NID;
    id.pid       = 0;

    id.nid       = 9;
    if ( PtlNIDist( ni_handle, id, &distance ) != PTL_OK ) {
	fprintf(stderr,"PtlNIDist() failed\n");
	return 1;
    }
    printf("from %d to %d => %ld\n",rank,id.nid,distance );

    id.nid       = 17;
    if ( PtlNIDist( ni_handle, id, &distance ) != PTL_OK ) {
	fprintf(stderr,"PtlNIDist() failed\n");
	return 1;
    }
    printf("from %d to %d => %ld\n",rank,id.nid,distance );

    id.nid       = 25;
    if ( PtlNIDist( ni_handle, id, &distance ) != PTL_OK ) {
	fprintf(stderr,"PtlNIDist() failed\n");
	return 1;
    }
    printf("from %d to %d => %ld\n",rank,id.nid,distance );

    id.nid       = 1024;
    if ( PtlNIDist( ni_handle, id, &distance ) != PTL_OK ) {
	fprintf(stderr,"PtlNIDist() failed\n");
	return 1;
    }
    printf("from %d to %d => %ld\n",rank,id.nid,distance );

    PtlNIFini( ni_handle );

    /* finalize library */
    PtlFini();

    return 0;
}
