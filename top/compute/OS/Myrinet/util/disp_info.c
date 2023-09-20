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
** $Id: disp_info.c,v 1.20 2001/06/12 19:36:22 rolf Exp $
** A set of utilities to display registers and Puma/MCP/myrptl data
** structures.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include "lanai_device.h"
#include "defines.h"
#include "../MCP/MCPshmem.h"
#include "../MCP/hstshmem.h"
#include "common.h"
#include "disp_info.h"
//#include <portals/comm_buff.h>


/******************************************************************************/

static void
print_flag(char *str, int *indent)
{

    if ((*indent + strlen(str) + 2) > 75)   {
	*indent= 8 + strlen(str) + 2;
	printf("\n        ");
    } else   {
	*indent+= strlen(str) + 2;
    }
    printf("%s, ", str);	/* + 2 for the comma and space */

}  /* end of print_flag() */

/******************************************************************************/
/*
** printISR()
** display the names of the set bits in the ISR
*/
void
printISR(int isr, int unit, int type)
{

int indent= 0;


    printf("    ISR register is 0x%08x\n", (unsigned)isr);

    if (isr != 0)   {
	printf("        ");
	indent+= 8;
    }
    if (isr & DBG_BIT)   {
	print_flag("debug bit set", &indent);
    }
    if (isr & HOST_SIG_BIT)   {
	print_flag("host sig bit set", &indent);
    }
    if (type == L7)   {
	if (isr & LINK2_INT_BIT)   {
	    print_flag("link2: pkt too long", &indent);
	}
	if (isr & LINK1_INT_BIT)   {
	    print_flag("link1: down", &indent);
	}
	if (isr & LINK0_INT_BIT)   {
	    print_flag("link0: up", &indent);
	}
    }
    if (type == L9)   {
	if (isr & LANAI_SIG_BIT)   {
	    print_flag("lanai sig bit set", &indent);
	}
	if (isr & BEAT_MISSED_INT_BIT)   {
	    print_flag("beat missed bit set", &indent);
	}
	if (isr & TX_BLOCKED_INT_BIT)   {
	    print_flag("tx blocked bit set", &indent);
	}
	if (isr & TX_TOO_LONG_INT_BIT)   {
	    print_flag("tx too long bit set", &indent);
	}
	if (isr & RX_TOO_LONG_INT_BIT)   {
	    print_flag("rx too long bit set", &indent);
	}
	if (isr & ILLEGAL_RECEIVED_INT_BIT)   {
	    print_flag("illegal rcvd bit set", &indent);
	}
	if (isr & BEAT_RECEIVED_INT_BIT)   {
	    print_flag("beat rcvd bit set", &indent);
	}
	if (isr & DATA_RECEIVED_INT_BIT)   {
	    print_flag("data rcvd bit set", &indent);
	}
	if (isr & DATA_SENT_INT_BIT)   {
	    print_flag("data sent bit set", &indent);
	}
	if (isr & LINK_DOWN_BIT)   {
	    print_flag("link down bit set", &indent);
	}
    }
    if ((type == L4) || (type == L7))   {
	if (isr & LAN7_SIG_BIT)   {
	    print_flag("lan sig bit 7 set", &indent);
	}
	if (isr & LAN6_SIG_BIT)   {
	    print_flag("lan sig bit 6 set", &indent);
	}
	if (isr & LAN5_SIG_BIT)   {
	    print_flag("lan sig bit 5 set", &indent);
	}
	if (isr & LAN4_SIG_BIT)   {
	    print_flag("lan sig bit 4 set", &indent);
	}
	if (isr & LAN3_SIG_BIT)   {
	    print_flag("lan sig bit 3 set", &indent);
	}
	if (isr & LAN2_SIG_BIT)   {
	    print_flag("lan sig bit 2 set", &indent);
	}
	if (isr & LAN1_SIG_BIT)   {
	    print_flag("lan sig bit 1 set", &indent);
	}
	if (isr & LAN0_SIG_BIT)   {
	    print_flag("lan sig bit 0 set", &indent);
	}
    }
    if (type == L4)   {
	if (isr & WORD_RDY_BIT)   {
	    print_flag("word ready", &indent);
	}
	if (isr & HALF_RDY_BIT)   {
	    print_flag("half word ready", &indent);
	}
	if (isr & SEND_RDY_BIT)   {
	    print_flag("send ready", &indent);
	}
	if (isr & LINK_INT_BIT)   {
	    print_flag("reserved (link int)", &indent);
	}
    }
    if ((type == L7) || (type == L9))   {
	if (isr & PARITY_INT_BIT)   {
	    print_flag("parity err on Lbus", &indent);
	}
	if (isr & MEMORY_INT_BIT)   {
	    print_flag("segv", &indent);
	}
	if (isr & TIME2_INT_BIT)   {
	    print_flag("interrupt timer 2 expired", &indent);
	}
	if (isr & WAKE7_INT_BIT)   {
	    print_flag("wake input pin high", &indent);
	}
    }
    if (isr & NRES_INT_BIT)   {
	print_flag("network reset", &indent);
    }
    if (type == L4)   {
	if (isr & WAKE4_INT_BIT)   {
	    print_flag("wake input pin high", &indent);
	}
    }
    if ((type == L7) || (type == L9))   {
	if (isr & OFF_BY_4_BIT)   {
	    print_flag("overrun 4", &indent);
	}
    }
    if (isr & OFF_BY_2_BIT)   {
	print_flag("overrun 2", &indent);
    }
    if (isr & OFF_BY_1_BIT)   {
	print_flag("overrun 1", &indent);
    }
    if (type == L4)   {
	if (isr & TAIL_INT_BIT)   {
	    print_flag("tail bit consumed", &indent);
	}
	if (isr & WDOG_INT_BIT)   {
	    print_flag("watch dog timer expired", &indent);
	}
	if (isr & TIME_INT_BIT)   {
	    print_flag("interrupt timer expired", &indent);
	}
	if (isr & DMA_INT_BIT)   {
	    print_flag("host DMA complete", &indent);
	}
    }
    if ((type == L7) || (type == L9))   {
	if (isr & TIME1_INT_BIT)   {
	    print_flag("interrupt timer 1 expired", &indent);
	}
	if (isr & TIME0_INT_BIT)   {
	    print_flag("interrupt timer 0 expired", &indent);
	}
    }
    if (type == L7)   {
	if (isr & LAN9_SIG_BIT)   {
	    print_flag("lan sig bit 9 set", &indent);
	}
	if (isr & LAN8_SIG_BIT)   {
	    print_flag("lan sig bit 8 set", &indent);
	}
    }
    if (isr & SEND_INT_BIT)   {
	print_flag("send DMA complete", &indent);
    }
    if (isr & BUFF_INT_BIT)   {
	print_flag("receive buffer full", &indent);
    }
    if (isr & RECV_INT_BIT)   {
	print_flag("receive DMA complete", &indent);
    }
    if (type == L4)   {
	if (isr & BYTE_RDY_BIT)   {
	    print_flag("byte ready", &indent);
	}
    }
    if ((type == L7) || (type == L9))   {
	if (isr & HEAD_INT_BIT)   {
	    print_flag("head received", &indent);
	}
    }
    if (isr != 0)   {
	printf("\n");
    }

}  /* end of printISR() */

