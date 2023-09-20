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
 * $Id: ni.c,v 1.16 2001/05/24 23:12:06 rbbrigh Exp $
 *
 * api-p30/ni.c
 *
 * Network Interface code
 *
 */

#include <p30.h>
#include <p30/internal.h>
#include <p30/coll.h>
#include <stdio.h>

#define MAX_NIS	8
nal_t *ptl_interfaces[ MAX_NIS ];
int ptl_num_interfaces = 0;

int ptl_ni_init( void )
{
	int i;

	for( i=0 ; i<MAX_NIS ; i++ )
		ptl_interfaces[i] = NULL;

	return PTL_OK;
}

void ptl_ni_fini( void)
{
	int i;

	for( i=0 ; i<MAX_NIS ; i++ ) {
		nal_t	*nal = ptl_interfaces[i];
		if( !nal )
			continue;

		if( nal->shutdown )
			nal->shutdown( nal, i );
	}
}


int PtlNIInit(
	ptl_interface_t		interface,
	ptl_pt_index_t		ptl_size,
	ptl_ac_index_t		acl_size,
	ptl_handle_ni_t		*handle
)
{
	nal_t			*nal;
	int                      i;

	if( !ptl_init )
		return PTL_NOINIT;

	nal = interface( ptl_num_interfaces, ptl_size, acl_size );

	if( !nal )
		return PTL_NAL_FAILED;

	for( i=0 ; i<ptl_num_interfaces ; i++ ) {
	    if ( ptl_interfaces[i] == nal ) {
		*handle = (i) << 16;
		return PTL_OK;
	    }
	}

	*handle = (ptl_num_interfaces) << 16;

	if (ptl_num_interfaces >= MAX_NIS)   {
	    return PTL_NOSPACE;
	}

	ptl_interfaces[ ptl_num_interfaces++ ] = nal;

	ptl_eq_ni_init( nal );
	ptl_me_ni_init( nal );
	ptl_md_ni_init( nal );
	ptl_coll_init( *handle );

	return PTL_OK;
}


int PtlNIFini(
	ptl_handle_ni_t		ni
)
{
	nal_t *nal;
	int rc;

	if( !ptl_init )
		return PTL_NOINIT;

	coll_did_init = 0;

	nal = PTL_INTERFACE( ni );

	ptl_coll_fini( ni );
	ptl_md_ni_fini( nal );
	ptl_me_ni_fini( nal );
	ptl_eq_ni_fini( nal );

	rc = PTL_OK;
	if( nal->shutdown )
		rc= nal->shutdown( nal, PTL_INTERFACE_NUM(ni) );

	ptl_interfaces[ PTL_INTERFACE_NUM(ni) ] = NULL;
	ptl_num_interfaces--;


	return rc;
}

int PtlNIHandle(
	ptl_handle_any_t	handle_in,
	ptl_handle_ni_t		*ni_out
)
{
	*ni_out = handle_in & 0xFFFF;

	return PTL_OK;
}

