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
** $Id: Pkt_handler.c,v 1.26.2.1 2002/03/15 01:00:04 jbogden Exp $
** This file contains an interrupt handler for our packet MCP
*/

#define VERBOSE
#undef VERBOSE
#define VERY_VERBOSE
#undef VERY_VERBOSE

#undef CONFIG_PORTALS
#include <asm/io.h>		/* Import virt_to_bus() */
#include <linux/kernel.h>
#include <linux/sched.h>	/* Import task_struct */
#include <asm/byteorder.h>
#include <sys/defines.h>
#include "lanai_device.h"
#include "MCPshmem.h"
#include "MCPSHMEM_ADDR.h"	/* For MCPSHMEM_ADDR */
#include "integrity.h"
#include "hstshmem.h"
#include "arch_asm.h"		/* Import mb() and wmb() */
#include "Pkt_handler.h"
#include "Pkt_send.h"		/* Import pkt_init_send() */
#include "Pkt_recv.h"		/* Import pkt_init_recv() Pkt_rcvLANai_start()*/
#include "Pkt_proc.h"
#include "printf.h"         /* for PRINTF() macro */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif


/******************************************************************************//*
** Global mcpshmem pointer
*/
mcpshmem_t *mcpshmem= NULL;

static int int_err=0;

/******************************************************************************/
/*
** Local functions
*/
static void report_MCP_fault(void);
static void report_MCP_warning(unsigned int warning, unsigned int lastrcv,
		unsigned int lastsnd);
static void setupDMA(int unit);
static void dma_e2l_test(void);
static void dma_l2e_test(void);
static void dma_l2e_integrity_test(int itest, int iter);
static void dma_e2l_integrity_test(int itest, int iter);


