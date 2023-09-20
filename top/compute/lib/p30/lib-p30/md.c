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
** $Id: md.c,v 1.44 2001/12/21 00:12:04 rbbrigh Exp $
*/
/*
 * lib-p30/md.c
 *
 * Memory Descriptor management routines
 *
 */

#include <stdio.h>
#include <lib-p30.h>
#include <p30/arg-blocks.h>

/*
 * We do not invalidate the memory referenced by this MD here
 * because it may still be in use by the library for an incoming
 * message.  It is the responsibility of the caller to lib_md_decrement()
 * or lib_md_unlink() to invalidate the memory on its own.
 */
void lib_md_unlink( nal_cb_t *nal, lib_md_t *md )
{
	lib_ni_t	*ni	= &nal->ni;

	md->me          = NULL;

	if( ni->debug & PTL_DEBUG_UNLINK )
		nal->cb_printf( nal, "%d: Unlinking md %ld\n",
			ni->rid,
			(long)( md - ni->md )
		);

        nal->cb_invalidate( nal, md->start, md->length, md->addrkey);

	lib_unlink( nal, md );
	lib_md_free( nal, md );
}

static int lib_md_build( nal_cb_t *nal, ptl_md_t *md, lib_md_t **new_out )
{
	lib_eq_t	*eq = NULL;
	lib_md_t	*new = NULL;
        void            *addrkey;

	/*
	 * Translate the event queue handle into a event
	 * queue pointer if requested.  Do this before
	 * allocating the MD so that it does not have to
	 * report an error after allocating and do the cleanup.
	 */
	if( md->eventq != PTL_EQ_NONE ) {
		int eq_index	= md->eventq & 0xFFFF;
		eq	= &nal->ni.eq[eq_index];

		if( eq_index < 0 || eq_index > MAX_EQS || !inuse( eq ) )
			return PTL_INV_EQ;
	}

        /* attempt to validate the region at hand */
        if ( nal->cb_validate( nal, md->start, md->length, &addrkey ) < 0 ) {
          return PTL_VAL_FAILED;
        }

	new = lib_md_alloc( nal );
	if( !new ) {
          nal->cb_invalidate( nal, md->start, md->length, addrkey );
          return PTL_NOSPACE;
        }

	new->me         = NULL;
	new->start	= md->start;
	new->offset     = 0;
	new->length	= md->length;
	new->threshold	= md->threshold;
	new->max_offset = md->max_offset;
	new->pending    = 0;
	new->unlink     = PTL_UNLINK;
	new->options	= md->options;
	new->user_ptr	= md->user_ptr;
	new->eq		= eq;
	new->do_unlink  = 0;
	new->prev	= NULL;
	new->next	= NULL;

        new->addrkey    = addrkey;

	*new_out = new;

	return PTL_OK;
}

void lib_md_deconstruct( nal_cb_t *nal, lib_md_t *md, ptl_md_t *new )
{
	new->start	= md->start;
	new->length	= md->length;
	new->threshold	= md->threshold;
	new->max_offset = md->max_offset;
	new->options	= md->options;
	new->user_ptr	= md->user_ptr;
	new->eventq	= md->eq ? md->eq - nal->ni.eq : PTL_EQ_NONE;
}
	



int do_PtlMDAttach_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_me_t current_in
	 *	ptl_md_t md_in
	 *	ptl_unlink_t unlink_in
	 *
	 * Outgoing:
	 *	ptl_handle_md_t		* handle_out
	 */
	
	PtlMDAttach_internal_in		*args	= v_args;
	PtlMDAttach_internal_out	*ret	= v_ret;

	lib_ni_t	*ni	= &nal->ni;
	ptl_handle_ni_t	ni_h	= args->current_in & ~0xFFFF;
	int		current	= args->current_in & 0xFFFF;
	lib_me_t	*me	= &ni->me[current];
	lib_md_t	*new	= NULL;

	if( current < 0 || current > MAX_MES || !inuse( me ) ) {
		ret->rc = PTL_INV_ME;
		return ret->rc;
	}

	if( me->md ) {
		ret->rc = PTL_INUSE;
                return ret->rc; 
	}

	if( (ret->rc = lib_md_build( nal, &args->md_in, &new )) ) {
		return ret->rc;
	}

	me->md		= new;
	new->prev	= &(me->md);
	new->unlink	= args->unlink_in;

	new->me         = me;

	ret->handle_out	= (new - ni->md) | ni_h;
	return ret->rc = PTL_OK;
}


int do_PtlMDInsert_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t current_in
	 *	ptl_md_t md_in
	 *	ptl_unlink_t unlink_in
	 *	ptl_ins_pos_t position_in
	 *
	 * Outgoing:
	 *	ptl_handle_md_t		* handle_out
	 */
	
	PtlMDInsert_internal_in	*args	= v_args;
	PtlMDInsert_internal_out	*ret	= v_ret;

	lib_ni_t	*ni	= &nal->ni;
	ptl_handle_ni_t	ni_h	= args->current_in & ~0xFFFF;
	int		current	= args->current_in & 0xFFFF;
	lib_md_t	*md	= &ni->md[current];
	lib_md_t	*new	= NULL;

	if( current < 0 || current > MAX_MDS || !inuse( md ) ) {
		ret->rc = PTL_INV_MD;
		return ret->rc;
	}

	if( (ret->rc = lib_md_build( nal, &args->md_in, &new )) ) {
                return ret->rc;
	}

	lib_insert( md, new, args->position_in );

	new->me         = md->me;

	new->unlink	= args->unlink_in;
	ret->handle_out	= (new - ni->md) | ni_h;
	return ret->rc	= PTL_OK;
}