/******************************************************************************/
/*
** printCounters()
** Print the values of the counters in shared memory.
*/
void
printCounters(mcpshmem_t *mcpshmem, int verbose, int type)
{

    printf("    Counters\n");
    printf("        Sends               %12ld     Receives             %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.sends),
	(unsigned long int)ntohl(mcpshmem->counters.rcvs));
    printf("        Dumped              %12ld     CRC on dumped msg    %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.dumped),
	(unsigned long int)ntohl(mcpshmem->counters.dumped_crc));
    printf("        Network resets      %12ld     CRC errors           %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.fres),
	(unsigned long int)ntohl(mcpshmem->counters.crc));
    printf("        LANai resets        %12ld     Host delayed pkt     %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.reset),
	(unsigned long int)ntohl(mcpshmem->counters.hst_dly));
    if (type == L7)   {
	printf("        Send pkt blocked    %12ld\n",
	    (unsigned long int)ntohl(mcpshmem->counters.link2));
    }
    printf("        Send pkt timeouts   %12ld     Truncated rcv pkts   %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.send_timeout),
	(unsigned long int)ntohl(mcpshmem->counters.truncated));
    printf("        Mem parity errors   %12ld     Too long rcv pkts    %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.mem_parity),
	(unsigned long int)ntohl(mcpshmem->counters.toolong));
    printf("        Bytes dumped        %12ld     Wrong protocol msgs  %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.dumped_num),
	(unsigned long int)ntohl(mcpshmem->counters.wrngprtcl));
    printf("        Myricom data packets%12ld     Myricom map packets  %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.MyriData),
	(unsigned long int)ntohl(mcpshmem->counters.MyriMap));
    printf("        Myricom probe pkts  %12ld     Myricom option pkts  %12ld\n",
	(unsigned long int)ntohl(mcpshmem->counters.MyriProbe),
	(unsigned long int)ntohl(mcpshmem->counters.MyriOption));

}  /* end of printCounters() */

