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
** $Id: lib-types.h,v 1.31.4.2 2002/08/19 14:42:02 galagun Exp $
*/
#ifndef _LIB_TYPES_H_
#define _LIB_TYPES_H_


/*
 * p30/lib-types.h
 * 
 * Types used by the library side routines that do not need to be
 * exposed to the user application
 */

#include <p30/types.h>
#if defined (__KERNEL__)
#	include <linux/types.h>
#else
#	include <sys/types.h>
#endif

typedef char*	user_ptr;
typedef struct lib_msg_t	lib_msg_t;
typedef struct lib_ptl_t	lib_ptl_t;
typedef struct lib_ac_t		lib_ac_t;
typedef struct lib_me_t		lib_me_t;
typedef struct lib_md_t		lib_md_t;
typedef struct lib_eq_t		lib_eq_t;

typedef enum {
	PTL_MSG_ACK = 0,
	PTL_MSG_PUT,
	PTL_MSG_GET,
	PTL_MSG_REPLY,
	PTL_MSG_BARRIER
} ptl_msg_type_t;

typedef struct {
	ptl_msg_type_t		type;
	int			nid;
	int			pid;
        ptl_process_id_t	src;
	union {
		struct {
			ptl_handle_md_t		dst_md;
			ptl_size_t		mlength;
			ptl_match_bits_t	match_bits;
		} ack;

		struct {
			int			ptl_index;
			ptl_handle_md_t		ack_md;
			ptl_match_bits_t	match_bits;
			ptl_size_t		length,
						offset;
			ptl_hdr_data_t          hdr_data;
		} put;

		struct {
			int			ptl_index;
			ptl_match_bits_t	match_bits;
			ptl_size_t		length,
						src_offset,
						return_offset;
			ptl_handle_md_t		return_md;
		} get;

		struct {
			ptl_handle_md_t		dst_md;
			ptl_size_t		dst_offset,
						length;
		} reply;
			
		struct {
			int			sequence;
		} barrier;
	} msg;
} ptl_hdr_t;


typedef struct {
	long	recv_count,
		recv_length,
		send_count,
		send_length,
		drop_count,
		drop_length,
		msgs_alloc,
		msgs_max;
} lib_counters_t;


struct lib_msg_t {
	lib_msg_t	*next_free;
        lib_md_t        *md;
	int             gid;
	int		rid;
	int             nid;
	int		pid;
	ptl_event_t	ev;
	ptl_handle_md_t	return_md;
};



struct lib_ptl_t {
	ptl_pt_index_t		size;
	lib_me_t		**tbl;
};

struct lib_ac_t {
	int			next_free;
};

struct lib_eq_t {
	lib_eq_t		*next_free;
	ptl_seq_t		sequence;
	ptl_size_t		size;
	ptl_event_t		*base;
	int			pending;
};

struct lib_me_t {
	lib_me_t		*next_free;
	ptl_process_id_t	match_id;
	ptl_match_bits_t	match_bits,
				ignore_bits;
	ptl_unlink_t		unlink;
	lib_md_t		*md;		
	lib_me_t		*next;
	lib_me_t		**prev;
};

struct lib_md_t {
	lib_md_t		*next_free;  
        lib_me_t                *me;  
	user_ptr		start;
	int			offset;
	ptl_size_t		length;
	int			threshold; 
	int                     max_offset;
        int                     pending;
	ptl_unlink_t		unlink;
	unsigned int		options;
	void			*user_ptr;
	lib_eq_t		*eq;
	int                      do_unlink;
	lib_md_t		*next;
	lib_md_t		**prev;
        void                    *addrkey;
};

typedef struct {
	int                     up;
	int			nid,
				pid,
				rid,
				gid,
				num_nodes;
	unsigned int		debug;
	lib_ptl_t		tbl;
	lib_ac_t		ac;
	lib_eq_t		*eq, *free_eq;
	lib_me_t		*me, *free_me;
	lib_md_t		*md, *free_md;
	lib_msg_t		*msg, *free_msg;
	lib_counters_t		counters;
} lib_ni_t;



#endif
