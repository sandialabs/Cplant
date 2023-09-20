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
** $Id: Pkt_statem.c,v 1.45 2002/02/14 18:38:12 jbogden Exp $
** The state machine that drives the packet MCP.
*/

#include "lanai_def.h"
#include "MCP.h"
#include "MCPshmem.h"
#include "init.h"
#include "route.h"
#include "Pkt_statem.h"
#include "netlink.h"
#include "ebus.h"
#include "eventlog.h"


/*
** Local functions
*/
static void rcv_statem(void);
static void snd_statem(void);
static int is_pkt_rdy(int protocol);


/*
** Externals
*/
extern mcpshmem_t __mcpshmem;


/*
** Globals
*/
world_time_t world_time;
unsigned int lastrcv;
unsigned int lastsnd;


/*
** Local variables
*/
typedef enum {S0= 1, S1, S2} snd_states_t;
typedef enum {R0= 1, R1, R2, R3, R4, R5} rcv_states_t;

#ifdef STATE_DEBUG
    #define RECORD_RECEIVE_STATE(state) mcpshmem->rcv_state= (state);
    #define RECORD_SEND_STATE(state) mcpshmem->snd_state= (state);
#else
    #define RECORD_RECEIVE_STATE(state)
    #define RECORD_SEND_STATE(state)
#endif /* STATE_DEBUG */

#define NEW_STATE(s) SndState= (s);

#define RCV_A		((unsigned)mcpshmem->rcv_buf_A)
#define RCV_B		((unsigned)mcpshmem->rcv_buf_B)
#define SWAP_RCVBUF(x)	x= (int *)((RCV_A ^ RCV_B) ^ (unsigned)x)
#define SND_A		((unsigned)mcpshmem->snd_buf_A)
#define SND_B		((unsigned)mcpshmem->snd_buf_B)
#define SWAP_SNDBUF(x)	x= (int *)((SND_A ^ SND_B) ^ (unsigned)x)

#define NET_DELAY	(4000000) /* 2000.00 milliseconds */


/******************************************************************************/

extern __inline__ int
is_pkt_rdy(int protocol)
{

    if (protocol == MYRI_PORTAL_PACKET_TYPE)   {
	return TRUE;
    } else if (protocol == MYRI_MyriData_TYPE)   {
	mcpshmem->counters.MyriData++;
	return FALSE;
    } else if (protocol == MYRI_MyriMap_TYPE)   {
	mcpshmem->counters.MyriMap++;
	return FALSE;
    } else if (protocol == MYRI_MyriProbe_TYPE)   {
	mcpshmem->counters.MyriProbe++;
	return FALSE;
    } else if (protocol == MYRI_MyriOption_TYPE)   {
	mcpshmem->counters.MyriOption++;
	return FALSE;
    } else if (protocol == MYRI_SELF_TEST_TYPE)   {
	/* Should check type, pnid, and CRC */
	mcpshmem->self_test= SELF_TEST_OK;
	return FALSE;
    } else if (protocol == MYRI_SELF_DIRECT_TYPE)   {
	/* Should check type, pnid, and CRC */
	mcpshmem->self_test= SELF_TEST_NOSWITCH;
	return FALSE;
    } else   {
	mcpshmem->counters.wrngprtcl++;
	record_event(EVENT_WRNGPROT, 0, (unsigned int)RMP, (unsigned int)RML);
	return FALSE;
    }

}  /* end of is_pkt_rdy() */