/******************************************************************************/
/*
** Print benchmark results that were optained during MCP load
*/
void
printBench(mcpshmem_t *mcpshmem)
{

int i;


    printf("    nop time            %12dns   "
	"LED time             %12dns\n",
	(int)ntohl(mcpshmem->benchmark.nop),
	(int)ntohl(mcpshmem->benchmark.led));
    printf("    ISR write time      %12dns   "
	"ISR read time        %12dns\n",
	(int)ntohl(mcpshmem->benchmark.isr_w),
	(int)ntohl(mcpshmem->benchmark.isr_r));
    printf("    Memory wrt time     %12dns   "
	"Memory read time     %12dns\n",
	(int)ntohl(mcpshmem->benchmark.isr_w),
	(int)ntohl(mcpshmem->benchmark.isr_r));
    printf("\n    Host to LANai DMA tests, %d trials each\n",
	DMA_TEST_CNT);
    printf("     Size       total time    Avg BW       Avg time\n");
    for (i= 0; i < DMA_TEST_MAX; i++)   {
	if (mcpshmem->e2l_len[i] == 0)   {
	    continue;
	}
	printf("    %5d B      %6.0f us    %5.1f MB/s %6.0f us\n",
	    (unsigned int)ntohl(mcpshmem->e2l_len[i]),
	    (float)ntohl(mcpshmem->e2l_result[i]) / 2,
	    ((1.0 / ((float)ntohl(mcpshmem->e2l_result[i]) * 0.0000005)) *
	    DMA_TEST_CNT * (float)ntohl(mcpshmem->e2l_len[i])) / 1000000.0,
	    ((float)ntohl(mcpshmem->e2l_result[i]) / 2.0) / DMA_TEST_CNT);
    }
    printf("\n    LANai to Host DMA tests, %d trials each\n",
	DMA_TEST_CNT);
    printf("     Size       total time    Avg BW       Avg time\n");
    for (i= 0; i < DMA_TEST_MAX; i++)   {
	if (mcpshmem->l2e_len[i] == 0)   {
	    continue;
	}
	if (mcpshmem->l2e_result[i] == 0)   {
	    printf("    %5d B      FAILED\n",
		(unsigned int)ntohl(mcpshmem->l2e_len[i]));
	} else   {
	    printf("    %5d B      %6.0f us    %5.1f MB/s %6.0f us\n",
		(unsigned int)ntohl(mcpshmem->l2e_len[i]),
		(float)ntohl(mcpshmem->l2e_result[i]) / 2,
		((1.0 / ((float)ntohl(mcpshmem->l2e_result[i]) * 0.0000005)) *
		DMA_TEST_CNT * (float)ntohl(mcpshmem->l2e_len[i])) / 1000000.0,
		((float)ntohl(mcpshmem->l2e_result[i]) / 2.0) / DMA_TEST_CNT);
	}
    }
}  /* end of printBench() */

