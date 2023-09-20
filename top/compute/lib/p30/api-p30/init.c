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
 * $Id: init.c,v 1.7 2001/05/24 23:12:06 rbbrigh Exp $
 *
 * api-p30/init.c
 *
 * Initialization and global data for the p30 user side library
 *
 * All handles have their interface number stored in the second 16 bit word
 */

#include <p30.h>
#include <p30/internal.h>
#include <stdio.h>

int ptl_init = 0;

int __p30_initialized=0; 
int __p30_myr_initialized=0; 
int __p30_ip_initialized=0; 
ptl_handle_ni_t __myr_ni_handle;
ptl_handle_ni_t __ip_ni_handle;

int __p30_myr_timeout = 10;
int __p30_ip_timeout  = 0;

int PtlInit( void )
{

	if( ptl_init )
		return PTL_OK;

	ptl_ni_init();
	ptl_me_init();
	ptl_md_init();
	ptl_eq_init();

	ptl_init = 1;
	__p30_initialized=1;

	return PTL_OK;
}


void PtlFini( void )
{

	/* Reverse order of initialization */
	ptl_eq_fini();
	ptl_md_fini();
	ptl_me_fini();
	ptl_ni_fini();

	ptl_init = 0;
}
