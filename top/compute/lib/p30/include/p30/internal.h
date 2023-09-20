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
** $Id: internal.h,v 1.9 2000/06/26 19:54:02 rolf Exp $
*/
#ifndef _P30_INTERNAL_H_
#define _P30_INTERNAL_H_

/*
 * p30/internal.h
 *
 * Internals for the API level library that are not needed
 * by the user application
 */

#include <p30.h>

extern int	ptl_init;	/* Has the library be initialized */

#define PTL_MAX_INTERFACES	8
#define PTL_INTERFACE_NUM(h)	(((h)>>16) % PTL_MAX_INTERFACES)
#define PTL_INTERFACE(h)	(ptl_interfaces[ PTL_INTERFACE_NUM(h) ])

extern nal_t *ptl_interfaces[];

extern int ptl_ni_init( void );
extern int ptl_me_init( void );
extern int ptl_md_init( void );
extern int ptl_eq_init( void );

extern int ptl_me_ni_init( nal_t *nal );
extern int ptl_md_ni_init( nal_t *nal );
extern int ptl_eq_ni_init( nal_t *nal );

extern void ptl_ni_fini( void );
extern void ptl_me_fini( void );
extern void ptl_md_fini( void );
extern void ptl_eq_fini( void );

extern void ptl_me_ni_fini( nal_t *nal );
extern void ptl_md_ni_fini( nal_t *nal );
extern void ptl_eq_ni_fini( nal_t *nal );

#endif
