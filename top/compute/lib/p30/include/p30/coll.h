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
** $Id: coll.h,v 1.7 2000/06/26 19:54:02 rolf Exp $
*/
#ifndef _P30_COLL_H_
#define _P30_COLL_H_

/*
 * <p30/coll.h>
 *
 * These are collective routines that are used to implement PtlNIBarrier.
 * They are generic in the sense that anyone can use generic_fanin or
 * generic_fanout to implement their own global reduction / broadcast
 * and gather / scatter operations.
 */
extern int coll_debug;
extern int coll_did_init;
extern int ptl_coll_init( ptl_handle_ni_t ni );
extern int ptl_coll_fini( ptl_handle_ni_t ni );

extern int PtlBroadcast_all( ptl_handle_ni_t ni, int root, int *value_inout );
extern int PtlReduce_all( ptl_handle_ni_t ni, int root, int *value_inout );

extern int generic_fanin(
	void *private,
	int myrank, int rootrank, int ngroup,
	int (*as_child)( void *private, int myrank, int partner ),
	int (*as_parent)( void *private, int myrank, int partner ) );

extern int generic_fanout(
	void *private,
	int myrank, int rootrank, int ngroup,
	int (*as_child)( void *private, int myrank, int partner ),
	int (*as_parent)( void *private, int myrank, int partner ) );


#ifdef P30_COLL_INTERNAL

#	define COLLECTIVE_PORTAL	0
#	define COLLECTIVE_BCAST_BITS	0x777
#	define COLLECTIVE_REDUCE_BITS	0x555

	extern volatile int bcast_buf, reduce_buf;
	extern ptl_handle_md_t bcast_md, reduce_md;
	extern ptl_handle_eq_t bcast_eq, reduce_eq;
#endif

#endif
