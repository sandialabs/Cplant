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
** $Id: ebus.h,v 1.11 2001/08/29 16:22:27 pumatst Exp $
** This file contains common functions that deal with the EBUS side; i.e.
** functions to DMA to and from the host.
*/

#ifndef EBUS_H
#define EBUS_H

#include "lanai4_limits.h"	/* For CHAR_BIT */
#include "timer.h"		/* For world_time_t in int_hst() */
#include "init.h"		/* For fault() */
#include "MCP.h"		/* For assertion numbers */
#include "MCPshmem.h"		/* For ALIGN_32b, DBL_BUF_SIZE, etc. */
#include "lanai_def.h"		/* For DMA_DIR, ISR, etc. */
#include "integrity-enum.h"


/******************************************************************************/
/*
** Calculate offset into hstshmem data structure. Used to call hstshmem_read*()
** and hstshmem_write*().
*/
#define FIELD_OFFSET(field)	((unsigned)&(hstshmem->field) - \
				    (unsigned)&(hstshmem->first))

/******************************************************************************/
/*
** On LANai 4 we have to arbitrate access to the PCI bus. LANai 7
** does it in hardware.
*/
extern int PCIavailable;

extern __inline__ int  is_pci_bus_avail(void);
extern __inline__ void grab_pci_bus(void);
extern __inline__ void free_pci_bus(void);

/******************************************************************************/

extern __inline__ int  ntoh(unsigned int in);
extern __inline__ void write_int(int *addr, int value);
extern __inline__ void hstshmem_write(unsigned offset, int value);
extern __inline__ void l2e_dma_start(int *buf, int len, unsigned int dest_addr);
extern __inline__ void e2l_dma_start(int *buf, int len, unsigned int src_addr);
extern __inline__ int  l2e_dma_done(void);
extern __inline__ int  e2l_dma_done(void);
extern __inline__ void int_hst(int int_type);

extern void ebus_dma_init(void);
extern int ebus_dma_test(int type);

int dma_integrity_test(void);

/******************************************************************************/
/*
** Functions that differ between LANai 4.x and 7.x
*/
#if defined (L4)
    #include "ebus.4.h"
#endif
#if defined (L7) || defined (L9)
    #include "ebus.7.h"
#endif


/******************************************************************************/
/*
** Functions that are the same among LANai 4.x and 7.x
*/
/******************************************************************************/
/*
** Convert from network (LANai) representation to host representation (Alpha)
*/
extern __inline__ int
ntoh(unsigned int in)
{

    return (((in << (3 * CHAR_BIT)) & 0xff000000) |
	    ((in << (1 * CHAR_BIT)) & 0x00ff0000) |
	    ((in >> (1 * CHAR_BIT)) & 0x0000ff00) |
	    ((in >> (3 * CHAR_BIT)) & 0x000000ff));

}  /* end of ntoh() */

/******************************************************************************/

extern __inline__ void
hstshmem_write(unsigned offset, int value)
{

int *addr;

    addr= (int *)((unsigned)mcpshmem->hstshmem + offset);
    write_int(addr, value);

}  /* end of hstshmem_write() */

/******************************************************************************/
/*
** int_hst()
** Send an interrupt to the host to let it know a message is comming in.
** The current MCP can generate an interrupt at about every 15 microseconds
** if bombarded with short packets. That's too much, even for a 500MHz Alpha.
** So, throttle it a little. Don't interrupt more often than every INTERVAL
** RTC clock ticks. (Nothing will get lost, since the interrupt handler will
** consume several packets, if they are available, and we interrupt again
** if the host doesn't consume our packets.)
*/

#define INTERVAL		(50)

extern __inline__ void
int_hst(int int_type)
{

static world_time_t next= {0,0};


    mcpshmem->LANai2host |= int_type;
    if (checkAlarm(&next))   {
	SET_HOST_SIG_BIT();
	setAlarm(&next, INTERVAL);
    }

}  /* end of int_hst() */

/******************************************************************************/
/*
** Fill a buffer with test data
*/
extern __inline__ int
fill_data(int *buf, int len)
{

int i;
int *ptr;


    ptr= buf;
    for (i= 0; i < (len / sizeof(int)); i++)   {
	*(ptr++)= 0xcccc3333 ^ ((i << 16) | i);
    }
    return TRUE;

}  /* end of fill_data() */

