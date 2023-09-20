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
** $Id: processAckPacket.h,v 1.2 1998/09/10 20:35:57 rolf Exp $
*/
#include <portals/pkt/ptlPkt.h>
#include <portals/pkt/sendThread.h>
#ifdef MDEBUG
    #include "mdebug.h"
#endif MDEBUG


static inline void processAckPacket( ptlhdr_t *ptlhdr )
{
    queue_t        *next = GL_sentQ;
    int j1 = 0;
    int j2 = 0;
    int j3 = 0;

    PRINTK("processAckPacket() packet num %i from node %i pid %i\n",
                             ptlhdr->pktNum, ptlhdr->srcNid, ptlhdr->srcPid );

    /* check the list of packets waiting for ACKS */
    while ( ( next = QgetNext( next ) ) != GL_sentQ->prev ) {
        if ( next && next->data ) {
            sentPktQent_t  *pktEnt = (sentPktQent_t *) next->data;

	    if ( ptlhdr->srcNid == pktEnt->sendEntry->info.hdr.dst_nid ) {
		 j1= 1;
		 j3 = pktEnt->seqNum;
		 if ( ptlhdr->seqNum == pktEnt->seqNum) {

		    PRINTK("processAckPacket() numPktsAcked %i, total %i \n",
			pktEnt->sendEntry->numPktsAcked,
			pktEnt->sendEntry->numPkts );

		    j2 = 1;
		    ++slist[pktEnt->sendEntry->info.hdr.dst_nid].seqNum;
		    freeSentEntry( next );
		    break;
		 }
            }
        }
    }

    #ifdef MDEBUG
	if (j2 == 0)   {
	    mdebugInc(PDD_IGNACK);  /* The ack was not used! */
	}
    #endif MDEBUG
    PRINTK("processAckPacket(), returning\n");
}
