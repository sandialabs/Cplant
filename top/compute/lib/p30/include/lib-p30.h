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
** $Id: lib-p30.h,v 1.35.6.1 2002/08/19 14:26:40 galagun Exp $
*/
#ifndef _LIB_P30_H_
#define _LIB_P30_H_

/*
 * lib-p30.h
 *
 * Top level include for library side routines
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include <p30.h>
#include <p30/types.h>
#include <p30/errno.h>
#include <p30/lib-types.h>
#include <p30/lib-nal.h>
#include <p30/lib-dispatch.h>

#ifndef __GNUC__
#	define inline
#endif

#define MAX_MES		4096
#define MAX_MDS		4096
#define MAX_MSGS	1024		/* Outstanding messages */

/* MAX_EQS is defined in <p30/types.h> since it is shared with the user */

/*
 * All library objects have an pointer to the next item in the free list
 * as their first component if the object is not inuse; it is NULL if in use.
 * We can take advantage of this to search through the arrays of them to find
 * one that is not inuse and return it in a generic fashion.  0xDEADBEEF
 * marks the end of the list, not NULL.
 */
#define mk_alloc( type )						\
	static inline lib_##type##_t *_lib_##type##_alloc(		\
		nal_cb_t *nal						\
	) {								\
		lib_##type##_t *obj = nal->ni.free_##type;		\
		if( !obj || obj == (lib_##type##_t*) 0xDEADBEEF )	\
			return 0;					\
		nal->ni.free_##type = nal->ni.free_##type##->next_free;	\
		obj->next_free = 0;					\
		return obj;						\
	}								\
									\
	static inline void _lib_##type##_free(				\
		nal_cb_t	*nal,					\
		lib_##type##_t	*obj					\
	) {								\
		if( obj->next_free ) {					\
			nal->cb_printf( nal, "%d: %s() Freeing already "\
			"free %p\n", nal->ni.rid, "_lib_" #type "_free",\
			obj);						\
		} else {						\
			obj->next_free = nal->ni.free_##type;		\
			nal->ni.free_##type = obj;			\
		}							\
	}

mk_alloc( me )
mk_alloc( md )
mk_alloc( eq )
mk_alloc( msg )

static inline lib_me_t *lib_me_alloc( nal_cb_t *nal ) {
	return _lib_me_alloc( nal );
}

static inline lib_md_t *lib_md_alloc( nal_cb_t *nal ) {
	return _lib_md_alloc( nal );
}

static inline lib_eq_t *lib_eq_alloc( nal_cb_t *nal ) {
	return _lib_eq_alloc( nal );
}


static inline void lib_me_free( nal_cb_t *nal, lib_me_t *obj ) {
	_lib_me_free( nal, obj );
}

static inline void lib_md_free( nal_cb_t *nal, lib_md_t *obj ) {
	_lib_md_free( nal, obj );
}

static inline void lib_eq_free( nal_cb_t *nal, lib_eq_t *obj ) {
	_lib_eq_free( nal, obj );
}

static inline lib_msg_t *lib_msg_alloc( nal_cb_t *nal ) {
	lib_msg_t	*msg	= _lib_msg_alloc( nal );
	lib_counters_t	*count	= &nal->ni.counters;
	if( msg ) {
                msg->md         = NULL;
		msg->return_md	= -1;
		if( ++count->msgs_alloc > count->msgs_max )
			count->msgs_max = count->msgs_alloc;
	}
	return msg;
}

static inline void lib_msg_free( nal_cb_t *nal, lib_msg_t *obj )
{
	nal->ni.counters.msgs_alloc--;
	_lib_msg_free( nal, obj );
}


#define lib_insert( obj, new, pos )					\
	do {								\
		if( pos == PTL_INS_AFTER ) {				\
			(new)->next	= (obj)->next;			\
			(new)->prev	= &((obj)->next);		\
			(obj)->next	= new;				\
			if( (new)->next )				\
				(new)->next->prev = &((new)->next);	\
		} else {						\
			(new)->next	= obj;				\
			(new)->prev	= (obj)->prev;			\
			(obj)->prev	= &((new)->next);		\
			if( (new)->prev )				\
				*((new)->prev) = new;			\
		}							\
	} while( 0 )

#define lib_unlink( nal, obj )						\
	do {								\
		if( (obj)->prev )					\
			*((obj)->prev)		= (obj)->next;		\
		if( (obj)->next )					\
			(obj)->next->prev	= (obj)->prev;		\
		(obj)->prev	= 0;					\
		(obj)->next	= 0;					\
	} while(0)


#define inuse( ptl_obj)		\
	( (ptl_obj)->next_free ? 0 : 1 )


extern int lib_init(
	nal_cb_t	*cb,
	int		nid,
	int		pid,
	int		rid,
	int		gid,
	int		gsize,
	ptl_pt_index_t	tbl_size,
	ptl_ac_index_t	ac_size
);

extern int lib_fini(
	nal_cb_t	*cb
);

extern void lib_dispatch(
	nal_cb_t	*cb,
	void		*private_,
	int		index,
	void		*arg_block,
	void		*ret_block
);

extern char *dispatch_name(
	int		index
);

/*
 * When the NAL detects an incoming message, it should call
 * lib_parse() decode it.  The NAL callbacks will be handed
 * the private cookie as a way for the NAL to maintain state
 * about which transaction is being processed.  An extra parameter,
 * lib_cookie will contain the necessary information for
 * finalizing the message.
 *
 * After it has finished the handling the message, it should
 * call lib_finalize() with the lib_cookie parameter.
 * Call backs will be made to write events, send acks or
 * replies and so on.
 */
extern int lib_parse( nal_cb_t *nal, ptl_hdr_t *hdr, void *private_ );
extern int lib_finalize( nal_cb_t *nal, void *private_, lib_msg_t *msg );

extern void print_hdr( nal_cb_t *nal, ptl_hdr_t *hdr );


/*
 * When the library operates on an MD it should decrement the
 * threshold of the MD via lib_md_decrement().  A zero return
 * indicates that the memory is still valid.
 */
extern int lib_md_decrement( nal_cb_t *nal, lib_md_t *md_in );
extern void lib_md_deconstruct( nal_cb_t *nal, lib_md_t *md_in, ptl_md_t *md_out );
extern void lib_md_unlink( nal_cb_t *nal, lib_md_t *md_in );
extern void lib_me_unlink( nal_cb_t *nal, lib_me_t *me_in );
extern void lib_eq_unlink( nal_cb_t *nal, lib_eq_t *eq_in );
extern int lib_trans_id( nal_cb_t *nal, ptl_process_id_t id_in,
		ptl_process_id_t *id_out );

#ifndef NULL
	#define NULL	((void*)0)
#endif

#endif
