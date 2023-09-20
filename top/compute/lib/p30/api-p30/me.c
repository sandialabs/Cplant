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
 * $Id: me.c,v 1.3 2001/02/05 01:52:30 lafisk Exp $
 *
 * api-p30/me.c
 *
 * Match Entry local operations.
 *
 */

#include <p30.h>

int	ptl_me_init( void )		{ return PTL_OK; }
void	ptl_me_fini( void )		{ /* Nothing to do */ }
int	ptl_me_ni_init( nal_t *nal )	{ return PTL_OK; }
void 	ptl_me_ni_fini( nal_t *nal )	{ /* Nothing to do... */ }