/******************************************************************************/
/*
** Verify that the data we got is what we expected
*/
extern __inline__ int
check_data(int *buf, int len)
{

int i;
int *ptr;


    ptr= buf;
    for (i= 0; i < (len / sizeof(int)); i++)   {
	if ( (*(ptr++)) != (0x5555aaaa ^ ((i << 16) | i)))   {
	    return FALSE;
	}
    }
    return TRUE;

}  /* end of check_data() */

/******************************************************************************/
extern __inline__ void
e2l_dma_test(int len, int tst_num)
{

int i;
unsigned int start, end;


    mcpshmem->DMA_test_buf= 0;
    mcpshmem->DMA_test_len= len;
    mcpshmem->DMA_test_result= 0;
    mcpshmem->LANai2host= INT_DMA_E2L_TEST;
    SET_HOST_SIG_BIT();

    /* Wait for the host to do setup a buffer we can pull in */
    while (mcpshmem->DMA_test_buf == 0)   {
    }

    /* Now go get it and measure how long it takes */
    start= RTC;
    for (i= 0; i < DMA_TEST_CNT; i++)   {
	e2l_dma_start(mcpshmem->rcv_buf_A, len, mcpshmem->DMA_test_buf);
	while (!e2l_dma_done())   {
	    /* wait */
	}
    }
    end= RTC;


    if (check_data(mcpshmem->rcv_buf_A, len))   {
	mcpshmem->DMA_test_result= end - start;
    } else   {
	mcpshmem->DMA_test_result= 1;
    }

    while (mcpshmem->DMA_test_result != 0)   {
	/* Wait for host ack */
    }
    while ( !(EIMR & HOST_SIG_BIT))   {
    }

    mcpshmem->e2l_len[tst_num]= len;
    mcpshmem->e2l_result[tst_num]= end - start;

}  /* end of e2l_dma_test() */

/******************************************************************************/

extern __inline__ void
l2e_dma_test(int len, int tst_num)
{

register int i;
unsigned int start, end;


    mcpshmem->DMA_test_buf= 0;
    mcpshmem->DMA_test_len= len;
    mcpshmem->DMA_test_result= 0;
    mcpshmem->LANai2host= INT_DMA_L2E_TEST;
    SET_HOST_SIG_BIT();

    fill_data(mcpshmem->rcv_buf_A, len);

    /* Wait for the host to do setup a buffer we can pull in */
    while (mcpshmem->DMA_test_buf == 0)   {
    }

    /* Now go get it and measure how long it takes */
    start= RTC;
    for (i= 0; i < DMA_TEST_CNT; i++)   {
	l2e_dma_start(mcpshmem->rcv_buf_A, len, mcpshmem->DMA_test_buf);
	while (!l2e_dma_done())   {
	    /* wait */
	}
    }
    end= RTC;

    mcpshmem->DMA_test_result= end - start;

    while (mcpshmem->DMA_test_result != 0)   {
	/* Wait for host ack */
    }
    while ( !(EIMR & HOST_SIG_BIT))   {
    }

    if (mcpshmem->DMA_test_len != 0)   {
	/* There were errors on the host side */
	mcpshmem->l2e_len[tst_num]= len;
	mcpshmem->l2e_result[tst_num]= 0;
    } else   {
	mcpshmem->l2e_len[tst_num]= len;
	mcpshmem->l2e_result[tst_num]= end - start;
    }
}  /* end of l2e_dma_test() */

/******************************************************************************/

extern __inline__ int
ebus_dma_test(int type)
{

    /*
    ** Always do the SANITY test to get the DMA transfer numbers
    ** displayed by mcpstat -bench
    */

    /* ************************* Host to LANai tests ********************* */
    e2l_dma_test(16 * 1024, 0);
    e2l_dma_test( 8 * 1024, 1);
    e2l_dma_test( 4 * 1024, 2);
    e2l_dma_test(     1024, 3);
    e2l_dma_test(      512, 4);
    e2l_dma_test(      256, 5);
    e2l_dma_test(      128, 6);
    e2l_dma_test(        4, 7);

    /* ************************* LANai to Host tests ********************* */
    l2e_dma_test(16 * 1024, 0);
    l2e_dma_test( 8 * 1024, 1);
    l2e_dma_test( 4 * 1024, 2);
    l2e_dma_test(     1024, 3);
    l2e_dma_test(      512, 4);
    l2e_dma_test(      256, 5);
    l2e_dma_test(      128, 6);
    l2e_dma_test(        4, 7);

    if (type == INTEGRITY)   {
	return dma_integrity_test();
    }
    return 0;

}  /* end of ebus_dma_test() */

/******************************************************************************/

#endif /* EBUS_H */
