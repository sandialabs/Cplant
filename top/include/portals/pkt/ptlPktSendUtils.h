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
** $Id: ptlPktSendUtils.h,v 1.11 1998/09/03 21:08:11 rolf Exp $
*/

#ifndef PTLPKTSENDUTILS_H
#define PTLPKTSENDUTILS_H

#include <portals/base/ptlStateInfo.h>
#include <portals/pkt/ethernetPacketSend.h>
#include <portals/pkt/ptlPkt.h>
#include <portals/pkt/sendThread.h>

enum { HEAD, BODY };
static inline void sendResetPacket( int dstNid )
{
    ptlhdr_t	new;	
    PRINTK("sendResetPacket to %i from %i\n", dstNid, ptlGetPhysNid());

    new.ackMe = FALSE;
    new.packetType = RESET;
    new.srcNid = ptlGetPhysNid();
    new.srcPid = -1;
    new.pktNum = 0;
    new.numPkts = 1;
    new.dstNid = dstNid;
    new.dstPid = -1;
    new.seqNum = 0;

    ethernetPacketSend( FALSE, ptlDeviceGetDev( dstNid ), &new, NULL, 0, 0);
}

static inline void sendAckPacket( ptlhdr_t *ptlhdr )
{
    ptlhdr_t	new;	
    PRINTK("sendAckPacket to %i from %i\n", ptlhdr->srcNid, ptlGetPhysNid());

    new.ackMe = FALSE;
    new.packetType = ACK;
    new.srcNid = ptlGetPhysNid();
    new.srcPid = -1;
    new.pktNum = ptlhdr->pktNum;
    new.numPkts = 1;
    new.dstNid = ptlhdr->srcNid;
    new.dstPid = ptlhdr->srcPid;
    new.seqNum = ptlhdr->seqNum;

    ethernetPacketSend( FALSE, ptlDeviceGetDev( new.dstNid ), &new, NULL, 0, 0);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static inline 
void *sendStreamPacket( int type, pktSendEntry_t *entry, int pktNum, 
						void *body, int nbytes )
{
    void 		*retval;	
    ptlhdr_t 		ptlhdr;  

    PRINTK("sendStreamPacket type %i, pktNum %i body %p nbytes %i\n",
		type, pktNum, body, nbytes );
    ptlhdr.packetType = STREAM;
    ptlhdr.srcNid = entry->info.hdr.src_nid;
    ptlhdr.srcPid = entry->info.hdr.src_pid;
    ptlhdr.dstNid = entry->info.hdr.dst_nid;
    ptlhdr.dstPid = entry->info.hdr.dst_pid;
    ptlhdr.dstPtl = entry->info.hdr.dst_portal;
    ptlhdr.pktNum = pktNum;	
    ptlhdr.numPkts = entry->numPkts;
    ptlhdr.truncBytes = 0; 
    ptlhdr.seqNum = slist[entry->info.hdr.dst_nid].seqNum + 
			slist[entry->info.hdr.dst_nid].pending;

    PRINTK("%i sending packnum %i to %i, seq num %i \n",
			entry->info.hdr.src_nid, ptlhdr.pktNum, 
			entry->info.hdr.dst_nid, ptlhdr.seqNum);

    if ( type == HEAD ) {
	/*
	** Note that we don't worry about truncBytes here because of the
	** PUMA_HDR; i.e. we're guaranteed longer than the minimum Ethernet
	** packet size.
	*/ 
        retval = ethernetPacketSend( TRUE, entry->info.dev, &ptlhdr,
		(void *) &entry->info.hdr, entry->info.hdrSize, KERNEL_SPACE,
		body, nbytes, USER_SPACE, NULL  );
    } else {
        if ( entry->info.dev->mtu == ETH_DATA_LEN ) {
            /* note that this relies on sizeof(ptlhdr_t) < ETH_ZLEN */
	    if ( nbytes < ETH_ZLEN - ( ETH_HLEN + sizeof( ptlhdr_t) ) ) {
            	ptlhdr.truncBytes = 
		    ((ETH_ZLEN - ( ETH_HLEN + sizeof( ptlhdr_t) ))) - nbytes;
	    }
        } 
        retval = ethernetPacketSend( TRUE, entry->info.dev, (void *) &ptlhdr, 
		body, nbytes, USER_SPACE, NULL );
    }

    if ( retval  ) {
    	incPending();

        QsentPkt( entry, retval, ptlhdr.seqNum );
	++slist[entry->info.hdr.dst_nid].pending;

    }
    PRINTK("sendStreamPacket(), returning\n"); 
    return retval;
}

#endif
