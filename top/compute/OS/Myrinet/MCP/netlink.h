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
** $Id: netlink.h,v 1.3 2001/09/21 16:42:45 pumatst Exp $
** This file contains functions that deal with the network DMA.
*/

#ifndef NETLINK_H
#define NETLINK_H

#include "lanai_def.h"


extern __inline__ void snd_bufEOM(int *buf, int len);
extern __inline__ void snd_buf(int *buf, int len);


#if defined (L4)
    #include "netlink.4.h"
#endif
#if defined (L7) || defined (L9)
    #include "netlink.7.h"
#endif

#endif /* NETLINK_H */
