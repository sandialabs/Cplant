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
** $Id: RTSCTS_p3.h,v 1.18.2.1 2002/05/22 21:40:02 jbogden Exp $
*/
#ifndef RTSCTS_P3_H
#define RTSCTS_P3_H

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/* defines for the opt parameter passed to p3_send() */
  #define NOOP                  0
  #define SEND_P3_PING_REQ      1
  #define SEND_P3_PING_ACK      2
#ifdef EXTENDED_P3_RTSCTS
  #define SEND_P3_NULL          3
  #define SEND_P3_SYNC          4
#endif /* EXTENDED_P3_RTSCTS */

/* error number defines */
#define P3_OK                   0
#define P3_DEST_NOT_ACKED       -1
#define P3_MCPSHMEM_ERROR       -2
#define P3_INVALID_LEN          -3
#define P3_NAL_ERROR            -4
#define P3_FAILED_BUF_MEMCPY    -5
#define P3_XMIT_FAILED          -6
#define P3_BUILD_PAGE_FAILED    -7
#define P3_ENQUEUE_FAILED       -8
#define P3_INVALID_DEST         -9


    int p3_recvBody(nal_cb_t *nal, void *private, void *data, size_t mlen,
                    size_t rlen, lib_msg_t *cookie);
    void handleP3(unsigned int msgID, unsigned int msgNum, ptl_hdr_t *hdr,
                  void *pkt, unsigned long len, pkt_type_t type,
                  struct NETDEV *dev, unsigned int crc);
    int p3_send(nal_cb_t *nal, void *buf, size_t len, int dst_nid,
                ptl_hdr_t *hdr, lib_msg_t *cookie, unsigned int msgID,
                unsigned int msgNum, unsigned int opt);
    int p3_dist(nal_cb_t *nal, int nid, unsigned long *dist );
    void p3_exit(void *task);
    int p3_send_ping(unsigned long nid);
    int p3_send_ping_ack(unsigned long nid, unsigned int msgID);
    int p3_get_ping(unsigned long nid);

    #ifdef DO_TIMEOUT_PROTOCOL
	void handleP3resend(unsigned int msgID, unsigned int msgNum);
    #endif /* DO_TIMEOUT_PROTOCOL */

    #if defined(DO_TIMEOUT_PROTOCOL) && defined(EXTENDED_P3_RTSCTS)
    void handleP3sync(unsigned int msgID, unsigned int msgNum);
    #endif /* (DO_TIMEOUT_PROTOCOL) && (EXTENDED_P3_RTSCTS) */
    
#endif /* RTSCTS_P3_H */