/******************************************************************************/
/*
** printEvents()
*/
void
printEvents(mcpshmem_t *mcpshmem, int unit, int type)
{

int i;
unsigned long s1, s2;
long double delta;


    printf("    Events (%ld)\n", ntohl(mcpshmem->event_num));

    for (i= 0; i < EVENT_MAX; i++)   {
	switch (ntohl(mcpshmem->eventlog[i].mcp_event))   {
	    case EVENT_CRC:	printf("    %2d CRC:      ", i); break;
	    case EVENT_HSTORUN:	printf("    %2d HSTORUN:  ", i); break;
	    case EVENT_TRUNC:	printf("    %2d TRUNC:    ", i); break;
	    case EVENT_LONG:	printf("    %2d LONG:     ", i); break;
	    case EVENT_MINT:	printf("    %2d MEMINT:   ", i); break;
	    case EVENT_MPAR:	printf("    %2d MEMPAR:   ", i); break;
	    case EVENT_NRES:	printf("    %2d NRESET:   ", i); break;
	    case EVENT_LINK2:	printf("    %2d LINK2:    ", i); break;
	    case EVENT_WRNGPROT:printf("    %2d WRNGPROT: ", i); break;
	    case EVENT_NONE:	continue; break;
	    default:		printf("    %2d ???:      ", i); break;
	}
	s1= ntohl(mcpshmem->eventlog[i].t0);
	s2= ntohl(mcpshmem->eventlog[i].t1);
	delta= s1 * 0.0000005 + s2 * 0.0000005 * 4294967295.0;

	printf("after %8.6Lfs, %ld rcvs, %ld snds\n", delta,
	    (unsigned long int)ntohl(mcpshmem->eventlog[i].rcvs),
	    (unsigned long int)ntohl(mcpshmem->eventlog[i].snds));
	printf("       RMP 0x%08lx,  RML 0x%08lx,  len %ld\n",
	    ntohl(mcpshmem->eventlog[i].RMPvalue),
	    ntohl(mcpshmem->eventlog[i].RMLvalue),
	    ntohl(mcpshmem->eventlog[i].len));
	printf("       type/len 0x%08lx  xxx 0x%08lx  ver 0x%08x  src/ptype 0x%08x\n",
	    ntohl(mcpshmem->eventlog[i].word0),
	    ntohl(mcpshmem->eventlog[i].word1),
	    mcpshmem->eventlog[i].word2,
	    mcpshmem->eventlog[i].word3);
	printf("       msgID    0x%08x  len 0x%08x  seq 0x%08x  msgNum    0x%08x\n",
	    mcpshmem->eventlog[i].word4,
	    mcpshmem->eventlog[i].word5,
	    mcpshmem->eventlog[i].word6,
	    mcpshmem->eventlog[i].word7);
	printf("       1st data 0x%08x  xxx 0x%08x  crc 0x%08x  xxx       0x%08x\n",
	    mcpshmem->eventlog[i].word8,
	    mcpshmem->eventlog[i].word9,
	    mcpshmem->eventlog[i].word10,
	    mcpshmem->eventlog[i].word11);
	printISR(ntohl(mcpshmem->eventlog[i].isr), unit, type);
	printf("   ----------------------------------------------------------------------------\n\n");
    }
    
}  /* end of printEvents() */

/******************************************************************************/
/*
** printFault()
** print the fault register.
*/
void
printFault(int fault, mcpshmem_t *mcpshmem, int verbose, int unit, int type)
{
    printf("    Last fault (%4d)                        Last reset IMR         "
	    "0x%08x\n", fault, (unsigned)ntohl(mcpshmem->reset_imr));

    if ((fault > 100) && (fault < 200))   {
	/* This is an assertion */
	printf("        Assertion a%2d was triggered          Last reset ISR   "
		"      0x%08x\n", fault - 100,
		(unsigned)ntohl(mcpshmem->reset_isr));
    } else   {

	switch (fault)   {
	    case NO_FAULT:
		printf("        No fault                             Last "
			"reset ISR         0x%08x\n",
			(unsigned)ntohl(mcpshmem->reset_isr));
		break;
	    default:
		printf("        Unknown fault                        Last "
			"reset ISR         0x%08x\n",
			(unsigned)ntohl(mcpshmem->reset_isr));
	}
    }

    printf("    Last fault ISR 0x%08x    \n",
	(unsigned)ntohl(mcpshmem->fault_isr));
    if (verbose)   {
	printISR(ntohl(mcpshmem->fault_isr), unit, type);
    }

}  /* end of printFault() */

