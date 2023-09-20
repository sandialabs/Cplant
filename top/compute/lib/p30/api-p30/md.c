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
 * $Id: md.c,v 1.6 2001/02/05 01:52:30 lafisk Exp $
 *
 * api-p30/md.c
 *
 * Memory descriptor functions that need address validation
 * There are a few standing issues...
 *  - Addresses are invalidated by the library without telling us.
 */
#include <stdio.h>
#include <p30.h>
#include <p30/internal.h>
#include <p30/nal.h>


int	 ptl_md_init( void )		 { return PTL_OK; }
void	 ptl_md_fini( void )		 { /* Nothing to do */ }
int	 ptl_md_ni_init( nal_t *nal )	 { return PTL_OK; }
void	 ptl_md_ni_fini( nal_t *nal )	 { /* Nothing to do */ }


int PtlMDAttach(
	ptl_handle_me_t	current_in,
	ptl_md_t	md_in,
	ptl_unlink_t	unlink_in,
	ptl_handle_md_t	*handle_out
)
{
	int		rc;
	nal_t		*nal;
	nal = ptl_interfaces[(current_in >> 16) & 0xF];
	if( !nal )
		return PTL_NOINIT;

	rc = nal->validate( nal, md_in.start, md_in.length );
	if( rc )
		return PTL_SEGV;

	rc = PtlMDAttach_internal( current_in, md_in, unlink_in, handle_out );
	return rc;
}



int PtlMDInsert(
	ptl_handle_md_t	current_in,
	ptl_md_t	md_in,
	ptl_unlink_t	unlink_in,
	ptl_ins_pos_t	position_in,
	ptl_handle_md_t	*handle_out
)
{
	int		rc;
	nal_t		*nal;
	nal = ptl_interfaces[(current_in >> 16) & 0xF];
	if( !nal )
		return PTL_NOINIT;

	rc = nal->validate( nal, md_in.start, md_in.length );
	if( rc )
		return PTL_SEGV;

	rc = PtlMDInsert_internal( current_in, md_in, unlink_in, position_in, handle_out );

	return rc;
}



int PtlMDBind(
	ptl_handle_ni_t	ni_in,
	ptl_md_t	md_in,
	ptl_handle_md_t		*	handle_out
)
{
	int		rc;
	nal_t		*nal;
	nal = ptl_interfaces[(ni_in >> 16) & 0xF];
	if( !nal )
		return PTL_NOINIT;

	rc = nal->validate( nal, md_in.start, md_in.length );
	if( rc )
		return PTL_SEGV;

	rc = PtlMDBind_internal( ni_in, md_in, handle_out );
	return rc;
}

int PtlMDUnlink(
	ptl_handle_md_t	md_in
)
{
	int	rc;
	nal_t		*nal;
	ptl_md_t	md;

	nal = ptl_interfaces[(md_in >> 16) & 0xF];
	if( !nal )
		return PTL_NOINIT;

	rc = PtlMDUnlink_internal( md_in, &md );
	return rc;
}


