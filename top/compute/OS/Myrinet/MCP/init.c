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
** $Id: init.c,v 1.59.2.2 2002/05/08 20:39:01 jbogden Exp $
** Initialize the MCP
*/
#include "lanai4_limits.h"
#include "lanai_def.h"
#include "MCP.h"
#include "MCPshmem.h"
#include "init.h"
#include "route.h"
#include "ebus.h"	/* For ntoh, hstshmem_write (write_int) */
#include "hstshmem.h"

/*
** Local functions
*/
static void dark(int num, int delay);
static void bit0(void);
static void bit1(void);
static void morse(int num);
static void consume(void);
static void bench(void);

/*
** The data structure and a pointer to it that we use to share
** information with the host.
*/
mcpshmem_t __mcpshmem;
extern mcpshmem_t *mcpshmem;	/* Defined in crt0.S */

/*
** The following structure is here only for address offset calculations.
** It actually resides on the host, but it makes life easier if we have
** a copy here (that contains no data).
*/
static hstshmem_t *hstshmem;


/*
** Reserve some room for the stack. Make mcp_stack point to the end of it.
** mcp_stack is used in crt0.S to initialize the CPU.
*/
unsigned int mcp_stack_array[2 * 1024 / sizeof(unsigned int)];
unsigned int *mcp_stack= (unsigned int *)((char *)mcp_stack_array +
		sizeof(mcp_stack_array));


