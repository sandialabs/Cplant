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
** $Id: msg.c,v 1.29 2002/01/08 01:18:43 ktpedre Exp $
*/
/*
 * lib-p30/msg.c
 *
 * Message decoding, parsing and finalizing routines
 */

#include <stdio.h>
#include <lib-p30.h>

int lib_finalize( nal_cb_t *nal, void *private, lib_msg_t *msg )
{
        lib_md_t        *md;
	lib_eq_t	*eq;
	int		rc = 0;

	/* ni went down while processing this message */
	if ( nal->ni.up == 0 ) {
		return -1;
        }

	if ( ! msg ) {
	    return 0;
	}

	if( (eq = msg->md->eq) ) {
		ptl_event_t	*ev	= &msg->ev;

		ev->sequence            = eq->sequence++;

		if ( nal->cb_write( nal, private,
			(user_ptr) (eq->base + (ev->sequence % eq->size)),
			ev,
			sizeof( *ev )) !=0 ) {
                  return -1; 
                }

		if( --eq->pending < 0 ) {
			eq->pending = 0;
			nal->cb_printf( nal, "%d: eq %ld (%p) "
				"has negative events pending!\n",
				nal->ni.rid,
				(long)( eq - nal->ni.eq ), eq
			);
		}
	}

	if( msg->return_md >= 0 ) {
		ptl_hdr_t		ack;

		ack.type		= PTL_MSG_ACK;
		ack.nid			= msg->nid;
		ack.pid			= msg->pid;
		ack.src.nid             = nal->ni.nid;
		ack.src.pid             = nal->ni.pid;
		ack.src.gid             = nal->ni.gid;
		ack.src.rid             = nal->ni.rid;
		ack.msg.ack.dst_md	= msg->return_md;
		ack.msg.ack.mlength	= msg->ev.mlength;
		ack.msg.ack.match_bits  = msg->ev.match_bits;

		rc= nal->cb_send( nal, private, NULL,
			&ack,
			msg->nid,
			msg->pid,
			msg->gid,
			msg->rid,
			NULL, 0 );
	}

	if ( (md = msg->md) ) {
	    if( --md->pending == 0 ) {
		/* no more outstanding operations on this md */
		if ( md->do_unlink ) {
		    /* unlink the md and possibly the me */
		    lib_me_t *me = md->me;
		    lib_md_unlink( nal, md );
		    if( me && !me->md && me->unlink == PTL_UNLINK ) {
			lib_me_unlink( nal, me );
		    }
		}
	    }
	}

	lib_msg_free( nal, msg );

        return rc;
}
