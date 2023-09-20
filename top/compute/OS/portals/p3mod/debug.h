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
** $Id: debug.h,v 1.5 2002/02/20 22:30:46 jbogden Exp $
** Debugging information about P3 messages based on progress through the
** rts/cts protocol.
*/

#ifndef P3_DEBUG_H
#define P3_DEBUG_H

#include <p30/lib-types.h>	/* For ptl_hdr_t */

typedef enum {SND_INITIATED= 0, SND_QUEUED, SND_DEQUEUED, SND_STARTED,
    SND_FINISHED, RCV_STARTED, RCV_FINISHED} p3_event_type_t;

typedef enum {P3STAT_OK= 0, P3STAT_NOMEM, P3STAT_NOPAGE, P3STAT_BADCPY,
    P3STAT_ABORTED, P3STAT_DROPPED, P3STAT_CHKSUM, P3STAT_NOMCP, P3STAT_LEN,
    P3STAT_HDR, P3STAT_BUILDPG, P3STAT_NAL, P3STAT_MEMCPYF, P3STAT_BADXMIT,
    P3STAT_PROC, P3STAT_DOOM, P3STAT_SEQ, P3STAT_MEMCPYT, P3STAT_PROTO,
    P3STAT_FINALIZE, P3STAT_DSTNID} p3_status_t;

void p3_debug_init(void);
void p3_debug_add(unsigned int msgID, ptl_hdr_t *hdr, p3_event_type_t event,
        p3_status_t status);
void p3_debug_proc(char **pb_ptr, char *pb_end);

#endif /* P3_DEBUG_H */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
