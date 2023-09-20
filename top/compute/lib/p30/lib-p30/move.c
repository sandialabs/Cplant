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
** $Id: move.c,v 1.58.2.1 2002/07/11 01:35:16 jjohnst Exp $
*/
/*
 * lib-p30/move.c
 *
 * Data movement routines
 */

#include <stdio.h>
#include <p30.h>
#include <lib-p30.h>
#include <p30/arg-blocks.h>
#define min(a,b)	( (a) < (b) ? (a) : (b) )

/*
 * Finding the correct MD is a little tricky.
 * I've tried to write this as clearly as possible, but it is
 * still a little hairy in a few places.
 *
 * This is abstracted out for both PUT and GET messages so that
 * we don't have to repeat the code in multiple places.
 * Right now it does not check access control lists nor does
 * it ensure that the MD is of the correct type.
 *
 * MD searching is as described in the specification.  Remove the
 * goto from the second for loop to get the more rational behaviour
 * of:
 * Each match
 * entry has a linked list of memory descriptors, as documented.  Upon
 * examining a ME, each MD in its list is searched in turn for one that
 * matches.  If none do, the next ME is searched until all ME's are
 * exhausted or a matching MD is found.
 *
 * The document states that only the first MD of each ME is searched, but
 * this does not make sense for an MD to be able to hit a threshold at
 * which it is inactive but not unlinked.  If the threshold also unlinked
 * the MD, then it would make searching the first MD reasonable since
 * the full MD would be cleared out. 
 *
 * Otherwise, each ME will most likely only have one MD under it and
 * there will be many ME's with identical criteria to simulate the
 * behaviour of searching all MD's.
 */
static lib_md_t *lib_find_md(
	nal_cb_t	*nal,
	ptl_id_t        src_gid,
	ptl_id_t        src_rid,
	ptl_id_t        src_nid,
	ptl_id_t        src_pid,
	int		index,
	ptl_msg_type_t  type,
	int		rlength,
	int		roffset,
	ptl_match_bits_t	match_bits,
	int		*length_out,
	int		*offset_out
)
{
	lib_ni_t	*ni	= &nal->ni;
	lib_ptl_t	*tbl	= &ni->tbl;
	ptl_process_id_t  mid;
	lib_me_t	*me;
	lib_md_t	*md;
	int		length	= 0,
			offset	= 0;

	if( index < 0 || index > tbl->size ) {
		nal->cb_printf( nal,
			"%d: request from %d for invalid portal index %d\n",
			ni->rid,
			src_rid,
			index
		);
		return NULL;
	}

	if( ni->debug & PTL_DEBUG_REQUEST )
		nal->cb_printf( nal,
			"%d: Request from %d of length %ld into portal %d "
			"MB=%016lx\n",
			ni->rid,
			src_rid,
			length,
			index,
			match_bits
		);

	for( me = tbl->tbl[index] ; me ; me = me->next ) {

		if( !inuse( me ) ) {
			nal->cb_printf( nal,
				"%d: portal %d: ME %ld not inuse\n",
				ni->rid,
				index, (long)( me - ni->me ) );
			continue;
		}

		mid = me->match_id;

		/*
		 * In addition to this being a bug, we should check access
		 * control list here, too.  The bug is that we do not check
		 * to see if the match_id is of type PTL_ID_GID.
		 */
		if( ni->debug & PTL_DEBUG_DELIVERY )
			nal->cb_printf( nal,
				"%d: Looking at me %ld (%p): gid/rid=%d/%d\n",
				ni->rid,
				(long) (me - ni->me), me,
				mid.gid,
				mid.rid
			);

		if ( ((mid.addr_kind == PTL_ADDR_GID) || (mid.addr_kind == PTL_ADDR_BOTH)) &&

		     ( ((src_gid != mid.gid) && (mid.gid != PTL_ID_ANY)) ||
		       ((src_rid != mid.rid) && (mid.rid != PTL_ID_ANY))   )  ){

		     continue;

		 }
		if ( ((mid.addr_kind == PTL_ADDR_NID) || (mid.addr_kind == PTL_ADDR_BOTH)) &&

		     ( ((src_nid != mid.nid) && (mid.nid != PTL_ID_ANY)) ||
		       ((src_pid != mid.pid) && (mid.pid != PTL_ID_ANY))   )  ){


		     continue;
		 }
		if ((me->match_bits & ~me->ignore_bits) == (match_bits & ~me->ignore_bits) ) {

		    break;
		}

		/* If we don't find a working MD, we come back here */
		next_me:
		continue;
	}

	if( !me ) {
		/* This should be debug enabled in some fashion */
		if( ni->debug & PTL_DEBUG_DROP )
			nal->cb_printf( nal,
				"%d: dropping message (no match) portal %d "
				"from rid=%d mb=%016x\n",
				ni->rid,
				index,
				src_rid,
				match_bits
			);
		return NULL;
	}

	for( md = me->md ; md ; md = md->next ) {

	        if( (type == PTL_MSG_PUT) && !(md->options & PTL_MD_OP_PUT) ){
		    goto next_me;
                }

		if( (type == PTL_MSG_GET) && !(md->options & PTL_MD_OP_GET) ){
		    goto next_me;
                }

		if( md->threshold == 0 ){
		    goto next_me;
                }

		if( !inuse( md ) ) {
			nal->cb_printf( nal,
				"%d: md %ld not inuse but in "
				"me %ld on portal %d\n",
				ni->nid,
				(long) (md - ni->md),
				(long)( me - ni->me),
				index
			);
			continue;
		}

		if( md->options & PTL_MD_MANAGE_REMOTE ) {
			offset	= roffset;
		}
		else {

                        if ( md->offset > md->max_offset ) {

                            goto next_me;
			}

			offset	= md->offset;
		}

		if( (md->options & PTL_MD_TRUNCATE) && offset <= md->length ) {
			length	= min( md->length - offset, rlength );
			break;
		}

		if( offset + rlength <= md->length ) {
			length = rlength;
			break;
		}

		if( ni->debug & PTL_DEBUG_DELIVERY )
			nal->cb_printf( nal, "%d: md failed to match? "
				"offset=%d (opts=%d %c)\n",
				ni->nid, offset,
				md->options,
				(md->options & PTL_MD_MANAGE_REMOTE ? 'R' : 'L' ) );
		/*
		 * According to the specification, we should not look
		 * at any more MD's in this list.  So instead we just
		 * go back to looking at the next match entry.
		 *
		 * Remove this goto to have rational searching...
		 */
		goto next_me;
	}

	if( !md )
		goto next_me;

	md->offset	+= length;

	*length_out	= length;
	*offset_out	= offset;

	return md;
}


