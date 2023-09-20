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
** $Id: ptlPktHandler.h,v 1.2 1998/03/18 23:32:57 mjleven Exp $
*/


#ifndef PTLPKTHANDLER_H
#define PTLPKTHANDLER_H

#include <linux/netdevice.h>
#include <portals/pkt/ptlPkt.h>

extern int portalPacketProcess( struct device *dev,
				ptlhdr_t *pkt, void *data, int nbytes );

extern int module_portalPacketFree( struct sk_buff *skb );


#endif
