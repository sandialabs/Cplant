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
** $Id: api.h,v 1.15 2002/01/31 19:36:03 rbbrigh Exp $
*/
#ifndef P30_API_H
#define P30_API_H

#include <p30/types.h>

#ifndef PTL_NO_WRAP
int	PtlInit( void );
void	PtlFini( void );

int PtlNIInit(
	ptl_interface_t		interface,
	ptl_pt_index_t		ptl_size_in,
	ptl_ac_index_t		acl_size_in,
	ptl_handle_ni_t		*interface_out
);

int PtlNIFini(
	ptl_handle_ni_t		interface_in
);

#endif

int PtlGetId(
	ptl_handle_ni_t		handle_in,
	ptl_process_id_t	*id_out,
	ptl_id_t		*gsize_out
);

int PtlTransId(
	ptl_handle_ni_t		handle_in,
	ptl_process_id_t	id_in,
	ptl_process_id_t	*id_out
);





/*
 * Network interfaces
 */

#ifndef PTL_NO_WRAP
int PtlNIBarrier(
	ptl_handle_ni_t		interface_in
);
#endif

int PtlNIStatus(
	ptl_handle_ni_t		interface_in,
	ptl_sr_index_t		register_in,
	ptl_sr_value_t		*status_out
);

int PtlNIDist(
	ptl_handle_ni_t		interface_in,
	ptl_process_id_t	process_in,
	unsigned long		*distance_out
);

#ifndef PTL_NO_WRAP
int PtlNIHandle(
	ptl_handle_any_t	handle_in,
	ptl_handle_ni_t		*interface_out
);
#endif


/*
 * PtlNIDebug: 
 *
 * This is not an official Portals 3 API call.  It is provided
 * by the reference implementation to allow the maintainers an
 * easy way to turn on and off debugging information in the
 * library.  Do not use it in code that is not intended for use
 * with any version other than the portable reference library.
 */
unsigned int PtlNIDebug(
	ptl_handle_ni_t		ni,
	unsigned int		mask_in
);


/*
 * Match entries
 */

int PtlMEAttach(
	ptl_handle_ni_t		interface_in,
	ptl_pt_index_t		index_in,
	ptl_process_id_t	match_id_in,
	ptl_match_bits_t	match_bits_in,
	ptl_match_bits_t	ignore_bits_in,
	ptl_unlink_t		unlink_in,
	ptl_handle_me_t		*handle_out
);

int PtlMEInsert(
	ptl_handle_me_t		current_in,
	ptl_process_id_t	match_id_in,
	ptl_match_bits_t	match_bits_in,
	ptl_match_bits_t	ignore_bits_in,
	ptl_unlink_t		unlink_in,
	ptl_ins_pos_t		position_in,
	ptl_handle_me_t		*handle_out
);

int PtlMEUnlink(
	ptl_handle_me_t		current_in
);

int PtlTblDump( ptl_handle_ni_t ni, int index_in );
int PtlMEDump( ptl_handle_me_t current_in );



/*
 * Memory descriptors
 */

#ifndef PTL_NO_WRAP
int PtlMDAttach(
	ptl_handle_me_t		current_in,
	ptl_md_t		md_in,
	ptl_unlink_t		unlink_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDInsert(
	ptl_handle_md_t		current_in,
	ptl_md_t		md_in,
	ptl_unlink_t		unlink_in,
	ptl_ins_pos_t		position_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDBind(
	ptl_handle_ni_t		ni_in,
	ptl_md_t		md_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDUnlink(
	ptl_handle_md_t		md_in
);

int PtlMDUpdate(
	ptl_handle_md_t		md_in,
	ptl_md_t		*old_inout,
	ptl_md_t		*new_inout,
	ptl_handle_eq_t		testq_in
);

#endif

/* These should not be called by users */
int PtlMDAttach_internal(
	ptl_handle_me_t		current_in,
	ptl_md_t		md_in,
	ptl_unlink_t		unlink_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDInsert_internal(
	ptl_handle_md_t		current_in,
	ptl_md_t		md_in,
	ptl_unlink_t		unlink_in,
	ptl_ins_pos_t		position_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDBind_internal(
	ptl_handle_ni_t		ni_in,
	ptl_md_t		md_in,
	ptl_handle_md_t		*handle_out
);

int PtlMDUpdate_internal(
	ptl_handle_md_t		md_in,
	ptl_md_t		*old_inout,
	ptl_md_t		*new_inout,
	ptl_handle_eq_t		testq_in,
	ptl_seq_t		sequence_in
);

int PtlMDUnlink_internal(
	ptl_handle_md_t		md_in,
	ptl_md_t		*status_out
);



/*
 * Event queues
 */
#ifndef PTL_NO_WRAP

/* These should be called by users */
int PtlEQAlloc(
	ptl_handle_ni_t		ni_in,
	ptl_size_t		count_in,
	ptl_handle_eq_t		*handle_out
);
int PtlEQFree(
	ptl_handle_eq_t		eventq_in
);

int PtlEQCount(
	ptl_handle_eq_t		eventq_in,
	ptl_size_t		*count_out
);

int PtlEQGet(
	ptl_handle_eq_t		eventq_in,
	ptl_event_t		*event_out
);


int PtlEQWait(
	ptl_handle_eq_t		eventq_in,
	ptl_event_t		*event_out
);

int PtlEQWait_timeout(
	ptl_handle_eq_t		eventq_in,
	ptl_event_t		*event_out,
	int			timeout
);
#endif

/* This should not be called by users */
int PtlEQAlloc_internal(
	ptl_handle_ni_t		ni_in,
	ptl_size_t		count_in,
	void			*base_in,
	int			len_in,
	ptl_handle_eq_t		*handle_out
);

int PtlEQFree_internal(
	ptl_handle_eq_t		eventq_in
);


/*
 * Access Control Table
 */
int PtlACEntry(
	ptl_handle_ni_t		ni_in,
	ptl_ac_index_t		index_in,
	ptl_process_id_t	match_id_in,
	ptl_pt_index_t		portal_in
);


/*
 * Data movement
 */

int PtlPut(
	ptl_handle_md_t		md_in,
	ptl_ack_req_t		ack_req_in,
	ptl_process_id_t	target_in,
	ptl_pt_index_t		portal_in,
	ptl_ac_index_t		cookie_in,
	ptl_match_bits_t	match_bits_in,
	ptl_size_t		offset_in,
	ptl_hdr_data_t          hdr_data_in
);

int PtlGet(
	ptl_handle_md_t		md_in,
	ptl_process_id_t	target_in,
	ptl_pt_index_t		portal_in,
	ptl_ac_index_t		cookie_in,
	ptl_match_bits_t	match_bits_in,
	ptl_size_t		offset_in
);



#endif