/******************************************************************************/
/*
** printOwner()
** Who started this MCP and when?
*/
void
printOwner(mcpshmem_t *mcpshmem)
{

struct passwd *pw;
char *owner;
time_t t, diff;


    pw= getpwuid(mcpshmem->uid);
    if (pw == NULL)   {
	owner= "unknown";
    } else   {
	owner= pw->pw_name;
    }

    /*
    ** If the MCP is not initialized, mcpshmem->stime will be garbage.
    ** passing the address of mcpshmem->stime directly to ctime() hangs
    ** in that case!
    */
    t= (time_t)mcpshmem->stime;
    diff= time(NULL) - t;
    printf("    MCP was started %ds ago by %s on %s\n", (int)diff, owner, ctime(&t));

}  /* end of printOwner() */

/******************************************************************************/

void
printMisc(mcpshmem_t *mcpshmem, int verbose)
{

int rstate, sstate;
int i;


    printf("    Misc.\n");
    rstate= ntohl(mcpshmem->rcv_state);
    sstate= ntohl(mcpshmem->snd_state);
    if (rstate > 0)   {
	/*
	** State debugging is turned on.
	** State numbers on MCP start at 1, so we subtract 1 for the  humans
	*/
	printf("        Current send state           S%02d     "
	    "Current receive state         R%02d\n", sstate - 1, rstate - 1);
    } else   {
	printf("        State machine debugging is off\n");
    }
    printf("        My node id          %12ld     "
	"hstshmem addr          0x%08x\n", ntohl(mcpshmem->my_pnid),
	(unsigned)ntohl(mcpshmem->hstshmem));
    printf("        Self test status    ");
    switch ((int)ntohl(mcpshmem->self_test))   {
	case SELF_TEST_FAIL:	printf("%12s", "disconnected"); break;
	case SELF_TEST_INVAL:	printf("%12s", "corrupted"); break;
	case SELF_TEST_OTHER:	printf("%12s", "other pkt"); break;
	case SELF_TEST_UNKNOWN:	printf("%12s", "unknown"); break;
	case SELF_TEST_OK:	printf("%12s", "OK"); break;
	case SELF_TEST_NOSWITCH:	printf("%12s", "direct cnct"); break;
	default:		printf("%12s", "invalid");
    }
    printf("\n");

    if (verbose)   {
	for (i= 0; i < MAX_DEBUG / 2; i++)   {
	    printf("        dbg[%2d] 0x%08x (%12d)    "
		"dbg[%2d] 0x%08x (%12d)\n",
	    i*2, (int)ntohl(mcpshmem->debug[i*2]),
	    (int)ntohl(mcpshmem->debug[i*2]),
	    (i*2)+1, (int)ntohl(mcpshmem->debug[(i*2)+1]),
	    (int)ntohl(mcpshmem->debug[(i*2)+1]));
	}
    }

}  /* end of printMisc() */

/******************************************************************************/
#define MCP_OFFSET(mcpaddr, hstaddr) \
    (mcpaddr - ((long)hstaddr - (long)mcpshmem + mcp_mcpshmem))