/******************************************************************************/
/*
** MCP_init()
** Initialize everything.
*/
void
MCP_init(int mcp_type)
{

register int i;
static char *check_str;
static int first_time= TRUE;
int myrinet_mask;


    /*
    ** We put the mcpshmem_t structure into shared memory, so the host
    ** can see it. Make sure it is 8 byte aligned, or the Alpha will
    ** choke.
    */
    mcpshmem= &__mcpshmem;
    if ((unsigned)mcpshmem & ALIGN_64b)   {
	mcpshmem= (mcpshmem_t *)(((unsigned)mcpshmem + ALIGN_64b) & ~ALIGN_64b);
    }

    /* Initialize the data structure */
    mcpshmem->ID= MCP_ID;
    #if defined (L9)
	mcpshmem->LANai_vers= LANai_9;
	check_str= MCP_9_CHECK_STR;
    #elif defined (L7)
	mcpshmem->LANai_vers= LANai_7;
	check_str= MCP_7_CHECK_STR;
    #elif defined (L4)
	mcpshmem->LANai_vers= LANai_4;
	check_str= MCP_4_CHECK_STR;
    #else
	mcpshmem->LANai_vers= LANai_unknown;
	#warning "L4, L7, or L9 should be defined"
    #endif
    mcpshmem->version= MCP_version;
    mcpshmem->mcp_type= mcp_type;
    mcpshmem->mod_type= 0;

    mcpshmem->reset_imr= get_IMR();
    mcpshmem->reset_isr= get_ISR();

    if (!first_time)   {
	/* The LANai reset! Record some information, and try to continue */
	mcpshmem->counters.reset++;
	return;
    }
    first_time= FALSE;


    /* Initialize the LANai */
    myrinet_mask= 0;

    #if defined (USE_32BIT_CRC)
	#if defined (L9) || defined (L7)
	    myrinet_mask |= CRC32_ENABLE_BIT;	/* enable CRC 32 */
	#else
	    #error "Can't do 32-bit CRC on LANai 4"
	#endif
    #else
	/* enable CRC 8, but not CRC 32 */
	#if !defined (L9)
	    myrinet_mask |= CRC_ENABLE_BIT;
	#endif
    #endif
    
    #if defined (L9)
	#if defined(DO_TIMEOUT_PROTOCOL) && !defined(LINUX24)
	    myrinet_mask |= RX_CRC8_ENABLE_BIT | TX_CRC8_ENABLE_BIT | WINDOW1;
	    /* myrinet_mask &= ~(TIMEOUT0 | TIMEOUT1);  1/16s network timeout */
        myrinet_mask &= ~(TIMEOUT1);     /* 1/4s network timeout */
	#else
	    myrinet_mask |= RX_CRC8_ENABLE_BIT | TX_CRC8_ENABLE_BIT;
	    myrinet_mask |= TIMEOUT0 | TIMEOUT1 | WINDOW1;
	#endif
      #endif

    MYRINET= myrinet_mask;
    #if defined(L4) 
	#if defined(DO_TIMEOUT_PROTOCOL) && !defined(LINUX24)
	    /* TIMEOUT= 0;		1/16s network timeout */
        TIMEOUT = 1;        /* 1/4 s network timeout */
	#else
	    TIMEOUT= 3;		/* 4 s network timeout */
	#endif
    #endif

    #if defined(L7)
	#if defined(DO_TIMEOUT_PROTOCOL) && !defined(X86_BILD) && !defined(LINUX24)
	    /* TIMEOUT= 9;      64ms network timeout */
        TIMEOUT = 7;        /* 256ms network timeout */
	#else
	    TIMEOUT= 3;		/* 4 s network timeout */
	#endif
    #endif

    #if !defined(L9) && !defined(L7)
	DMA_STS= 15;		/* Should probably be determined by the host */
    #endif
    LED= 1;			/* LED on means we're ready */
    RTC= 0;			/* init real time clock (it counts up) */
    IT= 0xffffffff;		/* init interrupt timer (it counts down) */
    #if defined (L4)
	VERSION= 3;		/* For LANai 4.1, 4.3, 4.3 */
    #endif

    while (RTC < 45000)   {
	/* Spec says to wait > 10ms for SAN link to settle in */
    }

    /* We don't have a pointer to the shared memory area on the host side yet */
    hstshmem= (hstshmem_t *)0l;
    mcpshmem->hstshmem= 0;


    /* Disable interrupts */
    set_IMR(0);			/* We poll for all our interrupts */
    EIMR= 0;			/* We'll let the host turn on host interrupts */
    set_ISR(0xffffffff);	/* Clear all status bits */


    /* Tell the host where mcpshmem points to */
    mcpshmem->my_pnid= -1;		/* Host will tell us this as an ack */
    mcpshmem->DMA_test_type = SANITY;   /* after possibly changing this */
    SMP= mcpshmem;			/* SMP is not used yet */

    /* Interrupts are still off. mcpload is spinning on the ISR */
    SET_HOST_SIG_BIT();			/* Tell host SMP is ready */
    while (mcpshmem->my_pnid < 0)	/* Now wait for host */
	;
    /* mcpload has turned interrupts on now */


    for (i= 0; i < MAX_RCV_PKT_ENTRIES; i++)   {
	mcpshmem->rcv_pkt_list[i].phys_addr= 0;
	mcpshmem->rcv_pkt_list[i].len= -1;
    }

    for (i= 0; i < MAX_SND_PKT_ENTRIES; i++)   {
	mcpshmem->snd_pkt_list[i].phys_addr= 0;
	mcpshmem->snd_pkt_list[i].len= 0;
	mcpshmem->snd_pkt_list[i].dest= -1;
	mcpshmem->snd_pkt_list[i].page_idx= -1;
    }

    /* Clear the debug fields */
    for (i= 0; i < MAX_DEBUG; i++)   {
	mcpshmem->debug[i]= 0;
    }


    /* Initialize the counters and other fields */
    memset(&(mcpshmem->counters), 0, sizeof(counters_t));
    mcpshmem->fault= NO_FAULT;
    mcpshmem->rcv_state= -1;
    mcpshmem->snd_state= -1;
    mcpshmem->num_rcv_pkts= 0;

    mcpshmem->DMAchannel0= 0;
    mcpshmem->DMAchannel2= 0;

    /* Clear the DMA test fields */
    mcpshmem->DMA_test_buf= 0;
    mcpshmem->DMA_test_len= 0;
    mcpshmem->DMA_test_result= 0;

    for (i= 0; i < DMA_TEST_MAX; i++)   {
	mcpshmem->e2l_len[i]= 0;
	mcpshmem->e2l_result[i]= 0;
	mcpshmem->l2e_len[i]= 0;
	mcpshmem->l2e_result[i]= 0;
    }

    world_time.t0= 0;
    world_time.t1= 0;

    #ifdef BENCH
	bench();
    #endif /* BENCH */

    route_init();
    ebus_dma_init();

    mcpshmem->DMA_test_failed = DMA_TEST_NOTDONE;

    /*
    ** Interrupt the host and let it tell us where the shared
    ** data structure in the host memory is.
    */
    mcpshmem->hstshmem= 0;
    mcpshmem->mod_type= 0;
    mcpshmem->LANai2host= GET_HSTSHMEM;
    SET_HOST_SIG_BIT();
    while (mcpshmem->hstshmem == 0)
	;

    /* Initialize hstshmem, and let the host know we got it */
    hstshmem_write(FIELD_OFFSET(my_pnid), ntoh(mcpshmem->my_pnid));
    hstshmem_write(FIELD_OFFSET(rdy_to_snd), TRUE);

    while (mcpshmem->mod_type == 0)   {
	/* Wait for rtscts module to ack my_pnid & rdy_to_snd */
    }
    while ( !(EIMR & HOST_SIG_BIT))   {
    }

    if ( ebus_dma_test(mcpshmem->DMA_test_type) < 0) {
      mcpshmem->DMA_test_failed = DMA_TEST_FAILED;
    }
    else {
      mcpshmem->DMA_test_failed = DMA_TEST_OK;
    }

    /* Do a self test and send a packet to ourselves */
    self_test();

}  /* end of MCP_init() */