/******************************************************************************/
/*
** rcv_statem()
** check for receive events and act accordingly
*/
extern __inline__ void
rcv_statem(void)
{

static rcv_states_t RcvState= R0;
static int *cur_buf= __mcpshmem.rcv_buf_A;
static int *old_buf= __mcpshmem.rcv_buf_B;
static int *pkt_data;
static int cur_len;
static unsigned int cur_hst_addr;
static int threshold;
static unsigned int old_RMP= 0;
static unsigned int old_RML= 0;
static int last_rcv_pkt= 0;
static world_time_t idletime= {0, 0};
static int expected_len;
int avail_len;
int protocol;
int over, len;
unsigned int crc;
rcv_states_t new_state;
#if defined (USE_32BIT_CRC)
    unsigned int crc32;
#endif


    #ifdef STATE_DEBUG
	{
	    static int first_time= TRUE;

	    if (first_time)   {
		first_time= FALSE;
		RECORD_RECEIVE_STATE(R0);
	    }
	}
    #endif /* STATE_DEBUG */

    switch (RcvState)   {
	case R0:	/* Idle */
	    if (RMP != cur_buf)   {
		/*
		** Data has begun to stream in!
		** We've got at least 4 bytes (RMP increases in increments of
		** 4 for LANai 4.x and 8 for LANai 7.x)
		** The first 2 bytes are the packet type. The next 2 bytes are
		** the length.
		*/
		lastrcv= RTC;
		protocol= *((unsigned char *)cur_buf) << 8;
		protocol |= *((unsigned char *)cur_buf + 1);
		cur_hst_addr= mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr;

		if (is_pkt_rdy(protocol))   {
		    int_hst(INT_RCV_BEGUN);
		    if (cur_hst_addr & ~0x0f)   {
			/* Host is ready! */

			cur_len= *((unsigned char *)cur_buf + 2) << 8;
			cur_len |= *((unsigned char *)cur_buf + 3);
			expected_len= cur_len + 8; /* 8 for header */
			expected_len= (expected_len + ALIGN_64b) & ~ALIGN_64b;
			/* Make sure there is enough room on the host side */
			if (cur_len > mcpshmem->rcv_pkt_list[last_rcv_pkt].len){
			    /* There is a problem */
			    record_event(EVENT_TOOLONG, cur_buf, (unsigned int)RMP, (unsigned int)RML);
			    mcpshmem->counters.dumped++;
			    RcvState= R5;
			    RECORD_RECEIVE_STATE(RcvState);
			    break;
			} else   {
			    pkt_data= cur_buf + 2;	/* Start of real data */
			    threshold= mcpshmem->rcv_start_th;
			    mcpshmem->rcv_pkt_list[last_rcv_pkt].len= cur_len;
			    RcvState= R1;
			    RECORD_RECEIVE_STATE(RcvState);
			    /* FALL THROUGH */
			}
		    } else   {
			/* Host is not ready. Dump packet */
			world_time_t target;

			SET_HOST_SIG_BIT();	/* Don't hold back. Hard int */
			setAlarm(&target, 20);

			mcpshmem->counters.hst_dly++;
			while (!checkAlarm(&target))
			    ;

			if (mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr & 0x0f)   {
                /* Host is still not ready. Dump packet */
                record_event(EVENT_HSTORUN, cur_buf, (unsigned int)RMP, (unsigned int)RML);
                mcpshmem->counters.dumped++;
                RcvState= R5;
			} else   {
			    /* Saved by the bell */
                pkt_data= cur_buf + 2;	/* Start of real data */
                mcpshmem->rcv_pkt_list[last_rcv_pkt].len= cur_len;
                cur_hst_addr= mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr;
                RcvState= R1;
			}
			RECORD_RECEIVE_STATE(RcvState);
			break;
		    }
		} else   {
		    /* Not a packet we want */
		    RcvState= R5;
		    RECORD_RECEIVE_STATE(RcvState);
		    break;
		}
	    } else   {
		/* Nothing received so far */
		if (get_ISR() & NRES_INT_BIT)   {
		    record_event(EVENT_NRES, cur_buf, (unsigned int)RMP, (unsigned int)RML);
		    set_ISR(NRES_INT_BIT);
		    mcpshmem->counters.fres++;
		    warning(WARN_NRESRCV, RTC - lastrcv, RTC - lastsnd);
		}
		#if defined (L7) || defined (L9)
		    if (get_ISR() & PARITY_INT_BIT)   {
			record_event(EVENT_MPAR, cur_buf, (unsigned int)RMP, (unsigned int)RML);
			set_ISR(PARITY_INT_BIT);
			mcpshmem->counters.mem_parity++;
			warning(WARN_PARRCV, RTC - lastrcv, RTC - lastsnd);
		    }
		    if (get_ISR() & MEMORY_INT_BIT)   {
			record_event(EVENT_MINT, cur_buf, (unsigned int)RMP, (unsigned int)RML);
			set_ISR(MEMORY_INT_BIT);
			mcpshmem->counters.mem_parity++;
			/*
			warning(WARN_PARRCV, RTC - lastrcv, RTC - lastsnd);
			*/
		    }
		#endif /* L7 */
		if (get_ISR() & TIME_INT_BIT)   {
		    /* Interrupt timer has wrapped: 35 minutes */
		    set_ISR(TIME_INT_BIT);
		    world_time.t1++;
		}
		if (checkAlarm(&idletime))   {
		    if (mcpshmem->counters.rcvs != mcpshmem->num_rcv_pkts)   {
			/*
			** host is behind in processing packets
			** int_hst() delays interrupts if necessary
			*/
			int_hst(INT_RCV_BEGUN);
			setAlarm(&idletime, 0);
		    } else   {
			setAlarm(&idletime, 2000000); /* Nothing to do */
		    }
		}
		break;
	    }
	case R1:	/* Grab PCI bus */
	    if (is_pci_bus_avail())   {
		grab_pci_bus();
		RcvState= R2;
		RECORD_RECEIVE_STATE(RcvState);
		/* FALL THROUGH */
	    } else   {
		break;
	    }
	case R2:	/* Transfer data chunks to host */
	    avail_len= ((int)RMP - (int)pkt_data - sizeof(int) /* CRC */) &
			    ~ALIGN_64b;
	    fail_if (a89, (avail_len & 0x07));
	    if ((avail_len > threshold) && l2e_dma_done() &&
		    ((cur_len - avail_len) > mcpshmem->rcv_stop_th))   {
		/*
		** We got enough data to make it worthwhile to push it
		** to the host. And the PCI bus is available. And there
		** is enough data still on the wire to make one more transfer
		** reasonable. (Otherwise we just wait until we have it all
		** and tranfer it in one chunk.)
		*/
		l2e_dma_start(pkt_data, avail_len, cur_hst_addr);
		pkt_data += (avail_len / sizeof(int));
		cur_hst_addr += avail_len;
		cur_len -= avail_len;
		threshold= mcpshmem->rcv_cont_th;

		/* Stay in this state (R2) */

	    } else if ((get_ISR() & RECV_INT_BIT) || (get_ISR() & BUFF_INT_BIT))   {
		/*
		** All the data from the network is in. Set cur_buf to the
		** other buffer and restart the receive DMA.
		*/
		new_state= R3;
		if (get_ISR() & BUFF_INT_BIT)   {
		    /*
		    ** The packet was longer than our buffer. This should not
		    ** be. (Switch probably ate tail flit.) Toss this packet
		    ** and suck in the remainder.
		    */
		    record_event(EVENT_LONG, cur_buf, (unsigned int)RMP, (unsigned int)RML);
		    mcpshmem->counters.toolong++;
		    expected_len= -1;
		    new_state= R5;
		}

		old_buf= cur_buf;
		SWAP_RCVBUF(cur_buf);
		fail_if (a90, ((cur_buf != mcpshmem->rcv_buf_A) &&
		               (cur_buf != mcpshmem->rcv_buf_B)));

		old_RMP= (unsigned int)RMP;
		old_RML= (unsigned int)RML;
		RMP= cur_buf;
		RML= (int *)((int)cur_buf + DBL_BUF_SIZE - 4);
		RcvState= new_state;
		RECORD_RECEIVE_STATE(RcvState);

	    }

	    break;
	case R3:	/* Transfer last chunk to host and update vars */
	    if (l2e_dma_done())   {
		l2e_dma_start(pkt_data, cur_len, cur_hst_addr);
		RcvState= R4;
		RECORD_RECEIVE_STATE(RcvState);
	    }
	    break;
	case R4:	/* Wait for last push to host to complete */
	    if (l2e_dma_done())   {
		/* Check the CRC byte! */
		#if defined(L4)
		    /* RMP is 4 over on LANai 4.x */
		    len= old_RMP - (unsigned int)old_buf - 4;
		#endif /* L4 */
		#if defined(L7) || defined (L9)
		    /* RMP is 8 over on LANai 7.x */
		    len= old_RMP - (unsigned int)old_buf - 8;
		#endif /* L7 */
		#if defined (USE_32BIT_CRC)
		    /*
		    ** The CRC is in the 4 MSB of the 8-byte word at
		    ** location RMP - 8. Therefore, we want to look at RMP - 4;
		    */
		    crc32= *((unsigned int *)(old_RMP - 4));
		    crc= crc32;
		#else
		    crc= old_buf[len >> 2] & 0xff000000;
		#endif
		if (expected_len < len)   {
		    mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr= LONG_PKT;
		    mcpshmem->counters.toolong++;
		    record_event(EVENT_LONG, old_buf, (unsigned int)old_RMP, (unsigned int)old_RML);
		} else if (expected_len > len)   {
		    mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr= TRUNC_PKT;
		    mcpshmem->counters.truncated++;
		    record_event(EVENT_TRUNC, old_buf, (unsigned int)old_RMP, (unsigned int)old_RML);
		} else if (crc)   {
		    mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr= BAD_CRC_PKT;
		    mcpshmem->counters.crc++;
		    record_event(EVENT_CRC, old_buf, (unsigned int)old_RMP, (unsigned int)old_RML);
		} else   {
		    /* CRC is fine */
		    mcpshmem->rcv_pkt_list[last_rcv_pkt].phys_addr= GOOD_PKT;
		}

		setAlarm(&idletime, 0);
		last_rcv_pkt= (last_rcv_pkt + 1) % MAX_RCV_PKT_ENTRIES;
		mcpshmem->counters.rcvs++;
		free_pci_bus();
		RcvState= R0;
		RECORD_RECEIVE_STATE(RcvState);
	    }
	    break;
	case R5:	/* Consume unwanted packets */
	    if ((get_ISR() & RECV_INT_BIT) || (get_ISR() & BUFF_INT_BIT))   {
		/*
		** We're just waiting for the rest of this packet to stream
		** in. Then we post a new receive and go back to idle.
		*/

		new_state= R0;
		if (get_ISR() & BUFF_INT_BIT)   {
		    /*
		    ** The packet was longer than our buffer. This should not
		    ** be. (Switch probably ate tail flit.) Toss this packet.
		    */
		    record_event(EVENT_LONG, cur_buf, (unsigned int)RMP, (unsigned int)RML);
		    mcpshmem->counters.toolong++;
		    new_state= R5;
		}


		over= 0;
		if (get_ISR() & ORUN2_INT_BIT)   {
		    over= 2;
		}
		if (get_ISR() & ORUN1_INT_BIT)   {
		    over++;
		}
		len= (int)RMP - (int)cur_buf - over - 1;  /* don't count CRC */
		mcpshmem->counters.dumped_num += len;
		crc= ((char *)cur_buf)[len];
		if (crc)   {
		    mcpshmem->counters.dumped_crc++;
		}

		RMP= cur_buf;
		RML= (int *)((int)cur_buf + DBL_BUF_SIZE - 4);
		setAlarm(&idletime, 0);
		RcvState= new_state;
		RECORD_RECEIVE_STATE(RcvState);
	    }
	    break;
    }

}  /* end of rcv_statem() */