/******************************************************************************/
/*
** myrpkt_handle_interrupt()
** This is the function that gets called when the LANai causes an
** interrupt on the host.
*/
void
myrpkt_handle_interrupt(int unit, struct NETDEV *dev, int mcp_type)
{

int *himem= NULL;
int initial_isr;
int escape_cnt;
int reason;
unsigned int warning;


    /* Turn LANai interrupts off */
    mb();
    setEIMR(unit, mcp_type, 0);
    #ifndef NO_STATS
    proc_stat->rcv_ints++;
    #endif /* NO_STATS */
    initial_isr= getISR(unit, mcp_type);

    #ifdef VERY_VERBOSE
	printk("In myrpkt_handle_interrupt(unit %d)\n", unit);
    #endif /* VERY_VERBOSE */


    if (initial_isr & HOST_SIG_BIT)   {
	#ifdef VERY_VERBOSE
	    printk("HOST_SIG_BIT is set: Interrupt caused by LANai %d\n", unit);
	#endif /* VERY_VERBOSE */
    } else   {
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_ints_not_mcp++;
    #endif /* NO_ERROR_STATS */
	#ifdef VERBOSE
	    printk("### HOST_SIG_BIT not set 0x%08x, not handling interrupt\n",
		initial_isr);
	#endif /* VERBOSE */

	/* Turn LANai interrupts back on */
	setEIMR(unit, mcp_type, HOST_SIG_BIT);
	return;
    }

    if (mcpshmem == NULL)   {
	himem= (int *)(&LANAI[unit][MCPSHMEM_ADDR >> 1]);
	mcpshmem= (mcpshmem_t *)(&LANAI[unit][ntohl(*himem) >> 1]);
    }

    reason= ntohl(mcpshmem->LANai2host);
    mcpshmem->LANai2host= htonl(NO_REASON);


    #ifdef VERY_VERBOSE
	printk("himem is %p\n", (void *)himem);
	printk("mcpshmem is %p\n", (void *)mcpshmem);
	printk("MCP version %d\n", (int)ntohl(mcpshmem->version));
	printk("MCP ID %x\n", (int)ntohl(mcpshmem->ID));
	printk("interrupt type %d\n", reason);
    #endif /* VERY_VERBOSE */


    if (reason & INT_RCV_BEGUN)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_rcv++;
    #endif /* NO_STATS */
	Pkt_rcvLANai_start(dev);
    }
    if (reason & INT_MCP_FAULT)   {
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_ints_fault++;
    #endif /* NO_ERROR_STATS */
	report_MCP_fault();
    }
    if (reason & INT_MCP_WARNING)   {
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_ints_warning++;
    #endif /* NO_ERROR_STATS */
	warning= mcpshmem->warn0;
	mcpshmem->warn0= 0;
	report_MCP_warning(warning, mcpshmem->warn1, mcpshmem->warn2);
    }
    if (reason & INT_DMA_SETUP)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_dma_setup++;
    #endif /* NO_STATS */
	setupDMA(unit);
    }
    if (reason & INT_DMA_E2L_TEST)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_dma_e2l_test++;
    #endif /* NO_STATS */
	dma_e2l_test();
    }
    if (reason & INT_DMA_L2E_TEST)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_dma_l2e_test++;
    #endif /* NO_ERROR_STATS */
	dma_l2e_test();
    }
    if (reason & INT_DMA_E2L_INTEGRITY_TEST)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_dma_e2l_integrity++;
    #endif /* NO_STATS */
	dma_e2l_integrity_test(ntohl(mcpshmem->DMA_test_index), 
                               ntohl(mcpshmem->DMA_test_iter));
    }
    if (reason & INT_DMA_L2E_INTEGRITY_TEST)   {
    #ifndef NO_STATS
	proc_stat->rcv_ints_dma_l2e_integrity++;
    #endif /* NO_STATS */
	dma_l2e_integrity_test(ntohl(mcpshmem->DMA_test_index),
                               ntohl(mcpshmem->DMA_test_iter));
    }
    if (reason & GET_HSTSHMEM)   {
	/*
	** Since this is caused by a new MCP load, then mcpshmem might
	** have moved. Better go get it again.
	*/
	himem= (int *)(&LANAI[unit][MCPSHMEM_ADDR >> 1]);
	mcpshmem= (mcpshmem_t *)(&LANAI[unit][ntohl(*himem) >> 1]);

    #ifndef NO_STATS
	proc_stat->rcv_ints_init++;
    #endif /* NO_STATS */
    
	if (mcpshmem->hstshmem == 0)   {
	    /*
	    ** This is done during the MCP initialization to let the MCP
	    ** know where to find the shared memory on the host side.
	    */
        #ifdef VERBOSE
	    printk("Got GET_HSTSHMEM interrupt\n");
        #endif /* VERBOSE */
        
	    if (pkt_init_send() != 0)   {
		printk("Send buffer initialization problem\n");
	    } else if (pkt_init_recv() != 0)   {
		printk("Recv buffer initialization problem\n");
	    } else   {
		hstshmem->rdy_to_snd= htonl(FALSE);
		mcpshmem->hstshmem= htonl(virt_to_bus(hstshmem));
		escape_cnt= 0;
		while ((unsigned)hstshmem->rdy_to_snd == (unsigned)htonl(FALSE)){
		    if (++escape_cnt > 5000000)   {
			break;
		    }
		}
		if ((unsigned)hstshmem->rdy_to_snd != (unsigned)htonl(TRUE))   {
		    printk("MCP not running. Did not get rdy_to_snd!\n");
		} else   {
            #if VERBOSE
		    printk("MCP up and running as phys nid %d (%d on MCP)\n",
			hstshmem->my_pnid, (int)ntohl(mcpshmem->my_pnid));
            #endif /* VERBOSE */
		    mcpshmem->mod_type= hstshmem->mod_type;            
		}
	    }
	} else   {
	    #ifdef VERY_VERBOSE
		printk("### Invalid GET_HSTSHMEM interrupt from LANai!\n");
	    #endif /* VERY_VERBOSE */
	}
    }


    if (reason == NO_REASON)   {
	static unsigned int cnt= 0;
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_ints_no_reason++;
    #endif /* NO_ERROR_STATS */
	if ((cnt++ % 500000) == 0)   {
	    #ifdef VERBOSE
		printk("### No reason interrupt %d from LANai (%d times)\n",
		    reason, cnt - 1);
	    #endif /* VERBOSE */
	}
    }
    if (reason & ~(INT_MCP_FAULT | INT_RCV_BEGUN | GET_HSTSHMEM | NO_REASON |
	    INT_DMA_SETUP | INT_DMA_E2L_TEST | INT_DMA_L2E_TEST |
	    INT_DMA_L2E_INTEGRITY_TEST | INT_DMA_E2L_INTEGRITY_TEST |
	    INT_MCP_WARNING) )  {
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_ints_unknown++;
    #endif /* NO_ERROR_STATS */
	#ifdef VERBOSE
	    printk("### Unknown reason for interrupt %d (0x%04x) from LANai\n",
		reason, reason);
	#endif /* VERBOSE */
    }


    /* Turn LANai interrupts back on */
    setISR(unit, mcp_type, HOST_SIG_BIT);
    setEIMR(unit, mcp_type, HOST_SIG_BIT);
    wmb();

    #ifdef VERY_VERBOSE
	printk("Leaving myrpkt_handle_interrupt(unit %d)\n", unit);
    #endif /* VERY_VERBOSE */

}  /* end of myrpkt_handle_interrupt() */