void
printRegs(mcpshmem_t *mcpshmem, int unit)
{

long distance;
long rcv_buf_A_off, rcv_buf_B_off;
long snd_buf_A_off, snd_buf_B_off;
long x, mcp_mcpshmem;
char *buf_name;
long offset;
int type;


    type= get_lanai_type(unit, FALSE);

    printf("    LANai special registers\n");
    if (type == L0)   {
	printf("        Unsupported LANai type. Decoding not possible\n");
	return;
    }

    mcp_mcpshmem= get_mcp_mcpshmem(unit);
    snd_buf_A_off= (long)mcpshmem->snd_buf_A - (long)mcpshmem;
    snd_buf_B_off= (long)mcpshmem->snd_buf_B - (long)mcpshmem;
    rcv_buf_A_off= (long)mcpshmem->rcv_buf_A - (long)mcpshmem;
    rcv_buf_B_off= (long)mcpshmem->rcv_buf_B - (long)mcpshmem;

    if (type == L4)   {
	printf("        CKS\t0x%08x   Internet checksum (EBUS -> LBUS DMA)\n",
	    getCKS(unit, type));
	printf("        DMA_CTR\t0x%08x   EBUS DMA word counter\n",
	    getDMA_CTR(unit, type));
    }
    if (type == L7)   {
	printf("        CTR\t0x%08x   EBUS DMA counter\n",
	    getDMA_CTR(unit, type));
    }
    if (type == L9)   {
	printf("        CPUC\t0x%08x   CPU clock counter\n",
	    getDMA_CTR(unit, type)); /* It's really getCPUC() */
    }
    if (type == L4)   {
	printf("        EAR\t0x%08x   EBUS DMA host address\n",
	    getEAR(unit, type));
    }
    printf("        EIMR\t0x%08x   Host interrupt mask\n",
	getEIMR(unit, type));
    printf("        ISR\t0x%08x   Interrupt status register\n",
	getISR(unit, type));
    if (type == L4)   {
	printf("        IT\t0x%08x   Interrupt timer\n", getIT1(unit, type));
    }
    if ((type == L7) || (type == L9))   {
	printf("        IT0\t0x%08x   Interrupt timer 0\n", getIT0(unit, type));
	printf("        IT1\t0x%08x   Interrupt timer 1\n", getIT1(unit, type));
	printf("        IT2\t0x%08x   Interrupt timer 2\n", getIT2(unit, type));
    }
    printf("        LAR\t0x%08x   EBUS DMA local address\n",getLAR(unit, type));

    if ((type == L7) || (type == L9))   {
	printf("        RMC\t0x%08x   Receive-DMA header CRC\n",
	    getRMC(unit, type));
	printf("        RMW\t0x%08x   Receive-DMA header\n",
	    getRMW(unit, type));
    }
    x= getRML(unit, type) - (long)mcp_mcpshmem;
    if ((x  - rcv_buf_B_off) < 0)   {
	distance= x - rcv_buf_A_off + 4;
	buf_name= "rcv buf A";
	offset= MCP_OFFSET(getRMP(unit, type), mcpshmem->rcv_buf_A);
    } else   {
	distance= x - rcv_buf_B_off + 4;
	buf_name= "rcv buf B";
	offset= MCP_OFFSET(getRMP(unit, type), mcpshmem->rcv_buf_B);
    }
    if ((type == L7) || (type == L9))   {
	distance= distance - 4;
    }
    printf("        RML\t0x%08x   Receive memory limit (%ld bytes into %s)\n",
	getRML(unit, type), distance, buf_name);
    printf("        RMP\t0x%08x   Receive mem pointer (%ld bytes received)\n",
	getRMP(unit, type), offset);

    printf("        RTC\t0x%08x   Real time clock\n",
	getRTC(unit, type));

    if ((type == L7) || (type == L9))   {
	printf("        SMC\t0x%08x   Send-DMA header CRC\n",
	    getSMC(unit, type));
	printf("        SMH\t0x%08x   Send-DMA routing header\n",
	    getSMH(unit, type));
    }
    x= getSML(unit, type) - (long)mcp_mcpshmem;
    if ((x  - snd_buf_B_off) < 0)   {
	distance= x - snd_buf_A_off + 4;
	buf_name= "snd buf A";
	offset= MCP_OFFSET(getSMP(unit, type), mcpshmem->snd_buf_A);
    } else   {
	distance= x - snd_buf_B_off + 4;
	buf_name= "snd buf B";
	offset= MCP_OFFSET(getSMP(unit, type), mcpshmem->snd_buf_B);
    }
    if ((type == L7) || (type == L9))   {
	distance= distance - 4;
	offset= offset - 8;
    }
    printf("        SML\t0x%08x   Send memory limit (%ld bytes into %s)\n",
	getSML(unit, type), distance, buf_name);
    printf("        SMLT\t0x%08x   Send memory limit (terminate message)\n",
	getSMLT(unit, type));
    printf("        SMP\t0x%08x   Send memory pointer (%ld bytes sent)\n",
	    getSMP(unit, type), offset);

}  /* end of printRegs() */

