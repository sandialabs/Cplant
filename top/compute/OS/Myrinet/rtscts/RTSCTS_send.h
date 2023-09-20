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
** $Id: RTSCTS_send.h,v 1.22 2001/10/23 22:29:27 jsotto Exp $
** The functions to send a message
*/
#ifndef SEND_H
#define SEND_H

#include <linux/netdevice.h>	/* For struct device */
#include <p30/lib-types.h>	/* For ptl_hdr_t */
#include <p30/lib-nal.h>	/* For nal_cb_t */
#include "RTSCTS_protocol.h"	/* For pkt_type_t */
#include "RTSCTS_pkthdr.h"	/* For pkthdr_t */
#include "queue.h"		/* For Qentry_t */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif


    unsigned int next_msgNum(int dest_nid, int read_only);
    unsigned int incMsgNum(unsigned int num);
    void sendProtoMSG(pkt_type_t type, unsigned int msgID,
	unsigned short dst_nid, unsigned int info, unsigned short int info2,
        int payload, void *buf);
    void sendDATA(Qentry_t *entry, int num_pkts);
    int myr_send(struct NETDEV *dev, void *buf, int len, int dst_nid);
#ifndef RTSCTS_OVER_ETHERNET
    int build_page(pkt_type_t type, unsigned int msgID, 
                   ptl_hdr_t *phdr, unsigned short dst_nid, 
                   int payload, unsigned char **data_start,
	           pkthdr_t **pkthdr_start, ptl_hdr_t **ptlhdr_start,
	           unsigned int msgNum);
#else
    struct sk_buff*
        build_skb(pkt_type_t type, unsigned int msgID, 
                  ptl_hdr_t *phdr, unsigned short dst_nid, 
                  int payload, unsigned char **data_start,
	          pkthdr_t **pkthdr_start, ptl_hdr_t **ptlhdr_start,
	          unsigned int msgNum);
#endif
    void abort_old_msg(unsigned int msgID);
    void compute_crc(int src_pid, char *buf, int len);
    unsigned int compute_pktcrc(int page_idx, int len);
    unsigned int next_msgID(void);

#ifdef MSGCKSUM
    extern unsigned int crc;
#endif /* MSGCKSUM */

#endif /* SEND_H */