/*
 * Incoming messages have a ptl_msg_t object associated with them
 * by the library.  This object encapsulates the state of the
 * message and allows the NAL to do non-blocking receives or sends
 * of long messages.
 *
 */


static int parse_put( nal_cb_t *nal, ptl_hdr_t *hdr, void *private )
{
	lib_md_t	*md;
	int		offset	= 0,
			length	= 0,
			rlength	= 0;
	lib_msg_t	*msg	= NULL;
	lib_ni_t	*ni	= &nal->ni;

	rlength	= hdr->msg.put.length;

	md	= lib_find_md(
			nal,
			hdr->src.gid,
			hdr->src.rid,
			hdr->src.nid,
			hdr->src.pid,
			hdr->msg.put.ptl_index,
			hdr->type,
			rlength,
			hdr->msg.put.offset,
			hdr->msg.put.match_bits,
			&length,
			&offset
		);



	if( !md ) {
#if 0
		if( ni->debug & PTL_DEBUG_DROP )
#endif
			nal->cb_printf( nal,
				"%d: Dropping put into portal %d "
				"from rid %d of length %ld\n",
				ni->rid,
				hdr->msg.put.ptl_index,
				hdr->src.rid,
				rlength
			);

		ni->counters.drop_count++;
		ni->counters.drop_length += rlength;
		nal->cb_recv( nal, private, NULL, NULL, 0, rlength );
		return -1;
	}

	if( ni->debug & PTL_DEBUG_PUT )
		nal->cb_printf( nal,
			"%d: Incoming put %d from %d/%d %d/%d "
			"of length %ld/%ld into md %ld (%p+%ld:%ld)\n",
			ni->rid,
			hdr->msg.put.ptl_index,
			hdr->src.nid, hdr->src.pid,
			hdr->src.gid, hdr->src.rid,
			length, rlength,
			(long)( md - ni->md ),
			md->start,
			offset,
			length
		);

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal, "parse_put: BAD - could not allocate cookie!\n");
          return 1;
        }

	if( (hdr->msg.put.ack_md >= 0) && !(md->options & PTL_MD_ACK_DISABLE) )
	{
		msg->nid		= hdr->src.nid;
		msg->pid		= hdr->src.pid;
		msg->gid		= hdr->src.gid;
		msg->rid		= hdr->src.rid;
		msg->return_md		= hdr->msg.put.ack_md;
		msg->ev.match_bits      = hdr->msg.put.match_bits;
	}

	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_PUT;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.nid   = hdr->src.nid;
		msg->ev.initiator.pid   = hdr->src.pid;
		msg->ev.initiator.gid   = hdr->src.gid;
		msg->ev.initiator.rid   = hdr->src.rid;
		msg->ev.portal		= hdr->msg.put.ptl_index;
		msg->ev.match_bits	= hdr->msg.put.match_bits;
		msg->ev.rlength		= rlength;
		msg->ev.mlength		= length;
		msg->ev.offset		= offset;
		msg->ev.hdr_data        = hdr->msg.put.hdr_data;

		lib_md_deconstruct( nal, md, &msg->ev.md );
	}

	ni->counters.recv_count++;
	ni->counters.recv_length += length;

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {

	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }

	    if ( !(md->options & PTL_MD_MANAGE_REMOTE) && (md->offset > md->max_offset) ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	nal->cb_recv( nal, private, msg, md->start + offset, 
                      length, rlength );
	return 0;
}