/******************************************************************************/
/*
** We printk a little information here. It will go into /var/log/messages.
** A script on sss0 can monitor that file and send out mail when a fault
** occurs. The script could also reload the MCP.
*/
static void
report_MCP_fault(void)
{
    printk("===+ MCP has faulted! Help me! +===\n");
    printk("Node %d: S%02d R%02d, fault %d, isr 0x%08x\n",
	(int)ntohl(mcpshmem->my_pnid),
	(int)ntohl(mcpshmem->snd_state) - 1,
	(int)ntohl(mcpshmem->rcv_state) - 1,
	(int)ntohl(mcpshmem->fault),
	(int)ntohl(mcpshmem->fault_isr));

}  /* end of report_MCP_fault() */

/******************************************************************************/

static void
report_MCP_warning(unsigned int warning, unsigned int lastrcv,
	unsigned int lastsnd)
{

    warning= ntohl(warning);
    lastrcv= ntohl(lastrcv);
    lastsnd= ntohl(lastsnd);

    #ifdef PRINT_WARNINGS
    printk("===+ MCP Warning (0x%04x): last rcv start %d RTC ticks, last snd "
	"start %d ticks\n", warning, lastrcv, lastsnd);

    if (warning & WARN_NRESRCV)   {
	printk("===+ NRES during receive\n");
    }
    if (warning & WARN_PARRCV)   {
	printk("===+ Parity error during receive\n");
    }
    if (warning & WARN_LINK2RCV)   {
	printk("===+ Link2 error during receive\n");
    }
    if (warning & WARN_LINK2SND)   {
	printk("===+ Link2 error during send\n");
    }
    if (warning & WARN_SNDTIMEOUT)   {
	printk("===+ Send timeout\n");
    }
    #endif /* PRINT_WARNINGS */

}  /* end of report_MCP_warning() */

/******************************************************************************/
/*
** The MCP knows where it wants DMA chains to be in LANai memory. However,
** it seems it cannot put the start address of these chains into the DMA
** controller. The host has to do it. So, we pull the addresses the MCP
** put into the shared memory region, and move them into the DMA controller.
*/
static void
setupDMA(int unit)
{
    LANAI_BOARD[unit][0x00800100 / 4] = ntohl(mcpshmem->DMAchannel0);
    LANAI_BOARD[unit][0x00800108 / 4] = ntohl(mcpshmem->DMAchannel2);

    /* Tell the LANai we've done it. */
    mcpshmem->DMAchannel0= 0;

}  /* end of setupDMA() */

/******************************************************************************/

