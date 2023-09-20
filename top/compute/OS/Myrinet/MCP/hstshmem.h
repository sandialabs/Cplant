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
** $Id: hstshmem.h,v 1.19 2001/08/22 23:00:32 pumatst Exp $
** This file describes the data structure in wired down, DMAable
** memory on the host that is shared between the LANai and the host.
*/

#ifndef HSTSHMEM_H
#define HSTSHMEM_H

typedef struct   {
    int first;		/* Make sure this is always the first field!!! */
    int pad;		/* start_info has to be 64 bit aligned ! */


    /* Some counters */
    long unaligned_snd_head;
    long unaligned_snd_tail;
    long unaligned_rcv_head;
    long unaligned_rcv_tail;

    long total_snd_bytes;
    long total_snd_msgs;
    long total_rcv_bytes;
    long total_rcv_msgs;

    #if !defined(__alpha__)
	/* A long is only 4 bytes. Pad it out (for the MCP for example) */
	int mcp_pad1[8];
    #endif

    /* -------- no more 8 byte aligned entries below rcv_header[] ----------- */

    volatile int rdy_to_snd;
    volatile int my_pnid;		/* Used to put into Puma header */
    volatile int LANai2host;		/* reason for LAnai to host interrupt */

    int mod_type;			/* What type of module is this? */

    int pad2[32 - 6 - (8 * 2)];
} hstshmem_t;

extern hstshmem_t *hstshmem;

#endif /* HSTSHMEM_H */