static int parse_get( nal_cb_t *nal, ptl_hdr_t *hdr, void *private )
{
	lib_md_t	*md;
	ptl_hdr_t	out;
	int		offset	= 0,
			length	= 0,
			rlength	= 0,
			rc	= 0;
	lib_msg_t	*msg	= NULL;
	lib_ni_t	*ni	= &nal->ni;


	rlength	= hdr->msg.get.length;

	/* Complete the incoming message, which should have zero bytes */
	nal->cb_recv( nal, private, NULL, NULL, 0, 0 );

	md	= lib_find_md(
			nal,
			hdr->src.gid,
			hdr->src.rid,
			hdr->src.nid,
			hdr->src.pid,
			hdr->msg.get.ptl_index,
			hdr->type,
			rlength,
			hdr->msg.get.src_offset,
			hdr->msg.get.match_bits,
			&length,
			&offset
		);

	if( !md ) {

#if 0
		if( ni->debug & PTL_DEBUG_DROP )
#endif
			nal->cb_printf( nal,
				"%d: Dropping get from portal %d "
				"from rid %d of length %ld\n",
				ni->rid,
				hdr->msg.get.ptl_index,
				hdr->src.rid,
				rlength
			);

		ni->counters.drop_count++;
		ni->counters.drop_length += rlength;
		rc= -1;
		return rc;
	}

	if( ni->debug & PTL_DEBUG_GET )
		nal->cb_printf( nal,
			"%d: Incoming get from portal %d from rid %d "
			"of length %ld/%ld from md %ld (%p+%ld:%ld)\n",
			ni->rid,
			hdr->msg.get.ptl_index,
			hdr->src.rid,
			length, rlength,
			(long)( md - ni->md ),
			md->start,
			offset,
			length
		);
	
	out.type		= PTL_MSG_REPLY;
	out.nid                 = hdr->src.nid;
	out.pid                 = hdr->src.pid;
	out.src.nid		= ni->nid;
	out.src.pid		= ni->pid;
	out.src.gid		= ni->gid;
	out.src.rid		= ni->rid;
	out.msg.reply.dst_md	= hdr->msg.get.return_md;
	out.msg.reply.length	= length;
	out.msg.reply.dst_offset	= hdr->msg.get.return_offset;

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal, "parse_get: BAD - could not allocate cookie!\n");
          return 1;
        }

	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_GET;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.nid   = hdr->src.nid;
		msg->ev.initiator.pid   = hdr->src.pid;
		msg->ev.initiator.gid   = hdr->src.gid;
		msg->ev.initiator.rid   = hdr->src.rid;
		msg->ev.portal		= hdr->msg.get.ptl_index;
		msg->ev.match_bits	= hdr->msg.get.match_bits;
		msg->ev.rlength		= rlength;
		msg->ev.mlength		= length;
		msg->ev.offset		= offset;
		msg->ev.hdr_data        = 0;

		msg->return_md		= -1;

		lib_md_deconstruct( nal, md, &msg->ev.md );
	}

	ni->counters.send_count++;
	ni->counters.send_length += length;

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {

	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }

	    if ( !(md->options & PTL_MD_MANAGE_REMOTE) && (md->offset > md->max_offset) ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	rc= nal->cb_send( nal, private, msg,
		&out,
                hdr->src.nid,
		hdr->src.pid,
		hdr->src.gid,
		hdr->src.rid,
		md->start + offset, length);


	return rc;
}