/******************************************************************************/
/*
** self_test()
** Try to find out how well we are. At the very least, send a packet to
** ourselves. Build a packet (including the route) in the send buffer
** and then start a DMA. When we fire-up the receive state machine the
** packet should be there waiting for us.
*/
void
self_test(void)
{

int i;
int direct_connect;


    /*
    ** First, check the route table. If all routes start with 0x00; i.e.
    ** no route bytes, then there is probably no switch out there. So
    ** we wont send a route byte and send a direct connect message to
    ** the other interface.
    */
    direct_connect= TRUE;
    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	if (mcpshmem->route[i][0] != 0)   {
	    direct_connect= FALSE;
	    break;
	}
    }

    /*
    ** Build a packet of 8 bytes that looks like this:
    **     0x80 0x08 0x03 0x59 my_pnid
    ** The 0x80 tells the switch we're attached to to return the packet to us.
    ** The 0x08 0x03 is the MYRI_SELF_TEST_TYPE packet type ID.
    ** The 0x59 is just a filler for a full word.
    ** The last 4 bytes are my_pnid. This should match on the receive side ;-)
    */
    if (!direct_connect)   {
	mcpshmem->snd_buf_A[0]= (0x80 << 24) |
					    (MYRI_SELF_TEST_TYPE << 8) | 0x59;
	mcpshmem->snd_buf_A[1]= mcpshmem->my_pnid;
    } else   {
	mcpshmem->snd_buf_A[0]= (MYRI_SELF_DIRECT_TYPE << 16) | (0x59 << 8);
	mcpshmem->snd_buf_A[1]= mcpshmem->my_pnid;
    }

    /* We'd better be ready to send */
    SMP= mcpshmem->snd_buf_A;
    #if defined (L7) || defined (L9)
	SMLT= (int *)((int)mcpshmem->snd_buf_A + 8);
    #elif defined (L4)
	SMLT= (int *)((int)mcpshmem->snd_buf_A + 4);
    #endif

    /* Upon receipt of this packet we'll set self_test to OK */
    mcpshmem->self_test= SELF_TEST_UNKNOWN;

}  /* end of self_test() */