/******************************************************************************/
/*
** snd_statem()
** check for send events and act accordingly
*/
extern __inline__ void
snd_statem(void)
{

static snd_states_t SndState= S0;
static int *cur_buf= __mcpshmem.snd_buf_A;
static int *pkt_data;
static int cur_len;
static int cur_dest;
static int last_snd_pkt= 0;
static int threshold;
static int pkt_on_LANai;
unsigned int hst_addr;
#if !defined (L7) && !defined (L9)
    int avail_len;
#endif
static world_time_t net_timeout= {0,0};
static int timeout_recorded= FALSE;


    #ifdef STATE_DEBUG
	{
	    static int first_time= TRUE;

	    if (first_time)   {
		first_time= FALSE;
		RECORD_SEND_STATE(S0);
	    }
	}
    #endif /* STATE_DEBUG */


    switch (SndState)   {
	case S0:	/* Idle */
	    hst_addr= mcpshmem->snd_pkt_list[last_snd_pkt].phys_addr;
	    if (is_pci_bus_avail() && hst_addr)   {
		/* The host is ready to send */

		SWAP_SNDBUF(cur_buf);
		fail_if (a92, ((cur_buf != mcpshmem->snd_buf_A) &&
		               (cur_buf != mcpshmem->snd_buf_B)));
		cur_len= mcpshmem->snd_pkt_list[last_snd_pkt].len;
		fail_if (a91, (hst_addr & ALIGN_32b));
		e2l_dma_start(cur_buf, cur_len, hst_addr);
		cur_dest= mcpshmem->snd_pkt_list[last_snd_pkt].dest;
		pkt_data= cur_buf;
		pkt_on_LANai= FALSE;
		timeout_recorded= FALSE;
		grab_pci_bus();
		NEW_STATE(S1);
		RECORD_SEND_STATE(SndState);
		/* FALL THROUGH */
	    } else if (checkAlarm(&net_timeout) && !(get_ISR() & SEND_RDY_BIT) &&
		    !timeout_recorded)   {
		mcpshmem->counters.send_timeout++;
		timeout_recorded= TRUE;
		warning(WARN_SNDTIMEOUT, RTC - lastrcv, RTC - lastsnd);
		break;
	    } else   {
		break;
	    }
	case S1:	/* Wait for the network */
	    if (get_ISR() & SEND_RDY_BIT)   {
		lastsnd= RTC;
		start_msg(cur_dest, cur_len);
		setAlarm(&net_timeout, NET_DELAY);
		threshold= mcpshmem->snd_start_th;
		timeout_recorded= FALSE;
		NEW_STATE(S2);
		RECORD_SEND_STATE(SndState);
		/* FALL THROUGH */
	    } else if (e2l_dma_done() && !pkt_on_LANai)   {
		/* We're still waiting to send, but release
		   the PCI bus when that transfer finishes. */
		free_pci_bus();
		pkt_on_LANai= TRUE;
		break;
	    } else if (checkAlarm(&net_timeout) && !timeout_recorded)   {
		mcpshmem->counters.send_timeout++;
		timeout_recorded= TRUE;
		warning(WARN_SNDTIMEOUT, RTC - lastrcv, RTC - lastsnd);
		break;
#if defined (L7)
	    } else if (get_ISR() & LINK2_INT_BIT)   {
		record_event(EVENT_LINK2, cur_buf, (unsigned int)RMP, (unsigned int)RML);
		set_ISR(LINK2_INT_BIT);
		mcpshmem->counters.link2++;
		warning(WARN_LINK2SND, RTC - lastrcv, RTC - lastsnd);
		break;
#endif
	    } else   {
		break;
	    }
	case S2:	/* Transmit some data */
#if !defined (L7) && !defined (L9)
	    avail_len= (cur_len - DMA_CTR) & ~ALIGN_64b;
#endif
	    if ((pkt_on_LANai || e2l_dma_done()) && (get_ISR() & SEND_INT_BIT)) {
		snd_bufEOM(pkt_data, cur_len);
		setAlarm(&net_timeout, NET_DELAY);
		mcpshmem->snd_pkt_list[last_snd_pkt].phys_addr= 0;
		mcpshmem->counters.sends++;
		last_snd_pkt= (last_snd_pkt + 1) % MAX_SND_PKT_ENTRIES;
		if (!pkt_on_LANai)   {
		    free_pci_bus();
		}
		timeout_recorded= FALSE;
		NEW_STATE(S0);
		RECORD_SEND_STATE(SndState);
#if !defined (L7) && !defined (L9)
	    } else if ((get_ISR() & SEND_INT_BIT) && (avail_len > threshold) &&
		    (DMA_CTR > mcpshmem->snd_stop_th))   {
		snd_buf(pkt_data, avail_len);
		pkt_data += (avail_len / sizeof(int));
		cur_len -= avail_len;
		threshold= mcpshmem->snd_cont_th;
		/* Stay in this state */
#endif /* L7 */
	    } else if (e2l_dma_done() && !pkt_on_LANai)  {
		/* We're still waiting to send, but release
		   the PCI bus when that transfer finishes. */
		free_pci_bus();
		pkt_on_LANai= TRUE;
	    } else if (checkAlarm(&net_timeout) && !timeout_recorded)   {
		mcpshmem->counters.send_timeout++;
		timeout_recorded= TRUE;
		warning(WARN_SNDTIMEOUT, RTC - lastrcv, RTC - lastsnd);
	    }

	    break;
    }

}  /* end of snd_statem() */

