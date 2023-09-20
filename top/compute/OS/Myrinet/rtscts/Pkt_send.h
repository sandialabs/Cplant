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
** $Id: Pkt_send.h,v 1.7 2001/10/23 22:29:27 jsotto Exp $
** Send a packet over Myrinet
*/
#ifndef PKT_SEND_H
#define PKT_SEND_H

#include <linux/skbuff.h>

    int pkt_init_send(void);
    void pkt_free_snd_pages(void);
    int get_snd_page(void);
    void free_snd_page(int index);
    unsigned long page_addr(int page_idx);
    int myrPkt_xmit(unsigned short dst_nid, int page_idx, int len);
    int skb_xmit(unsigned short dst_nid, struct sk_buff *skb);

    #ifdef BUFDEBUG
	unsigned long *getSndPage(int back, int *idx, int *dnid, int *stat);
    #endif /* BUFDEBUG */

#endif /* PKT_SEND_H */
