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
** $Id: p30.h,v 1.12 2001/02/05 01:54:12 lafisk Exp $
*/
#ifndef _P30_H_
#define _P30_H_

/*
 * p30.h
 *
 * User application interface file
 */

#if defined (__KERNEL__)
#include <linux/types.h>
#else
#include <sys/types.h>
#endif

#include <p30/types.h>
#include <p30/nal.h>
#include <p30/api.h>
#include <p30/errno.h>

extern int __p30_initialized;      /* for libraries & test codes  */
extern int __p30_myr_initialized;  /*   that don't know if p30    */
extern int __p30_ip_initialized;   /*   had been initialized yet  */
extern ptl_handle_ni_t __myr_ni_handle, __ip_ni_handle;

extern int __p30_myr_timeout;      /* in seconds, for PtlNIBarrier,     */
extern int __p30_ip_timeout;       /* PtlReduce_all, & PtlBroadcast_all */

/*
 * Debugging flags reserved for the Portals reference library.
 * These are not part of the API as described in the SAND report
 * but are for the use of the maintainers of the reference implementation.
 *
 * It is not expected that the real implementations will export
 * this functionality.
 */
#define PTL_DEBUG_NONE		0ul
#define PTL_DEBUG_ALL		(0x0FFFul)	/* Only the Portals flags */

#define __bit(x)		((unsigned long) 1<<(x))
#define PTL_DEBUG_PUT		__bit(0)
#define PTL_DEBUG_GET		__bit(1)
#define PTL_DEBUG_REPLY		__bit(2)
#define PTL_DEBUG_ACK		__bit(3)
#define PTL_DEBUG_DROP		__bit(4)
#define PTL_DEBUG_REQUEST	__bit(5)
#define PTL_DEBUG_DELIVERY	__bit(6)
#define PTL_DEBUG_UNLINK	__bit(7)
#define PTL_DEBUG_THRESHOLD	__bit(8)
#define PTL_DEBUG_API		__bit(9)

/*
 * These eight are reserved for the NAL to define
 * It should probably give them better names...
 */
#define PTL_DEBUG_NI_ALL	(0xF000ul)	/* Only the NAL flags */
#define PTL_DEBUG_NI0		__bit(24)
#define PTL_DEBUG_NI1		__bit(25)
#define PTL_DEBUG_NI2		__bit(26)
#define PTL_DEBUG_NI3		__bit(27)
#define PTL_DEBUG_NI4		__bit(28)
#define PTL_DEBUG_NI5		__bit(29)
#define PTL_DEBUG_NI6		__bit(30)
#define PTL_DEBUG_NI7		__bit(31)

#endif