static int parse_reply( nal_cb_t *nal, ptl_hdr_t *hdr, void *private )
{
	ptl_handle_md_t		current = hdr->msg.reply.dst_md & 0xFFFF;
	lib_md_t		*md	= &nal->ni.md[ current ];
	int			rlength,
				length,
				offset;
	lib_msg_t		*msg	= NULL;
	lib_ni_t		*ni	= &nal->ni;

	if( current < 0 || current > MAX_MDS || !inuse( md ) ) {
		nal->cb_printf( nal, "%d: Reply from %d to invalid md %d\n",
			ni->rid,
			hdr->src.rid,
			current
		);
		goto fail_reply;
	}

 	if( !md->threshold ) {
		if( ni->debug & PTL_DEBUG_THRESHOLD )
			nal->cb_printf( nal,
				"%d: Reply into inactive MD %d from %d\n",
				ni->rid,
				current,
				hdr->src.rid
			);
		goto fail_reply;
	}


	rlength = hdr->msg.reply.length;

	if( md->options & PTL_MD_MANAGE_REMOTE )
		offset = hdr->msg.reply.dst_offset;
	else
		offset = md->offset;

	if( rlength + offset > md->length && !(md->options & PTL_MD_TRUNCATE) )
		goto fail_reply;

	length = min( rlength, md->length - offset );

	if( ni->debug & PTL_DEBUG_REPLY ) {
		nal->cb_printf( nal,
			"%d: Reply from %d of length %ld/%ld into md %ld\n",
			ni->rid,
			hdr->src.rid,
			length,
			rlength,
			current
		);
        }

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal,"parse_reply: BAD - could not allocate cookie!\n");
          return 1;
        }

	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_REPLY;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.nid   = hdr->src.nid;
		msg->ev.initiator.pid   = hdr->src.pid;
		msg->ev.initiator.gid   = hdr->src.gid;
		msg->ev.initiator.rid   = hdr->src.rid;
		msg->ev.rlength		= rlength;
		msg->ev.mlength		= length;
		msg->ev.offset		= offset;
		msg->return_md		= -1;

		lib_md_deconstruct( nal, md, &msg->ev.md );
	}
		
	ni->counters.recv_count++;
	ni->counters.recv_length += length;

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {
	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	nal->cb_recv( nal, private, msg, 
                      md->start + offset, length, rlength );
	return 0;

fail_reply:
	ni->counters.drop_count++;
	ni->counters.drop_length += hdr->msg.reply.length;

	nal->cb_recv( nal, private, NULL, NULL, 0, hdr->msg.reply.length );
	return -1;
}


static int parse_ack( nal_cb_t *nal, ptl_hdr_t *hdr, void *private )
{
	lib_ni_t	*ni	= &nal->ni;
	ptl_handle_md_t	current	= hdr->msg.ack.dst_md & 0xFFFF;
	lib_md_t	*md	= &ni->md[ current ];
	lib_msg_t	*msg	= NULL;

	if( current < 0 || current > MAX_MDS || !inuse( md ) ) {
		nal->cb_printf( nal, "%d: ACK from %d to invalid md %d\n",
			ni->nid,
			hdr->src.rid,
			current
		);
		goto fail;
	}

 	if( !md->threshold ) {
		if( ni->debug & PTL_DEBUG_THRESHOLD )
			nal->cb_printf( nal,
				"%d: ACK to inactive MD %d from %d\n",
				ni->nid,
				current,
				hdr->src.rid
			);
		goto fail;
	}

	if( ni->debug & PTL_DEBUG_ACK ) {
		nal->cb_printf( nal,
			"%d: ACK from %d into md %ld\n",
			ni->rid,
			hdr->src.rid,
			current
		);
        }

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal, "parse_ack: BAD - could not allocate cookie!\n");
          return 1;
        }

	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_ACK;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.nid   = hdr->src.nid;
		msg->ev.initiator.pid   = hdr->src.pid;
		msg->ev.initiator.gid   = hdr->src.gid;
		msg->ev.initiator.rid   = hdr->src.rid;
		msg->ev.mlength		= hdr->msg.ack.mlength;
		msg->ev.match_bits      = hdr->msg.ack.match_bits;

		msg->return_md		= -1;

		lib_md_deconstruct( nal, md, &msg->ev.md );
	}

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {
	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	ni->counters.recv_count++;

	nal->cb_recv( nal, private, msg, NULL, 0, 0 );
	return 0;

fail:
	/* Must allow the NAL to clean up anyway */
	nal->cb_recv( nal, private, NULL, NULL, 0, 0 );
	return -1;
}


