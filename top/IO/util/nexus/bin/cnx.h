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
 * Miscellaneous declarations and definitions needed.
 *
 * $Id: cnx.h,v 1.1 1999/11/29 19:34:38 lward Exp $
 */

/*
 * Source file version identification.
 */
#if defined(__GNUC__)
#define IDENTIFY(s) \
	static const char __attribute__ ((__unused__)) rcsid[] = s
#else
#define IDENTIFY(s) \
	static const char rcsid[] = s
#endif

extern int cnx_staterr(cnx_status);
