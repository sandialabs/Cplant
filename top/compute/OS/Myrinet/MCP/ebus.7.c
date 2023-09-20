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
** $Id: ebus.7.c,v 1.3 2000/09/21 09:18:43 rolf Exp $
**
** This file contains functions that deal with the EBUS side; i.e.
** functions to DMA to and from the host, that are specific to LANai 7.x
*/

#include "ebus.h"

volatile struct DMA_BLOCK l2e_dma[2] __attribute__ ((aligned (8)));
volatile struct DMA_BLOCK e2l_dma[2] __attribute__ ((aligned (8)));

int PCIavailable= TRUE;

/******************************************************************************/
/*
** For now we create a circular list with a single element for the
** read (ebus to lbus) and write (lbus to ebus) DMA channels.
*/
void
ebus_dma_init(void)
{

    l2e_dma[0].next= (unsigned int) 0 | DMA_L2E | DMA_WAKE | DMA_TERMINAL;
    e2l_dma[0].next= (unsigned int) 0 | DMA_E2L | DMA_WAKE | DMA_TERMINAL;
    l2e_dma[1].next= (unsigned int) 0 | DMA_L2E | DMA_WAKE | DMA_TERMINAL;
    e2l_dma[1].next= (unsigned int) 0 | DMA_E2L | DMA_WAKE | DMA_TERMINAL;

    mcpshmem->DMAchannel0= (unsigned int) &(l2e_dma[0]);
    mcpshmem->DMAchannel2= (unsigned int) &(e2l_dma[0]);

    mcpshmem->LANai2host= INT_DMA_SETUP;
    GM_STBAR();
    SET_HOST_SIG_BIT();

    /* Wait for the host to do that */
    while (mcpshmem->DMAchannel0 != 0)   {
    }
    while ( !(EIMR & HOST_SIG_BIT))   {
    }

}  /* end of ebus_dma_init() */

/******************************************************************************/