void
print_hdr( nal_cb_t *nal, ptl_hdr_t *hdr)
{

char *type_str;


    switch (hdr->type)   {
	case PTL_MSG_ACK:	type_str= "Acknowledgement"; break;
	case PTL_MSG_PUT:	type_str= "Put Request"; break;
	case PTL_MSG_GET:	type_str= "Get Request"; break;
	case PTL_MSG_REPLY:	type_str= "Reply (to a get request)"; break;
	case PTL_MSG_BARRIER:	type_str= "Barrier"; break;
	default:		type_str= "Unknown";
    }

    nal->cb_printf( nal,"P3 Header at %p of type %s\n", hdr, type_str);
    nal->cb_printf( nal,"    From gid/rid %d/%d, nid/pid %d/%d",
	    hdr->src.gid, hdr->src.rid, hdr->src.nid, hdr->src.pid);
    nal->cb_printf( nal,"    To nid/pid %d/%d\n", hdr->nid, hdr->pid);

    if (hdr->type == PTL_MSG_PUT)   {
	nal->cb_printf( nal,"    Ptl index %d, ack md 0x%08x, match bits 0x%0lx\n",
	    hdr->msg.put.ptl_index, hdr->msg.put.ack_md,
	    hdr->msg.put.match_bits);
	nal->cb_printf( nal,"    Length %d, offset %d, hdr data 0x%0lx\n",
	    hdr->msg.put.length, hdr->msg.put.offset, hdr->msg.put.hdr_data);
    }

    if (hdr->type == PTL_MSG_GET)   {
	nal->cb_printf( nal,"    Ptl index %d, return md 0x%08x, match bits 0x%0lx\n",
	    hdr->msg.get.ptl_index, hdr->msg.get.return_md,
	    hdr->msg.get.match_bits);
	nal->cb_printf( nal,"    Length %d, src offset %d, return offset %d\n",
	    hdr->msg.get.length, hdr->msg.get.src_offset,
	    hdr->msg.get.return_offset);
    }

    if (hdr->type == PTL_MSG_ACK)   {
	nal->cb_printf( nal,"    dst md %d, manipulated length %d\n",
	    hdr->msg.ack.dst_md, hdr->msg.ack.mlength);
    }

    if (hdr->type == PTL_MSG_REPLY)   {
	nal->cb_printf( nal,"    dst md %d, dst offset %d, length %d\n",
	    hdr->msg.reply.dst_md, hdr->msg.reply.dst_offset,
	    hdr->msg.reply.length);
    }

    if (hdr->type == PTL_MSG_BARRIER)   {
	nal->cb_printf( nal,"    sequence %d\n", hdr->msg.barrier.sequence);
    }

}  /* end of print_hdr() */


int lib_parse( nal_cb_t *nal, ptl_hdr_t *hdr, void *private )
{

	if( 0 ) {
		nal->cb_printf( nal, "%d: lib_parse: nal=%p hdr=%p type=%d\n",
			nal->ni.rid, nal, hdr, hdr->type );
		print_hdr( nal, hdr );
	}

	if( hdr->type == PTL_MSG_ACK )
		return parse_ack( nal, hdr, private );
	if( hdr->type == PTL_MSG_PUT )
		return parse_put( nal, hdr, private );
	if( hdr->type == PTL_MSG_GET )
		return parse_get( nal, hdr, private );
	if( hdr->type == PTL_MSG_REPLY )
		return parse_reply( nal, hdr, private );

	nal->cb_printf( nal, "%d: lib_parse: Header corrupted.  Type=0x%x\n",
		nal->ni.nid, hdr->type );

	/*
	 * There was something coming in, but we don't know what it was.
	 * Tell the NAL to clean up, anyway.
	 */
	nal->cb_recv( nal, private, NULL, NULL, 0, 0 );
	return -1;
}


