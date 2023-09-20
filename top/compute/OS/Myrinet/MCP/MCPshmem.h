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
** $Id: MCPshmem.h,v 1.70 2001/08/22 23:00:32 pumatst Exp $
** This file describes the data structures in shared memory on the LANai
** that are used for communications between the host and the LANai.
*/

#ifndef MCPSHMEM_H
#define MCPSHMEM_H

/*
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
** Change the MCP_version every time you change the mcpshmem_t structure
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#define MCP_version	(314)
#define MCP_ID		(0x526f6c66)

#define MCP_7_CHECK_STR	"---> This is an MCP for a 7.x LANai <--"
#define MCP_4_CHECK_STR	"---> This is an MCP for a 4.x LANai <--"
#define MCP_9_CHECK_STR	"---> This is an MCP for a 9.x LANai <--"

enum{ SANITY, INTEGRITY };
enum{ DMA_TEST_NOTDONE, DMA_TEST_FAILED, DMA_TEST_OK };


#if defined (__lanai3__) || defined(__lanai7__) || defined (__lanai9__) || defined(__lanai__)
    /*
    ** There is no uid_t in the LANai libraries. The LANai doesn't look at
    ** it anyway. It is used by the host to store the uid of the user who
    ** started the MCP.
    */
    typedef int uid_t;
#else
    #ifndef MODULE
	#include <time.h>
	#include <sys/types.h>
    #endif /* MODULE */
#endif /* __lanai3__ */

#include "eventlog.h"


/* 64-bit accesses from the Alpha must be 8-byte aligned */
#undef ALIGN_32b
#define ALIGN_32b		(3)
#undef ALIGN_64b
#define ALIGN_64b		(7)

/* How many transfers to measure DMA (PCI bus) speed */
#define DMA_TEST_CNT		(1000)

/* How many tests each way we store */
#define DMA_TEST_MAX		(8)

/* Max DMA test size and location of buffer on host side */
#define DMATST_AREA_SIZE	(16 * 1024)
extern unsigned int *dmatst_area;

/* The size of the receive and send buffers on the LANai */
#define DBL_BUF_SIZE		(1 * 8 * 1024)

/* How many packet send requests can be submitted simultaneously? */
#define MAX_SND_PKT_ENTRIES	(2048)

/* How many rcv buffers (skb) do we have in our list? */
#define MAX_RCV_PKT_ENTRIES	(2048)

/* Some entries that can be used for debugging */
#define MAX_DEBUG		(32)

/* Routes are stored in a 2-D array */
#define MAX_ROUTE_LEN		(32)	/* Make sure this is a multiple of 8 */
#define MAX_NUM_ROUTES		(2048)	/* This should be a power of 2 */
#define MAX_TEST_ROUTE_LEN	(256)	/* Room for test routes */
#define TEST_ROUTE_ID		(54321)	/* This dest selects the test route */

/*
** This structure is used to build a circular list of entries of send
** packets. The host fills it in; the MCP removes the items.
*/
typedef struct   {
    volatile unsigned int phys_addr;	/* start address used by LANai */
    volatile unsigned int len;		/* how many bytes */
    volatile int dest;			/* Where does this packet go? */
    int page_idx;			/* Which page is this? */
} snd_pkt_list_t;


/*
** Markers used in phys_addr to indicate problems
** Whne the driver has processed a page and is returning it to the
** MCP to be filled again, then the driver set phys_addr to a bus
** address with the 13 LSB cleared, since it is page aligned. The
** driver sets the len field to MYRPKT_MTU.
** When the MCP has filled a page, it sets phys_addr to one of the
** markers below, and len to the actual number of bytes updated
** on the host side.
*/
#define GOOD_PKT		(1)
#define LONG_PKT		(2)
#define BAD_CRC_PKT		(3)
#define TRUNC_PKT		(4)
#define NO_PAGE			(5)

/*
** This structure holds a pointer to the physical address of a reserved
** buffer on the host.
*/
typedef struct   {
    volatile unsigned int phys_addr;	/* start address used by LANai */
    volatile unsigned int len;		/* how many bytes came in? */
} rcv_pkt_list_t;


typedef struct   {
    unsigned fres;		/* Network resets */
    unsigned reset;		/* LANai resets */
    unsigned hst_dly;		/* Host delayed pkt delivery */
    unsigned sends;		/* Messages sent */
    unsigned rcvs;		/* Messages received */
    unsigned dumped;		/* Messages received and bit bucketed */
    unsigned dumped_num;	/* Number of bytes dumped */
    unsigned crc;		/* CRC errors on receive */
    unsigned dumped_crc;	/* CRC errors on dumped data */
    unsigned truncated;		/* Short pkt received */
    unsigned toolong;		/* long pkt received */
    unsigned send_timeout;	/* Pkts that timed out on a send */
    unsigned link2;		/* send too long or blocked */
    unsigned wrngprtcl;		/* Wrong protocol messages */
    unsigned MyriData;		/* Num of Myrinet data packtes */
    unsigned MyriMap;		/* Num of Myrinet map packtes */
    unsigned MyriProbe;		/* Num of Myrinet probe packtes */
    unsigned MyriOption;	/* Num of Myrinet Option packtes */
    unsigned mem_parity;	/* Parity in LANai memory */
} counters_t;