int do_PtlMDBind_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t ni_in
	 *	ptl_md_t md_in
	 *
	 * Outgoing:
	 *	ptl_handle_md_t		* handle_out
	 */
	
	PtlMDBind_internal_in	*args	= v_args;
	PtlMDBind_internal_out	*ret	= v_ret;

	lib_ni_t	*ni	= &nal->ni;
	ptl_handle_ni_t	ni_h	= args->ni_in & ~0xFFFF;
	lib_md_t	*new	= NULL;

	if( (ret->rc = lib_md_build( nal, &args->md_in, &new ) ) ) {
		return ret->rc;
	}

	new->me         = NULL;
	ret->handle_out	= (new - ni->md) | ni_h;
	return ret->rc	= PTL_OK;
}


int do_PtlMDUnlink_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t md_in
	 *
	 * Outgoing:
	 */

	PtlMDUnlink_internal_in	*args	= v_args;
	PtlMDUnlink_internal_out	*ret	= v_ret;

	int		current	= args->md_in & 0xFFFF;
	lib_md_t	*md	= &nal->ni.md[current];

	if( current < 0 || current > MAX_MDS || !inuse( md ) ) {
		return ret->rc = PTL_INV_MD;
        }

	lib_md_deconstruct( nal, md, &ret->status_out );
	lib_md_unlink( nal, md );

	return ret->rc = PTL_OK;
}


int do_PtlMDUpdate_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t md_in
	 *	ptl_md_t		* old_inout
	 *	ptl_md_t		* new_inout
	 *	ptl_handle_eq_t testq_in
	 *	ptl_seq_t		sequence_in
	 *
	 * Outgoing:
	 *	ptl_md_t		* old_inout
	 *	ptl_md_t		* new_inout
	 */
	
	PtlMDUpdate_internal_in	*args	= v_args;
	PtlMDUpdate_internal_out	*ret	= v_ret;

	lib_ni_t	*ni	= &nal->ni;
	int		current	= args->md_in & 0xFFFF;
	lib_md_t	*md	= &ni->md[current];
	lib_eq_t	*eq	= NULL;
	ptl_md_t	*new	= &args->new_inout;

        void            *addrkey;

	if( current < 0 || current > MAX_MDS || !inuse( md ) )
		return ret->rc = PTL_INV_MD;

	/*
	 * This is a very complex function with lots of strange things
	 * going on.  To sumarize:
	 *
	 * If old is valid, then the current state of the incoming
	 * memory descriptor is copied into it.
	 */

	if( args->old_inout_valid )
		lib_md_deconstruct( nal, md, &ret->old_inout );

	if( !args->new_inout_valid )   {
		return ret->rc = PTL_OK;
	}

	if( args->testq_in != PTL_EQ_NONE ) {
		int eq_index	= args->testq_in & 0xFFFF;
		eq		= &ni->eq[ eq_index ];

		if( eq_index < 0 || eq_index > MAX_EQS || !inuse( md ) )   {
			return ret->rc = PTL_INV_EQ;
		}
	}

	if( !eq || ( !eq->pending && eq->sequence == args->sequence_in ) ) {

          if ( (new->start != md->start) || (new->length > md->length) ) {

	    nal->cb_invalidate( nal, md->start, md->length, md->addrkey );

            if ( nal->cb_validate( nal, new->start, new->length, &addrkey) 
                                                                    < 0 ) {
              return ret->rc = PTL_VAL_FAILED;
            }
            md->addrkey        = addrkey;
          }

          md->start       	= new->start;
	  md->length	        = new->length;
	  md->threshold	        = new->threshold;
	  md->max_offset        = new->max_offset;
	  md->options	        = new->options;
	  md->user_ptr	        = new->user_ptr;
	  md->eq                = NULL;


	  if ((new->eventq >= 0) && (new->eventq <= MAX_EQS) ){
	    eq = &ni->eq[ new->eventq ];

            if (inuse(eq)) {
		 md->eq  = eq;
            }
	  }

	  return ret->rc = PTL_OK;
	} else {
	  return ret->rc = PTL_NOUPDATE;
	}
}


#if 0
int do_PtlMDDump( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t current_in

	 *
	 * Outgoing:

	 */
	
	PtlMDDump_in	*args	= v_args;
	PtlMDDump_out	*ret	= v_ret;
	int		current	= args->current_in & 0xFFFF;
	lib_md_t	*md	= &nal->md[ current ];

	if( current < 0 || current > MAX_MDS || !md->inuse )
		return ret->rc = PTL_INV_MD;

	nal->printf( nal, "MD %p (%d)\n", md, current );
	nal->printf( nal, "\textent\t= %p .. %p (%d)\n",
		md->start,
		(char*)md->start + md->length,
		md->length
	);

	nal->printf( nal, "\tthresh\t= %d\n",	md->threshold );
	nal->printf( nal, "\tmax_off\t= %d\n",	md->max_offset );
	nal->printf( nal, "\toptions\t= %d\n",	md->options );
	nal->printf( nal,  "\tuser\t= %p\n",	md->user_ptr );
	nal->printf( nal,  "\teq\t= %p\n",		md->eq );
	nal->printf( nal,  "\tprev\t= %p\n",	md->prev );
	nal->printf( nal,  "\tnext\t= %p\n",	md->next );

	return ret->rc = PTL_OK;
}
#endif
