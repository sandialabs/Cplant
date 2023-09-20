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
** $Id: RTSCTS_recv.h,v 1.19 2001/08/22 16:29:07 pumatst Exp $
** The functions to receive and handle packets
*/
#ifndef RECV_H
#define RECV_H

#include <linux/netdevice.h>	/* For struct device */
#include <defines.h>		/* For UINT16 used in idtypes.h */
#include <p30/lib-nal.h>	/* For nal_cb_t */
#include "MCPshmem.h"		/* Import MAX_NUM_ROUTES */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/*
** Reserve slots in the MCP and pages to receive RTS requests from
** OTHER_NODES many nodes simultaneously.
*/
#define OTHER_NODES	(512)


/*
** Max number of data packets we ever let a sender send to us with a
** single CTS
*/
#define MAX_DATA_PKTS	(16)


extern int rtscts_recv(unsigned long page, struct NETDEV *dev);
extern unsigned int reset_Qentry(unsigned int msgID);

#ifdef DO_TIMEOUT_PROTOCOL
    extern void handleDataResend(unsigned int msgID);
#endif /* DO_TIMEOUT_PROTOCOL */

#ifdef MSGCKSUM
    extern int checksum(struct task_struct *task, char *buf, int len,
		int sndlen, unsigned int orig_crc);
#endif /* MSGCKSUM */

#endif /* RECV_H */
