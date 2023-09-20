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
** $Id: ptlPkt.h,v 1.9 2001/02/16 05:36:32 lafisk Exp $
*/


#ifndef PTLPKT_H
#define PTLPKT_H

#include <linux/netdevice.h>
#include <linux/if_ether.h>	/* For struct ethhdr */

#define ETH_P_PORTAL  0x8f00    /* portal protocol -- get it 
                                   out of the kernel */

enum { STREAM, ACK, RESET };

typedef struct {
	int		dstPid;
	int		srcPid;
	int		seqNum;
	INT16 		pktNum;
	INT16		numPkts;
	INT16 		srcNid;
	INT16		dstNid;
	unsigned char	dstPtl;
	unsigned char	ackMe;
	unsigned char	packetType;
	char	truncBytes;
} ptlhdr_t;

extern void ptlPktExit( int pid );
extern int 
portal_rcv( struct sk_buff *skb, struct device *dev, struct packet_type *pt );
extern void processResetPacket( int nid );
extern void ptlPktInit( void );

#define PORTAL_PKT_PAYLOAD(mtu) (mtu - sizeof (ptlhdr_t))


static inline void printPtlHdr( ptlhdr_t *ptl )
{
    printk("portals: PTL HDR: totalNum %i, packNum %i, srcNode %i, srcPid %i\n",
                 ptl->numPkts, ptl->pktNum, ptl->srcNid, ptl->srcPid );
}

#endif
