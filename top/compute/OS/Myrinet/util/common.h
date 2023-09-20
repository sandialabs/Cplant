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
** $Id: common.h,v 1.13 2001/08/22 16:45:14 pumatst Exp $
** Function definitions for common.c
*/

#ifndef COMMON_H
#define COMMON_H

#include "MCPshmem.h"

#ifdef __KERNEL__
    /* We want to refer  to the local copy of the shared data structure */
    #define shmemloc	hstshmem
#else
    /* We want to refer  to the LANai copy of the shared data structure */
    #define shmemloc	mcpshmem
#endif /* __KERNEL__ */


#define GET_SND_HEAD_LEN(entry) \
    ((ntohl(shmemloc->snd_xfer[entry].len) & LEN_HEAD_MASK) >> LEN_HEAD_SHIFT)
#define GET_SND_TAIL_LEN(entry) \
    ((ntohl(shmemloc->snd_xfer[entry].len) & LEN_TAIL_MASK) >> LEN_TAIL_SHIFT)
#define GET_SND_BODY_LEN(entry) \
    (ntohl(shmemloc->snd_xfer[entry].len) & LEN_BODY_MASK)

#define GET_RCV_HEAD_LEN(entry) \
    ((ntohl(shmemloc->rcv_xfer[entry].len) & LEN_HEAD_MASK) >> LEN_HEAD_SHIFT)
#define GET_RCV_TAIL_LEN(entry) \
    ((ntohl(shmemloc->rcv_xfer[entry].len) & LEN_TAIL_MASK) >> LEN_TAIL_SHIFT)
#define GET_RCV_BODY_LEN(entry) \
    (ntohl(shmemloc->rcv_xfer[entry].len) & LEN_BODY_MASK)

#define SND_HEAD(entry) (shmemloc->snd_xfer[entry].head)
#define SND_TAIL(entry) (shmemloc->snd_xfer[entry].tail)
#define RCV_HEAD(entry) (shmemloc->rcv_xfer[entry].head)
#define RCV_TAIL(entry) (shmemloc->rcv_xfer[entry].tail)

#define GET_SND_FLAGS(entry) (ntohl(shmemloc->snd_xfer[entry].head) & FLAG_MASK)
#define GET_RCV_FLAGS(entry) (ntohl(shmemloc->rcv_xfer[entry].head) & FLAG_MASK)
#define CLR_SND_FLAGS(entry, flag) SND_HEAD(entry) &= htonl(~(flag))
#define CLR_RCV_FLAGS(entry, flag) RCV_HEAD(entry) &= htonl(~(flag))
#define ASG_SND_FLAGS(entry, flag) SND_HEAD(entry)= htonl(flag)
#define ASG_RCV_FLAGS(entry, flag) RCV_HEAD(entry)= htonl(flag)

int map_lanai(char *pname, int verbose, int unit);
mcpshmem_t *get_mcpshmem(int verbose, int unit, char *pname, int force);
int get_mcp_mcpshmem(int unit);
int get_lanai_type(int unit, int print);

#endif /* COMMON_H */
