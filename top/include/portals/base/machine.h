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
#ifndef MACHINE_H
#define MACHINE_H

#ifdef DEBUG_ON
#define PRINTK(args...) printk("portals: " args)
#else
#define PRINTK(args...) 
#endif

typedef enum {PERR_NOTHING= 0, PERR_INVALPTL, PERR_SEND, PERR_INVAL_LEN,
    PERR_INVAL_LEN2, PERR_INVAL_BUF, PERR_INVAL_SFLAG, PERR_MCP_VER,
    PERR_INVAL_LEN3, PERR_BLKD, PERR_CPYBDY, PERR_NO_HDR, PERR_NO_TASK_INFO,
    PERR_BAD_PTL, PERR_INACTIVE_PTL, PERR_RCVERR, PERR_UNIMPL, PERR_INVAL_IDX,
    PERR_MLOOPS, PERR_BAD_MD, PERR_SBUFOVFLW, PERR_NOROOM, PERR_INDNOTFREE,
    PERR_INDBUFSHORT, PERR_NO_TASK_INFO2, PERR_MCP_NOTSNDRDY, PERR_ENQERR,
    PERR_STALL, PERR_NOPAGE, PERR_VERIFYMEM, PERR_DROPPED,
} perr_t;


#include <linux/sched.h>
#ifdef __i386__
#include "i386.h"
#endif

#ifdef __alpha__
#include "alpha.h"
#endif

#ifdef __ia64__
#include "ia64.h"
#endif

#endif