typedef struct   {
    int nop;		/* Time for a nop */
    int led;		/* Time for a write to LED */
    int isr_r;		/* Time to read from ISR */
    int isr_w;		/* Time to write to ISR */
    int mem_r;		/* Time to read from (shared) memory */
    int mem_w;		/* Time to write to (shared) memory */
} bench_t;


/*
** Some fields MUST start on a 64 bit boundary for the Alpha! They are:
**     stime
*/


/* The shared data structure */
typedef struct   {
    #if defined(__alpha__)
	time_t stime;		/* MCP start time as a 64 bit value on alpha */
    #elif defined(__lanai3__) || defined(__lanai7__) || defined (__lanai9__) || defined(__lanai__)
	int stime[2];		/* Reserve 64 bits. MCP never looks at this */
    #else
	time_t stime;		/* Start time as a 32 bit value */
	int pad;		/* Pad to 64 bits */
    #endif

    int snd_buf_A [DBL_BUF_SIZE / sizeof(int)];
    int snd_buf_B [DBL_BUF_SIZE / sizeof(int)];
    int rcv_buf_A [DBL_BUF_SIZE / sizeof(int)];
    int rcv_buf_B [DBL_BUF_SIZE / sizeof(int)];

    snd_pkt_list_t snd_pkt_list[MAX_SND_PKT_ENTRIES];
    rcv_pkt_list_t rcv_pkt_list[MAX_RCV_PKT_ENTRIES];

    /* Don't put anything below this line that needs 64 bit alignement */

    int debug[MAX_DEBUG];	/* Some entries for debugging information */

    char route[MAX_NUM_ROUTES][MAX_ROUTE_LEN];

    /*
    ** For each destination we need room for MAX_ROUTE_LEN bytes of route plus
    ** a 4-byte header containing the packet type and packet length. Due to
    ** LANai 7.x alignment restrictions, we make the header 8 bytes long
    ** (2 * sizeof(int))
    */
    int route_copy[MAX_NUM_ROUTES] [(MAX_ROUTE_LEN + 2 * sizeof(int)) /
	    sizeof(int)] __attribute__ ((aligned (8)));

    int test_route[MAX_TEST_ROUTE_LEN / sizeof(int)]
	    __attribute__ ((aligned (8)));
    int test_route_len;		/* How long is the test route? */

    counters_t counters;
    bench_t benchmark;		/* bench measurements during initialization */

    eventlog_entry_t eventlog[EVENT_MAX];
    unsigned int event_num;
    unsigned int eventlog_next;

    volatile int my_pnid;	/* This node's physical number */
    int fault;			/* Last MCP fault */
    int fault_isr;		/* ISR at time of fault */
    unsigned int warn0;		/* Pass info to host to be printed */
    unsigned int warn1;		/* Pass info to host to be printed */
    unsigned int warn2;		/* Pass info to host to be printed */
    int reset_imr;		/* IMR at time of last reset */
    int reset_isr;		/* ISR at time of last reset */
    uid_t uid;			/* User who started MCP */
    volatile int snd_state;	/* Debugging the state machine */
    volatile int rcv_state;	/* Debugging the state machine */
    int self_test;		/* Current state of self test */
    volatile int hstshmem;	/* Phys address of shmem on host side */

    volatile unsigned num_rcv_pkts;	/* Host received packets */
    volatile int LANai2host;	/* Type of interrupt */

    /* MCP thresholds */
    volatile int rcv_start_th;	/* Receive start threshold */
    volatile int rcv_cont_th;	/* Receive continue threshold */
    volatile int rcv_stop_th;	/* Receive stop threshold */
    volatile int snd_start_th;	/* Send start threshold */
    volatile int snd_cont_th;	/* Send continue threshold */
    volatile int snd_stop_th;	/* Send stop threshold */

    /* Type of MCP. Right now we have the portal, test, and packet MCP. */
    volatile int mcp_type;
    volatile int mod_type;	/* The module type should match the MCP */
    volatile int LANai_vers;	/* Version of LANai. We support 4.x & 7.x */

    volatile int route_request;
    volatile int route_stat;
    volatile int ping_stat;

    /*
    ** Tell the host to set up our DMA chains. LANai 7.x only. I
    ** haven't been able to figure out how to do that from the board itself.
    */
    volatile unsigned int DMAchannel0;
    volatile unsigned int DMAchannel2;

    /* Fields used to conduct PCI bus tests */
    volatile unsigned int DMA_test_buf;
    volatile unsigned int DMA_test_type;
    volatile unsigned int DMA_test_index;  /* which integrity test/pattern? */
    volatile unsigned int DMA_test_repeat; /* loop for integrity pattern */
    volatile unsigned int DMA_test_len;
    volatile unsigned int DMA_test_result;
    volatile unsigned int DMA_test_failed;
    volatile unsigned int DMA_test_iter;
    volatile unsigned int DMA_buf_id;
    volatile unsigned int DMA_test_done;

    volatile unsigned int e2l_len[DMA_TEST_MAX];
    volatile unsigned int e2l_result[DMA_TEST_MAX];
    volatile unsigned int l2e_len[DMA_TEST_MAX];
    volatile unsigned int l2e_result[DMA_TEST_MAX];


    /*
    ** Leave ID and versions down here! If the structure gets unaligned
    ** or is different on the host and the LANai, then the offset of ID
    ** and versions will differ. We check for that during MCP load and
    ** can detect it.
    */
    int ID;		/* Identifier for this structure */
    int version;	/* version of this structure */

    char pad_long[(256 * 1024)
	- (4 * DBL_BUF_SIZE)
	- (EVENT_MAX * sizeof(eventlog_entry_t))
	- (sizeof(snd_pkt_list_t) * MAX_SND_PKT_ENTRIES)
	- (sizeof(rcv_pkt_list_t) * MAX_RCV_PKT_ENTRIES)
	- (MAX_NUM_ROUTES * MAX_ROUTE_LEN)
	- (sizeof(int) * MAX_NUM_ROUTES * ((MAX_ROUTE_LEN + 2 * sizeof(int)) / sizeof(int)))
	- MAX_TEST_ROUTE_LEN
	- sizeof(counters_t)
	- sizeof(bench_t)
	- (sizeof(int) * MAX_DEBUG)
	- (4 * (sizeof(int) * DMA_TEST_MAX))
	- (48 * sizeof(int))
    ];
    int align_pad1;	/* In case we need to shift this data structure, so */
    int align_pad2;	/* the Alpha can access 64 bit values aliged. */
} mcpshmem_t;

