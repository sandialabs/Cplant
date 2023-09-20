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
** $Id: Pkt_handler.h,v 1.6 2001/08/22 16:29:06 pumatst Exp $
*/

#ifndef HANDLER_H
#define HANDLER_H

#include <linux/netdevice.h>	/* For struct device */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif


    void myrpkt_handle_interrupt(int unit, struct NETDEV *dev, int mcp_type);

#endif /* HANDLER_H */