/******************************************************************************/

#undef RCV_CONTINUOUS_DMA
#undef SND_CONTINUOUS_DMA

#ifdef RCV_CONTINUOUS_DMA
    #define RCV_START_THRESHOLD	(512)
    #define RCV_CONT_THRESHOLD	(512)
    #define RCV_STOP_THRESHOLD	(336)
#else
    #define RCV_START_THRESHOLD	(DBL_BUF_SIZE)
    #define RCV_CONT_THRESHOLD	(DBL_BUF_SIZE)
    #define RCV_STOP_THRESHOLD	(DBL_BUF_SIZE)
#endif /* RCV_CONTINUOUS_DMA */

#ifdef SND_CONTINUOUS_DMA
    #define SND_START_THRESHOLD	(256)
    #define SND_CONT_THRESHOLD	(256)
    #define SND_STOP_THRESHOLD	(172)
#else
    #define SND_START_THRESHOLD	(DBL_BUF_SIZE)
    #define SND_CONT_THRESHOLD	(DBL_BUF_SIZE)
    #define SND_STOP_THRESHOLD	(DBL_BUF_SIZE)
#endif /* SND_CONTINUOUS_DMA */


/*
** state_machine()
** Infinite loop that runs the receive state machine and then the send state
** machine.
*/
void
pkt_state_machine(void)
{

    /* Setup a DMA for receive messages */
    RMP= mcpshmem->rcv_buf_A;
    RML= (int *)((int)mcpshmem->rcv_buf_A + DBL_BUF_SIZE - 4);

    /* Set the default thresholds */
    mcpshmem->rcv_start_th= RCV_START_THRESHOLD;
    mcpshmem->rcv_cont_th= RCV_CONT_THRESHOLD;
    mcpshmem->rcv_stop_th= RCV_STOP_THRESHOLD;
    mcpshmem->snd_start_th= SND_START_THRESHOLD;
    mcpshmem->snd_cont_th= SND_CONT_THRESHOLD;
    mcpshmem->snd_stop_th= SND_STOP_THRESHOLD;

    while (TRUE)   {
	rcv_statem();
	snd_statem();
	/*
	** We have no valid host requests at this moment
	host_requests();
	*/
    }

}  /* end of state_machine() */

/******************************************************************************/
