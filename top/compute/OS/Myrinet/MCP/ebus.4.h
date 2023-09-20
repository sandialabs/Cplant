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
** $Id: ebus.4.h,v 1.6 2002/02/14 18:38:12 jbogden Exp $
**
** This file contains functions that deal with the EBUS side; i.e.
** functions to DMA to and from the host, that are specific to LANai 4.x
*/

#ifndef EBUS_4_H
#define EBUS_4_H


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
    fail_if (a81, len > DBL_BUF_SIZE);
#endif /* rrr */
    len= (len + ALIGN_32b) & ~ALIGN_32b;

    EAR= (int *)dest_addr;
    LAR= buf;
    DMA_DIR= 0;
    GM_STBAR();
    DMA_CTR= len;

}  /* end of l2e_dma_start() */

/******************************************************************************/
/*
** e2l_dma_start()
** Initiate a transfer from the host to a buffer in shared memory. We
** wont align the buffer.
*/
extern __inline__ void
e2l_dma_start(int *buf, int len, unsigned int src_addr)
{

    fail_if (a17, len <= 0);
    fail_if (a18, (unsigned)buf & ALIGN_32b);
    fail_if (a101, src_addr & ALIGN_32b);

    /* We may have to pad length as well */
    len= (len + ALIGN_32b) & ~ALIGN_32b;

    fail_if (a20, (len & ALIGN_32b));
#ifdef rrr
    fail_if (a82, len > DBL_BUF_SIZE);
#endif /* rrr */
    DMA_DIR= 1;
    EAR= (int *)src_addr;
    LAR= buf;
    GM_STBAR();
    DMA_CTR= len;

}  /* end of e2l_dma_start() */

/******************************************************************************/

extern __inline__ int
l2e_dma_done(void)
{
    ISR_BARRIER(DMA_CTR);
    return (get_ISR() & DMA_INT_BIT);
}  /* end of e2l_dma_done() */

/******************************************************************************/

extern __inline__ int
e2l_dma_done(void)
{
    ISR_BARRIER(DMA_CTR);
    return (get_ISR() & DMA_INT_BIT);
}  /* end of e2l_dma_done() */

/******************************************************************************/
/*
** Write an integer into the host memory (have to use DMA!)
*/
extern __inline__ void
write_int(int *addr, int value)
{

    fail_if (a53, ((unsigned)addr & ALIGN_32b));

    EAR= addr;
    LAR= &value;
    DMA_DIR= 0;
    GM_STBAR();
    DMA_CTR= sizeof(int);
    do   {
	ISR_BARRIER(DMA_CTR);
    } while (!(ISR & DMA_INT_BIT));

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

#endif /* EBUS_4_H */
