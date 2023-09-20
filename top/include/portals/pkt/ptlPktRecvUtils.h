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
** $Id: ptlPktRecvUtils.h,v 1.5 1998/09/14 21:44:26 rolf Exp $
*/

#ifndef PTLPKTRECVUTILS_H
#define PTLPKTRECVUTILS_H

#include <portals/base/debug.h>
#include <portals/base/ptlStateInfo.h>
#include <portals/base/machine.h>
#include <portals/base/updateUserInfo.h>

#ifdef MDEBUG
    #include "mdebug.h"
#endif MDEBUG

static inline void ptlPktRecvBody( int pid, int srcNid, void *dstAddr, 
				    int bytes, UINT32 *active_cnt, int decr)
{
    PRINTK("ptlPktRecvBody() dstAddr 0x%p, bytes %i, active_cnt %i\n",
                            dstAddr, bytes, get_user2( pid, active_cnt ) );

    if (dstAddr != NULL)   {
	rlist[srcNid].usrAddr = dstAddr;
    }
    incUserInfo(pid, active_cnt, -decr + 1);
    #ifdef MDEBUG
	mdebugInc(PDD_NPUTUSR2);
	mdebugAdd(PDD_LPUTUSR2, bytes);
    #endif MDEBUG
}

#endif