/******************************************************************************/

#if 0
void
printPumaHead(PUMA_MSG_HEAD *phead, char *title)
{

    printf("    %s\n", title);
    printf("        dst_matchbits 0x%08x%08x     ret_matchbits  0x%08x%08x\n",
	phead->dst_matchbits.ints.i1,
	phead->dst_matchbits.ints.i0,
	phead->return_matchbits.ints.i1,
	phead->return_matchbits.ints.i0);
    printf("        dst_pid                    %5d     "
	"dst_nid                     %5d\n",
	phead->dst_pid, phead->dst_nid);
    printf("        src_nid                    %5d     "
	"dst_portal                  %5d\n",
	phead->src_nid, phead->dst_portal);
    printf("        operation                  %5d     "
	"msg_len                %10d\n",
	phead->operation, phead->msg_len);
    printf("        src_grp                    %5d     "
	"src_rank                    %5d\n",
	phead->src_grp, phead->src_rank);
    printf("        src_pid                    %5d     "
	"acl_index                   %5d\n",
	phead->src_pid, phead->acl_index);
    printf("        return_portal              %5d     "
	"dst_offset             %10d\n",
	phead->return_portal, phead->dst_offset);
    printf("        reply_offset          %10d     "
	"reply_len              %10d\n",
	phead->reply_offset, phead->reply_len);
    printf("        user_data             0x%02x%02x%02x%02x     "
	"%02x%02x%02x%02x     %02x%02x%02x%02x\n",
	phead->user_data[0], phead->user_data[1], phead->user_data[2],
	phead->user_data[3], phead->user_data[4], phead->user_data[5],
	phead->user_data[6], phead->user_data[7], phead->user_data[8],
	phead->user_data[9], phead->user_data[10], phead->user_data[11]);

}  /* end of printPumaHead() */
#endif

/******************************************************************************/

void
printHstMisc(hstshmem_t *hstshmem)
{

    printf("    Misc.\n");
    if (hstshmem->rdy_to_snd)   {
	printf("     rdy_to_snd                   true\n");
    } else   {
	printf("     rdy_to_snd                  false\n");
    }

    printf("        my_pnid               %10d     LANai2host    ",
	hstshmem->my_pnid);
    switch (ntohl(hstshmem->LANai2host))   {
	case NO_REASON:		printf("     no int pending\n"); break;
	case INT_RCV_BEGUN:	printf("          rcv begun\n"); break;
	case GET_HSTSHMEM:	printf("       get hstshmem\n"); break;
	case INT_RCV_DONE:	printf("      rcv done (OK)\n"); break;
	case INT_MCP_FAULT:	printf("          MCP fault\n"); break;
	default:        	printf("            unknown\n"); break;
    }

}  /* end of printHstMisc */

/******************************************************************************/

void
printHstCounters(hstshmem_t *hstshmem)
{

    printf("    Message and byte counts\n");
    printf("        Msgs sent   %20ld     Bytes sent   %20ld\n",
	hstshmem->total_snd_msgs, hstshmem->total_snd_bytes);
    printf("        Msgs rcvd   %20ld     Bytes rcvd   %20ld\n",
	hstshmem->total_rcv_msgs, hstshmem->total_rcv_bytes);
    printf("    Unaligned messages\n");
    printf("        Snd head    %20ld     Snd tail     %20ld\n",
	hstshmem->unaligned_snd_head, hstshmem->unaligned_snd_tail);
    printf("        Rcv head    %20ld     Rcv tail     %20ld\n",
	hstshmem->unaligned_rcv_head, hstshmem->unaligned_rcv_tail);

}  /* end of printHstCounters */

/******************************************************************************/