static void
dma_e2l_test(void)
{

int escape_cnt;
int i;
unsigned int *ptr;


    if (dmatst_area == NULL)   {
	printk("dma_e2l_test() dmatst_area is NULL!\n");
	return;
    }

    if (ntohl(mcpshmem->DMA_test_len) > DMATST_AREA_SIZE)   {
	printk("dma_e2l_test() test len %u > buf size %d\n",
	    ntohl(mcpshmem->DMA_test_len), DMATST_AREA_SIZE);
	return;
    }

    /* Fill the buffer with a data pattern */
    ptr= dmatst_area;
    for (i= 0; i < (ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
	*ptr++ = htonl(0x5555aaaa ^ ((i << 16) | i));
    }

    /* Tell the MCP it is ready */
    mcpshmem->DMA_test_buf= (unsigned int)htonl(virt_to_bus(dmatst_area));

    escape_cnt= 0;
    while (mcpshmem->DMA_test_result == 0)   {
	/* wait */
	if (++escape_cnt > 5000000)   {
	    printk("dma_e2l_test() Can't wait for DMA test any longer\n");
	    return;
	}
    }

    #ifdef VERBOSE
    if (ntohl(mcpshmem->DMA_test_result) == 1)   {
	printk("dma_e2l_test() host to LANai DMA of %d * %ld bytes failed\n",
	    DMA_TEST_CNT, ntohl(mcpshmem->DMA_test_len));
    } else   {
	printk("dma_e2l_test() host to LANai DMA of %d * %ld bytes took %ld "
	    "ticks\n", DMA_TEST_CNT, ntohl(mcpshmem->DMA_test_len),
	    ntohl(mcpshmem->DMA_test_result));
    }
    #endif /* VERBOSE */
    mcpshmem->DMA_test_result= 0;

}  /* end of dma_e2l_test() */

/******************************************************************************/
static void
dma_e2l_integrity_test(int itest, int iter)
{

int escape_cnt;
int i, result, done;
unsigned int *ptr;
const char* bstring;

    if (dmatst_area == NULL)   {
	printk("dma_e2l_integrity_test(): dmatst_area is NULL!\n");
	return;
    }

    if (ntohl(mcpshmem->DMA_test_len) > DMATST_AREA_SIZE)   {
	printk("dma_e2l_integrity_test(): test len %u > buf size %d\n",
	    ntohl(mcpshmem->DMA_test_len), DMATST_AREA_SIZE);
	return;
    }

    bstring = stringib[ntohl(mcpshmem->DMA_buf_id)];
#if 0
    printk("dma_e2l_integrity_test(): doing %s, lanai_buf= %s\n", stringp[itest], bstring);
#endif

    /* Fill the buffer with a data pattern */
    ptr= dmatst_area;
    for (i=0; i<(ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
      *ptr++ = htonl(pattern[itest]);
    }

    /* Tell the MCP it is ready */
    mcpshmem->DMA_test_buf= (unsigned int)htonl(virt_to_bus(dmatst_area));

    /* wait for lanai to check the result */
    escape_cnt= 0;
    while (mcpshmem->DMA_test_result == 0)   {
      /* wait */
      if (++escape_cnt > 5000000)   {
	printk("dma_e2l_integrity_test() Can't wait for DMA test any longer\n");
	  return;
      }
    }

    result = (int) ntohl(mcpshmem->DMA_test_result);
    done   = (int) ntohl(mcpshmem->DMA_test_done);

    if ( result == 1 || done ) {
      if ( result == 1 ) {
        printk("dma_e2l_integrity_test():!! FAILED iteration %d !!\n", iter);
      } 
      else {
        printk("-------------------------------------------------------\n");
        printk("dma_integrity_test: PASSED iteration %d\n", iter);
        printk("-------------------------------------------------------\n");
      }
    }
    mcpshmem->DMA_test_result= 0;

}  /* end of dma_e2l_integrity_test() */

/******************************************************************************/

/******************************************************************************/
static void
dma_l2e_test(void)
{

int escape_cnt;
int i, err;
unsigned int *ptr;

    if (dmatst_area == NULL)   {
	printk("dma_l2e_test() dmatst_area is NULL!\n");
	return;
    }

    if (ntohl(mcpshmem->DMA_test_len) > DMATST_AREA_SIZE)   {
	printk("dma_l2e_test() test len %u > buf size %d\n",
	    ntohl(mcpshmem->DMA_test_len), DMATST_AREA_SIZE);
	return;
    }

    /* Fill the buffer with zero's */
    ptr= dmatst_area;
    for (i= 0; i < (ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
	*ptr++ = 0;
    }

    /* Tell the MCP it is ready */
    mcpshmem->DMA_test_buf= (unsigned int)htonl(virt_to_bus(dmatst_area));


    escape_cnt= 0;
    while (mcpshmem->DMA_test_result == 0)   {
	/* wait */
	if (++escape_cnt > 5000000)   {
	    printk("dma_l2e_test() Can't wait for DMA test any longer\n");
	    return;
	}
    }

    ptr= dmatst_area;
    err= 0;
    for (i= 0; i < (ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
	if ( *(ptr++) != htonl(0xcccc3333 ^ ((i << 16) | i)))   {
	    err++;
	}
    }

    if ( err ) {
	printk("dma_l2e_test(): failed SANITY dma test for %u bytes\n",
	    ntohl(mcpshmem->DMA_test_len));
    }
    else {
    #ifdef VERBOSE
	printk("dma_l2e_test(): passed SANITY dma test for %ld bytes\n",
	    ntohl(mcpshmem->DMA_test_len));
    #endif /* VERBOSE */
    }

    #ifdef VERBOSE
    printk("dma_l2e_test() LANai to host DMA of %d * %ld bytes took %ld "
	"ticks with %d errors\n", DMA_TEST_CNT, ntohl(mcpshmem->DMA_test_len),
	ntohl(mcpshmem->DMA_test_result), err);
    #endif /* VERBOSE */
    
    mcpshmem->DMA_test_len= htonl(err);
    mcpshmem->DMA_test_result= 0;

}  /* end of dma_l2e_test() */

/******************************************************************************/

static void
dma_l2e_integrity_test(int itest, int iter) {

int escape_cnt;
int i;
unsigned int *ptr;
const char* bstring;

    if (dmatst_area == NULL)   {
	printk("dma_l2e_integrity_test(): dmatst_area is NULL!\n");
	return;
    }

    if (ntohl(mcpshmem->DMA_test_len) > DMATST_AREA_SIZE)   {
	printk("dma_l2e_integrity_test(): test len %u > buf size %d\n",
	    ntohl(mcpshmem->DMA_test_len), DMATST_AREA_SIZE);
	return;
    }

    bstring = stringib[ntohl(mcpshmem->DMA_buf_id)];
#if 0
    printk("dma_l2e_integrity_test(): doing %s, lanai_buf= %s\n", stringp[itest], bstring);
#endif

    /* Fill the buffer with zero's */
    ptr= dmatst_area;
    for (i= 0; i < (ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
       *ptr++ = 0;
    }

    /* Tell the MCP it is ready */
    mcpshmem->DMA_test_buf= (unsigned int)htonl(virt_to_bus(dmatst_area));

    escape_cnt= 0;
    while (mcpshmem->DMA_test_result == 0)   {
	/* wait */
	if (++escape_cnt > 5000000)   {
	    printk("dma_l2e_integrity_test(): Can't wait for DMA test any longer\n");
	    return;
	}
    }

    ptr= dmatst_area;
    mb();
    for (i= 0; i < (ntohl(mcpshmem->DMA_test_len) / sizeof(int)); i++)   {
      if ( *(ptr) != htonl( pattern[itest] ) ) {
        printk("dma_l2e_integrity_test(): DMAed word 0x%x != %s\n", *ptr, stringp[itest]);
	int_err++;
        break;
      }
      ptr++;
    }

    /* overall test is done if we got an error this trial 
       (so notify mcp), or if mcp says so...
    */

    mcpshmem->DMA_test_len= htonl(int_err);

    if ( int_err || ntohl(mcpshmem->DMA_test_done) ) {
      if ( int_err ) {
        printk("dma_l2e_integrity_test():!! FAILED iteration %d !!\n", iter);
      }
      else {
        printk("--------------------------------------------------------\n");
        printk("dma_integrity_test: PASSED iteration %d\n", iter);
        printk("--------------------------------------------------------\n");
      }
      int_err = 0;
    }

    mcpshmem->DMA_test_result= 0;

}  /* end of dma_l2e_integrity_test() */

/******************************************************************************/