int do_PtlPut( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t md_in
	 *	ptl_ack_req_t ack_req_in
	 *	ptl_process_id_t target_in
	 *	ptl_pt_index_t portal_in
	 *	ptl_ac_index_t cookie_in
	 *	ptl_match_bits_t match_bits_in
	 *	ptl_size_t offset_in
	 *
	 * Outgoing:
	 */
	
	PtlPut_in		*args	= v_args;
	PtlPut_out		*ret	= v_ret;
	ptl_process_id_t	id_out;
	ptl_hdr_t		hdr;

	lib_ni_t		*ni	= &nal->ni;
	int			current	= args->md_in & 0xFFFF;
	lib_md_t		*md	= &ni->md[current];
	lib_msg_t		*msg	= NULL;
	ptl_process_id_t	*id	= &args->target_in;


	ret->rc = PTL_OK;
	if( current < 0 || current > MAX_MDS || !inuse( md ) || !md->threshold )
		return ret->rc = PTL_INV_MD;

	if( ni->debug & PTL_DEBUG_PUT )
		nal->cb_printf( nal, "%d: PtlPut -> %d: %d/%d %d/%d\n",
			ni->rid,
			id->addr_kind,
			id->nid, id->pid,
			id->gid, id->rid
		);

	/*
	** We don't know if the user gave us PTL_ADDR_NID, PTL_ADDR_GID, or
	** PTL_ADDR_BOTH, but we need PTL_ADDR_NID!
	*/
	if( lib_trans_id(nal, args->target_in, &id_out ) != PTL_OK)   {
		nal->cb_printf( nal, "%d: do_PtlPut: lib_trans_id() failed\n",
			ni->rid );
		return ret->rc = PTL_INV_PROC;
	}

	if( ni->debug & PTL_DEBUG_PUT )
		nal->cb_printf( nal, "%d: PtlPut translation nid/pid %d/%d\n",
		        ni->rid,
			id_out.nid, id_out.pid);


	hdr.nid			= id_out.nid;
	hdr.pid			= id_out.pid;
	hdr.type		= PTL_MSG_PUT;
        hdr.src.nid             = ni->nid;
	hdr.src.pid             = ni->pid;
        hdr.src.gid             = ni->gid;
	hdr.src.rid             = ni->rid;
	hdr.msg.put.ptl_index	= args->portal_in;
	hdr.msg.put.match_bits	= args->match_bits_in;
	hdr.msg.put.length	= md->length;
	hdr.msg.put.offset	= args->offset_in;
	hdr.msg.put.hdr_data    = args->hdr_data_in;

	if( args->ack_req_in == PTL_ACK_REQ )
		hdr.msg.put.ack_md	= current ;
	else
		hdr.msg.put.ack_md	= -1;
	

	ni->counters.send_count++;
	ni->counters.send_length += md->length;

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal, "do_PtlPut: BAD - could not allocate cookie!\n");
          return ret->rc = PTL_FAIL;
        }

	/*
	 * If this memory descriptor has an event queue associated with
	 * it we need to allocate a message state object and record the
	 * information about this operation that will be recorded into
	 * event queue once the message has been completed.
	 */
	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_SENT;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.gid   = ni->gid;
		msg->ev.initiator.rid   = ni->rid;
		msg->ev.initiator.nid   = ni->nid;
		msg->ev.initiator.pid   = ni->pid;
		msg->ev.portal		= args->portal_in;
		msg->ev.match_bits	= args->match_bits_in;
		msg->ev.rlength		= md->length;
		msg->ev.mlength		= md->length;
		msg->ev.offset		= args->offset_in;
		msg->ev.hdr_data        = args->hdr_data_in;

		lib_md_deconstruct( nal, md, &msg->ev.md );

		msg->return_md		= -1;
	}

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {
	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	/*
	 * Finally, we ask the NAL to send the bytes from user space
	 * to the destination node.  Once it has been completed the
	 * NAL should call lib_finalize(), passing in the message
	 * state object pointer and the event queue entry will be
	 * written out (with nal->write()).
	 */
	ret->rc= nal->cb_send( nal, private, msg,
		&hdr,
		id_out.nid,
		id_out.pid,
		id_out.gid,
		id_out.rid,
		md->start, md->length);

	return ret->rc;
}


