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
** $Id: types.h,v 1.17 2001/12/21 00:08:15 rbbrigh Exp $
*/
#ifndef _P30_TYPES_H_
#define _P30_TYPES_H_

typedef int ptl_id_t;
typedef int ptl_pt_index_t;
typedef int ptl_ac_index_t;

/* How to do this on the i386 boxen? */
#ifdef __i386__
typedef unsigned long long int ptl_match_bits_t;
#else
typedef unsigned long ptl_match_bits_t;
#endif
/* does this have to be a 64-bit quantity? */
typedef unsigned long ptl_hdr_data_t;

typedef int ptl_size_t;

typedef int ptl_handle_any_t;
typedef ptl_handle_any_t ptl_handle_ni_t;
typedef ptl_handle_any_t ptl_handle_eq_t;
typedef ptl_handle_any_t ptl_handle_md_t;
typedef ptl_handle_any_t ptl_handle_me_t;

#define PTL_EQ_NONE	((ptl_handle_eq_t) -1)
#define PTL_ID_ANY	((ptl_id_t) -1)

typedef enum {
	PTL_ADDR_NID,
	PTL_ADDR_GID,
	PTL_ADDR_BOTH
} ptl_addr_kind_t;

typedef struct {
	ptl_addr_kind_t	addr_kind;	/* kind of address pair */
	ptl_id_t	nid, pid;	/* node id / process id */
	ptl_id_t	gid, rid;	/* groupd id / rank id */
} ptl_process_id_t;


typedef enum {
	PTL_RETAIN = 0,
	PTL_UNLINK
} ptl_unlink_t;

typedef enum {
	PTL_INS_BEFORE,
	PTL_INS_AFTER
} ptl_ins_pos_t;

typedef struct {
	void			*start;
	ptl_size_t		length;
	int			threshold;
	int                     max_offset; /* Portals 3.1 */
	unsigned int		options;
	void			*user_ptr;
	ptl_handle_eq_t		eventq;
} ptl_md_t;

/* Options for the MD structure */
#define PTL_MD_OP_PUT		00001
#define PTL_MD_OP_GET		00002
#define PTL_MD_MANAGE_REMOTE	00004
#define PTL_MD_TRUNCATE		00010
#define PTL_MD_ACK_DISABLE	00020

#define PTL_MD_THRESH_INF	(-1)


typedef enum {
	PTL_EVENT_GET,
	PTL_EVENT_PUT,
	PTL_EVENT_REPLY,
	PTL_EVENT_ACK,
	PTL_EVENT_SENT
} ptl_event_kind_t;

typedef unsigned int ptl_seq_t;

typedef struct {
	ptl_event_kind_t	type;
	volatile ptl_seq_t		sequence;
	ptl_process_id_t	initiator;
	ptl_pt_index_t		portal;
	ptl_match_bits_t	match_bits;
	ptl_size_t		rlength,
				mlength,
				offset;
	ptl_md_t		md;
	ptl_hdr_data_t          hdr_data;
} ptl_event_t;


typedef enum {
	PTL_ACK_REQ,
	PTL_NOACK_REQ
} ptl_ack_req_t;


#define MAX_EQS			4096
typedef struct {
	int			inuse;
	volatile ptl_seq_t	sequence;
	ptl_size_t		size;
	ptl_event_t		*base;
} ptl_eq_t;

typedef struct {
	ptl_eq_t		*eq;
} ptl_ni_t;

/*
 * Status registers
 */
typedef enum {
	PTL_SR_DROP_COUNT,
	PTL_SR_DROP_LENGTH,
	PTL_SR_RECV_COUNT,
	PTL_SR_RECV_LENGTH,
	PTL_SR_SEND_COUNT,
	PTL_SR_SEND_LENGTH,
	PTL_SR_MSGS_MAX,
} ptl_sr_index_t;

typedef int ptl_sr_value_t;


#endif