extern mcpshmem_t *mcpshmem;


/*
** Self test status
*/
#define SELF_TEST_FAIL		(-3)	/* Most likely disconnected from net */
#define SELF_TEST_INVAL		(-2)	/* Corrupted self test packet */
#define SELF_TEST_OTHER		(-1)	/* Someone else's self test packet */
#define SELF_TEST_UNKNOWN	(0)	/* Unknown status */
#define SELF_TEST_OK		(1)	/* Connected to net and operational */
#define SELF_TEST_NOSWITCH	(2)	/* Connected directly to another IF */


/*
** Reasons why the LANai interrupted the host
*/
#define NO_REASON         (0x01)
#define INT_RCV_BEGUN     (0x02)    /* A receive has begun */
#define GET_HSTSHMEM      (0x04)    /* LANai wants address of hst shmem */
#define INT_RCV_DONE      (0x08)    /* A receive has been completed */
#define INT_MCP_FAULT     (0x10)    /* The MCP is dead */
#define INT_DMA_SETUP     (0x20)    /* LANai 7.x DMA setup */
#define INT_DMA_E2L_TEST  (0x40)    /* E2L test */
#define INT_DMA_L2E_TEST  (0x80)    /* L2E SANITY test */
#define INT_DMA_L2E_INTEGRITY_TEST  (0x100)  /* L2E INTEGRITY test */
#define INT_DMA_E2L_INTEGRITY_TEST  (0x200)  /* E2L INTEGRITY test */
#define INT_MCP_WARNING   (0x400)  /*Print a warning message */


/*
** Types of MCPs (for the mcp_type field)
*/
#define MCP_TYPE_PORTAL		(200)
#define MCP_TYPE_PKT		(201)
#define MCP_TYPE_TEST		(202)
#define MCP_TYPE_RTSCTS		(203)


/*
** LANai type
*/
#define LANai_unknown		(111)	/* ???? */
#define LANai_4			(4)	/* 4.x */
#define LANai_7			(7)	/* 7.x */
#define LANai_9			(9)	/* 9.x */


/*
** Possible faults
*/
#define NO_FAULT	(0)	/* We're fine */


/*
** Possible warnings (to be printed by the host)
*/
#define WARN_NRESRCV		(0x01)	/* NRES during receive */
#define WARN_PARRCV		(0x02)	/* Parity error during receive */
#define WARN_LINK2RCV		(0x04)	/* Link2 error during receive */
#define WARN_LINK2SND		(0x08)	/* Link2 error during send */
#define WARN_SNDTIMEOUT		(0x10)	/* Send timeout */


#endif /* MCPSHMEM_H */