int do_PtlGet( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_md_t md_in
	 *	ptl_process_id_t target_in
	 *	ptl_pt_index_t portal_in
	 *	ptl_ac_index_t cookie_in
	 *	ptl_match_bits_t match_bits_in
	 *	ptl_size_t offset_in
	 *
	 * Outgoing:
	 */
	
	PtlGet_in		*args	= v_args;
	PtlGet_out		*ret	= v_ret;
	ptl_process_id_t	id_out;

	ptl_hdr_t		hdr;
	lib_msg_t		*msg	= NULL;
	lib_ni_t		*ni	= &nal->ni;
	ptl_process_id_t	*id	= &args->target_in;

	int			current	= args->md_in & 0xFFFF;
	lib_md_t		*md	= &ni->md[current];

	ret->rc = PTL_OK;
	if( current < 0 || current > MAX_MDS || !inuse( md ) || !md->threshold )
		return ret->rc = PTL_INV_MD;

	if( ni->debug & PTL_DEBUG_GET )
		nal->cb_printf( nal, "%d: PtlGet -> %d: %d/%d %d/%d\n",
			ni->rid,
			id->addr_kind,
			id->nid, id->pid,
			id->gid, id->rid
		);

	if( lib_trans_id(nal, args->target_in, &id_out ) != PTL_OK)   {
		nal->cb_printf( nal, "do_PtlGet() lib_trans_id() failed\n");
		return ret->rc = PTL_INV_PROC;
	}

	hdr.type		= PTL_MSG_GET;
	hdr.nid			= id_out.nid;
	hdr.pid			= id_out.pid;
	hdr.src.nid             = ni->nid;
	hdr.src.pid             = ni->pid;
	hdr.src.gid             = ni->gid;
	hdr.src.rid             = ni->rid;
	hdr.msg.get.ptl_index	= args->portal_in;
	hdr.msg.get.match_bits	= args->match_bits_in;
	hdr.msg.get.length	= md->length;
	hdr.msg.get.src_offset	= args->offset_in;
	hdr.msg.get.return_offset	= md->offset;
	hdr.msg.get.return_md	= md - ni->md;
	

	ni->counters.send_count++;

        msg = lib_msg_alloc( nal );

        if ( ! msg ) {
	  nal->cb_printf(nal, "do_PtlGet: BAD - could not allocate cookie!\n");
          return ret->rc = PTL_FAIL;
        }

	/*
	 * If this memory descriptor has an event queue associated with
	 * it we must allocate a message state object that will record
	 * the information to be filled in once the message has been
	 * completed.  More information is in the do_PtlPut() comments.
	 */
	if( md->eq ) {
		md->eq->pending++;

		msg->ev.type		= PTL_EVENT_SENT;
		msg->ev.initiator.addr_kind = PTL_ADDR_BOTH;
		msg->ev.initiator.nid   = ni->nid;
		msg->ev.initiator.pid   = ni->pid;
		msg->ev.initiator.gid   = ni->gid;
		msg->ev.initiator.rid   = ni->rid;
		msg->ev.portal		= args->portal_in;
		msg->ev.match_bits	= args->match_bits_in;
		msg->ev.rlength		= md->length;
		msg->ev.mlength		= md->length;
		msg->ev.offset		= args->offset_in;
		msg->ev.hdr_data        = 0;

		lib_md_deconstruct( nal, md, &msg->ev.md );

		msg->return_md		= -1;
	}

	/* decrement the threshold */
	md->threshold -= (md->threshold == PTL_MD_THRESH_INF) ? 0 : 1;

	if ( md->unlink == PTL_UNLINK ) {
	    if ( md->threshold == 0 ) {
		md->do_unlink = 1;
	    }
	}

	/* hang on to the md */
	msg->md = md;

	/* indicate a pending operation */
	md->pending++;

	ret->rc= nal->cb_send( nal, private, msg,
		&hdr,
		id_out.nid,
		id_out.pid,
		id_out.gid,
		id_out.rid,
		NULL, 0
	);

	return ret->rc;
}