/******************************************************************************/

#define BIT16		(0x00010000)
#define BIT19		(0x00080000)
#define BIT20		(0x00100000)
#define BIT21		(0x00200000)
#define BIT16_MASK	(0x0001ffff)
#define BIT19_MASK	(0x000fffff)
#define BIT20_MASK	(0x001fffff)
#define BIT21_MASK	(0x003fffff)

/******************************************************************************/
/*
** Wait (and consume incomming bytes, if any) "num" miliseconds. We assume
** the RTC counts 500ns ticks.
*/
static void
wait_ms(int num)
{

int target_rtc;


    target_rtc= (RTC + (num * 2000)) & 0xffffff00;
    while ((RTC & 0xffffff00) != target_rtc)   {
	consume();
    }
}  /* end of wait_ms() */

/******************************************************************************/

void
flash(int num, int delay)
{

int i;

    for (i= 0; i < num; i++)   {
	LED= 1;
	wait_ms(delay);
	LED= 0;
	wait_ms(delay);
    }

}  /* end of flash() */

/******************************************************************************/

static void
dark(int num, int delay)
{

int i;


    LED= 0;
    for (i= 0; i < num; i++)   {
	wait_ms(delay);
    }

}  /* end of dark() */

/******************************************************************************/

static void
bit0(void)
{

    LED= 1;
    wait_ms(400);
    LED= 0;
    wait_ms(200);

}  /* end of bit0() */

/******************************************************************************/

static void
bit1(void)
{

    LED= 1;
    wait_ms(2000);
    LED= 0;
    wait_ms(200);

}  /* end of bit1() */

/******************************************************************************/

static void
morse(int num)
{

int bit, i;


    /* We do 1 byte, LSB first */
    bit= 1;
    for (i= 0; i < CHAR_BIT; i++)   {
	if (num & bit)   {
	    bit1();
	} else   {
	    bit0();
	}
	bit= bit << 1;
    }

}  /* end of morse() */

/******************************************************************************/
/*
** Consume any incomming packets
*/
static void
consume(void)
{

    #if defined (L7) || defined (L9)
	#warning "*** consume function dump count is only approximate"
	if ((get_ISR() & RECV_INT_BIT) || (get_ISR() & BUFF_INT_BIT))   {
	    mcpshmem->counters.dumped_num += (unsigned int)RMP -
			(unsigned int)mcpshmem->rcv_buf_A - 8;
	    RMP= mcpshmem->rcv_buf_A;
	    RML= (int *)((int)mcpshmem->rcv_buf_A + DBL_BUF_SIZE - 4);
	}
    #elif defined (L4)
	if (get_ISR() & BYTE_RDY_BIT)   {
	    int byte;

	    mcpshmem->counters.dumped_num++;
	    byte= RB;
	    touch(ISR);
	    if (get_ISR() & TAIL_INT_BIT)   {
		mcpshmem->counters.dumped++;
	    }
	}
    #endif

}  /* end of consume() */

/******************************************************************************/

void
fault(int num, unsigned int param1, unsigned int param2)
{

    mcpshmem->fault= num;
    mcpshmem->fault_isr= get_ISR();
    mcpshmem->debug[0] = param1;
    mcpshmem->debug[1] = param2;
    #if defined (L7) || defined (L9)
	RMP= mcpshmem->rcv_buf_A;
	RML= (int *)((int)mcpshmem->rcv_buf_A + DBL_BUF_SIZE - 4);
    #endif

    if (mcpshmem->hstshmem)   {
	/* Tell the host! */
	mcpshmem->LANai2host= INT_MCP_FAULT;
	SET_HOST_SIG_BIT();
    }

    while(1)   {
	flash(100, 20);
	dark(3, 500);
	morse(num);
	dark(3, 500);
    }

}  /* end of fault() */

