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
** $Id: ebus.7.h,v 1.5 2002/02/14 18:38:12 jbogden Exp $
**
** This file contains functions that deal with the EBUS side; i.e.
** functions to DMA to and from the host, that are specific to LANai 7.x
*/

#ifndef EBUS_7_H
#define EBUS_7_H


/******************************************************************************/

extern volatile struct DMA_BLOCK l2e_dma[2] __attribute__ ((aligned (8)));
extern volatile struct DMA_BLOCK e2l_dma[2] __attribute__ ((aligned (8)));

static int l2e_toggle= 0;/* toggle between the two MCP to host chain elements */
static int e2l_toggle= 0;/* toggle between the two host to MCP chain elements */


/******************************************************************************/
/*
** l2e_dma_start()
** Initiates a transfer from a buffer in shared memory to the host. We
** assume that "buf" and "dest_addr" are 4 byte aligned. If len is not
** a multiple of 4, we round up. This overrides some bytes on the
** host side. We know it goes into an sk_buf and we are not destroying
** anything at the end of data. So we go ahead and do it.
*/
extern __inline__ void
l2e_dma_start(int *buf, int len, unsigned int dest_addr)
{

    if (len == 0)   {
	return;
    }
    fail_if (a13, len < 0);
    fail_if (a14, (unsigned)buf & ALIGN_32b);
    fail_if2 (a16, (dest_addr & ALIGN_32b), dest_addr, 0);

    #ifdef rrr
    len= (len + ALIGN_32b) & ~ALIGN_32b;
    fail_if (a81, len > DBL_BUF_SIZE);
    #endif /* rrr */

    l2e_dma[1 - l2e_toggle].next= 0 | DMA_L2E | DMA_TERMINAL;
    l2e_dma[l2e_toggle].len= len;
    l2e_dma[l2e_toggle].lar= (unsigned int)buf;
    l2e_dma[l2e_toggle].eah= 0;
    l2e_dma[l2e_toggle].eal= (unsigned int)dest_addr;
    l2e_dma[l2e_toggle].next= (unsigned int) &(l2e_dma[1 - l2e_toggle]) | DMA_L2E | DMA_WAKE;

    GM_STBAR();
    PULSE= 2;
    ISR_BARRIER(PULSE);
    l2e_toggle= 1 - l2e_toggle;

}  /* end of l2e_dma_start() */

/******************************************************************************/
/*
** e2l_dma_start()
** Initiate a transfer from the host to a buffer in shared memory.
*/
extern __inline__ void
e2l_dma_start(int *buf, int len, unsigned int src_addr)
{

    fail_if (a17, len <= 0);
    fail_if (a18, (unsigned)buf & ALIGN_32b);
    fail_if (a101, src_addr & ALIGN_32b);

    #ifdef rrr
    /* We may have to pad length as well */
    len= (len + ALIGN_32b) & ~ALIGN_32b;

    fail_if (a20, (len & ALIGN_32b));
    fail_if (a82, len > DBL_BUF_SIZE);
    #endif /* rrr */

    e2l_dma[1 - e2l_toggle].next= 0 | DMA_E2L | DMA_TERMINAL;
    e2l_dma[e2l_toggle].len= len;
    e2l_dma[e2l_toggle].lar= (unsigned int)buf;
    e2l_dma[e2l_toggle].eah= 0;
    e2l_dma[e2l_toggle].eal= (unsigned int)src_addr;
    e2l_dma[e2l_toggle].next= (unsigned int) &(e2l_dma[1 - e2l_toggle]) | DMA_E2L | DMA_WAKE;

    GM_STBAR();
    PULSE= 4;
    ISR_BARRIER(PULSE);
    e2l_toggle= 1 - e2l_toggle;

}  /* end of e2l_dma_start() */

/******************************************************************************/

extern __inline__ int
l2e_dma_done(void)
{

    GM_STBAR();
    return l2e_dma[1 - l2e_toggle].next & DMA_TERMINAL;

}  /* end of l2e_dma_done() */

/******************************************************************************/
extern __inline__ int
e2l_dma_done(void)
{

    GM_STBAR();
    return e2l_dma[1 - e2l_toggle].next & DMA_TERMINAL;

}  /* end of e2l_dma_done() */

/******************************************************************************/
/*
** Write an integer into the host memory (have to use DMA!)
*/
extern __inline__ void
write_int(int *addr, int value)
{

    l2e_dma[1 - l2e_toggle].next= 0 | DMA_L2E | DMA_TERMINAL;
    l2e_dma[l2e_toggle].len= sizeof(int);
    l2e_dma[l2e_toggle].lar= (unsigned int)&value;
    l2e_dma[l2e_toggle].eah= 0;
    l2e_dma[l2e_toggle].eal= (unsigned int)addr;
    l2e_dma[l2e_toggle].next= (unsigned int) &(l2e_dma[1 - l2e_toggle]) | DMA_L2E | DMA_WAKE;

    GM_STBAR();
    PULSE= 2;
    ISR_BARRIER(PULSE);
    l2e_toggle= 1 - l2e_toggle;

    while (!(l2e_dma[1 - l2e_toggle].next & DMA_TERMINAL))
	/* wait */
	;

}  /* end of write_int() */

/******************************************************************************/
/*
** is_pci_bus_avail()
*/
extern __inline__ int
is_pci_bus_avail(void)
{

    return PCIavailable;

}  /* end of is_pci_bus_avail() */

/******************************************************************************/
/*
** grab_pci_bus()
** Mark it in use.
*/
extern __inline__ void
grab_pci_bus(void)
{

    fail_if (a48, (PCIavailable == FALSE));
    PCIavailable= FALSE;

}  /* end of grab_pci_bus() */

/******************************************************************************/
/*
** free_pci_bus()
** Mark it available.
*/
extern __inline__ void
free_pci_bus(void)
{

    fail_if (a49, (PCIavailable == TRUE));
    PCIavailable= TRUE;

}  /* end of free_pci_bus() */

/******************************************************************************/

#endif /* EBUS_7_H */
