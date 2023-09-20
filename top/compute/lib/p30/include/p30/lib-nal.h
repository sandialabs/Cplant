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
** $Id: lib-nal.h,v 1.19.4.1 2002/08/19 14:26:42 galagun Exp $
*/
#ifndef _LIB_NAL_H_
#define _LIB_NAL_H_

/*
 * nal.h
 *
 * Library side headers that define the abstraction layer's
 * responsibilities and interfaces
 */

#include <p30/lib-types.h>


typedef struct nal_cb_t nal_cb_t;

struct nal_cb_t {
	/*
	 * Per interface portal table, access control table
	 * and NAL private data field;
	 */
	lib_ni_t	ni;
	void		*nal_data;

	/*
	 * send:  Sends a preformated header and user data to a
	 * specified remote process.
	 */
	int	(*cb_send)(
			nal_cb_t		*nal,
			void			*private_,
			lib_msg_t		*cookie,
			ptl_hdr_t		*hdr,
			int			nid,
			int			pid,
			int			gid,
			int			rid,
			user_ptr		data,
			size_t			len
	);

	/*
	 * recv: Receives an incoming message from a remote process
	 */
	int	(*cb_recv)(
			nal_cb_t		*nal,
			void			*private_,
			lib_msg_t		*cookie,
			user_ptr		dst_addr,
			size_t			mlen,
			size_t			rlen
	);

	/*
	 * write: Writes a block of data into a specified user address
	 */
	int	(*cb_write)(
			nal_cb_t		*nal,
			void			*private_,
			user_ptr		dsr_addr,
			void			*src_addr,
			size_t			len
	);

	/*
	 *  malloc: Acquire a block of memory in a system independent
	 * fashion.
	 */
	void	*(*cb_malloc)(
			nal_cb_t		*nal,
			size_t			len
	);

	void	(*cb_free)(
			nal_cb_t		*nal,
			void			*buf
	);

	/*
	 * invalidate: mark a piece of user memory as no longer 
	 * in use by the system.
	 */
	void	(*cb_invalidate)(
			nal_cb_t		*nal,
			void			*base,
			size_t			extent,
                        void                    *addrkey
	);

        int     (*cb_validate)(
			nal_cb_t		*nal,
			void			*base,
			size_t			extent,
                        void                    **addrkey
        );

	void	(*cb_printf)(
			nal_cb_t		*nal,
			const char		*fmt,
			...
	);

	/* Turn interrupts off (begin of protected area) */
	void	(*cb_cli)(
			nal_cb_t		*nal,
			unsigned long		*flags
	);

	/* Turn interrupts on (end of protected area) */
	void	(*cb_sti)(
			nal_cb_t		*nal,
			unsigned long		*flags
	);

	/*
	 * Translate a gid/rid into a nid/pid
	 */
	int	(*cb_gidrid2nidpid)(
			nal_cb_t		*nal,
			ptl_id_t		gid,
			ptl_id_t		rid,
			ptl_id_t		*nid,
			ptl_id_t		*pid
	);


	/*
	 * Translate a nid/pid into a gid/rid
	 */
	int	(*cb_nidpid2gidrid)(
			nal_cb_t		*nal,
			ptl_id_t		nid,
			ptl_id_t		pid,
			ptl_id_t		*gid,
			ptl_id_t		*rid
	);

	/*
	 * Calculate a network "distance" to given node
	 */
	int    (*cb_dist)(
                  	nal_cb_t                *nal,
			ptl_id_t                 nid,
			unsigned long          *dist
	);

};
			
#endif