/******************************************************************************/
/*
** Send a warning to be printed by the host with the number of RTC
** ticks since teh last receive and send started.
*/
void
warning(int num, unsigned int lastrcv, unsigned int lastsnd)
{

    mcpshmem->warn0 |= num;
    mcpshmem->warn1= lastrcv;
    mcpshmem->warn2= lastsnd;

    /* Tell the host! */
    if (mcpshmem->hstshmem)   {
	int_hst(INT_MCP_WARNING);
    }

}  /* end of warning() */

/******************************************************************************/
#ifdef BENCH

#define TEN	asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
		asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
#define HUN	TEN TEN TEN TEN TEN TEN TEN TEN TEN TEN
#define THOU	HUN HUN HUN HUN HUN HUN HUN HUN HUN HUN
#define TTHOU	THOU THOU THOU THOU THOU THOU THOU THOU THOU THOU
#define HTHOU	TTHOU TTHOU 

/*
** Run some simple benchmarks. We assume one clock tick is 500ns
** and none of this gets optimized away.
*/
void
bench(void)
{

register int i;
int start;
int end;
int empty;
int dummy;
int *ptr;
int isr;


    start= RTC;
    for (i= 0; i < 100; i++)   {
    }
    end= RTC;
    empty= end - start;

    start= RTC;
    for (i= 0; i < 100; i++)   {
	THOU
    }
    end= RTC;
    mcpshmem->benchmark.nop= ((end - start) - empty) * 5 / 1000;

    /* ---------------------------------------------------------------------- */

    start= RTC;
    for (i= 0; i < 10000; i++)   {
    }
    end= RTC;
    empty= end - start;

    start= RTC;
    for (i= 0; i < 10000; i++)   {
	LED= 1;
    }
    end= RTC;
    mcpshmem->benchmark.led= ((end - start) - empty) / 20;

    /* ---------------------------------------------------------------------- */

    start= RTC;
    for (i= 0; i < 10000; i++)   {
    }
    end= RTC;
    empty= end - start;

    start= RTC;
    for (i= 0; i < 10000; i++)   {
	isr= get_ISR();
    }
    end= RTC;
    mcpshmem->benchmark.isr_r= ((end - start) - empty) / 20;

    /* ---------------------------------------------------------------------- */

    start= RTC;
    for (i= 0; i < 10000; i++)   {
    }
    end= RTC;
    empty= end - start;

    start= RTC;
    for (i= 0; i < 10000; i++)   {
	set_ISR(LAN0_SIG_BIT);
    }
    end= RTC;
    mcpshmem->benchmark.isr_w= ((end - start) - empty) / 20;

    /* ---------------------------------------------------------------------- */

    ptr= mcpshmem->rcv_buf_A;
    start= RTC;
    for (i= 0; i < 10000; i++)   {
	ptr++;
    }
    end= RTC;
    empty= end - start;

    ptr= mcpshmem->rcv_buf_A;
    start= RTC;
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(int)); i++)   {
	dummy= *ptr;
	ptr++;
    }
    end= RTC;
    mcpshmem->benchmark.mem_r= ((end - start) - empty) * 500 / DBL_BUF_SIZE;

    /* ---------------------------------------------------------------------- */

    ptr= mcpshmem->rcv_buf_A;
    start= RTC;
    for (i= 0; i < 10000; i++)   {
	ptr++;
    }
    end= RTC;
    empty= end - start;

    ptr= mcpshmem->rcv_buf_A;
    start= RTC;
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(int)); i++)   {
	*ptr= i;
	ptr++;
    }
    end= RTC;
    mcpshmem->benchmark.mem_w= ((end - start) - empty) * 500 / DBL_BUF_SIZE;

}  /* end of bench() */

#endif /* BENCH */
/******************************************************************************/
