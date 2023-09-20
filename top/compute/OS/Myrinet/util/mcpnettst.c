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
** $Id: mcpnettst.c,v 1.3 2001/08/22 16:45:15 pumatst Exp $
** This test assumes the LANai is connected to a Myrinet switch (or
** has a loop-back cable connected when the -noswitch option is given).
** The test sends data from LANai memory through the switch back into
** another portion of LANai memory.
** The test assumes LANai memory is OK (use mcpmemtst to make sure),
** and it also assumes host to LANai memory transfers are OK (hswap
** from Myricom can help here).
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "lanai_device.h"
#include "cplant.h"



/*
** Command line options
*/
static struct option opts[]=
{
    {"unit",		required_argument,	0, 'u'},
    {"verbose",		no_argument,		0, 'v'},
    {"suppress",	no_argument,		0, 's'},
    {"help",		no_argument,		0, 'h'},
    {"continue",	required_argument,	0, 'c'},
    {"noswitch",	no_argument,		0, 'n'},
    {"loop",		required_argument,	0, 'l'},
    {"length",		required_argument,	0, 'L'},
    {0, 0, 0, 0}
};


/*
** Local functions
*/
void usage(char *pname);
void setup_lanai(int unit, int verbose);
int xfer(int unit, int no_switch, unsigned int rcvbuf, unsigned int sndbuf,
    unsigned int len, int verbose);
int all0s(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit);
int all1s(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit);
int alternate(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit);
int prnd(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit);


/*
** External Functions
*/
extern int map_lanai(char *pname, int verbose, int unit);


#define LOOP_HDR		(0x18001234)


/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
main(int argc, char *argv[] )
{

int unit;
int verbose;
int suppress;
int cont;
int noswitch;
int loop;
int loop_cnt;
int iter;
int tstlen;

unsigned int mcpmem_len;
unsigned int *mcpmem_start;
unsigned long offset;
unsigned int *mcpmem_end;
unsigned int *recvbuf;
unsigned int *sendbuf;

extern char *optarg;
extern int optind;
int error;
int index;
int ch;


    /* Set the defaults */
    verbose= 0;
    unit= 0;
    suppress= FALSE;
    cont= 0;
    noswitch= FALSE;
    loop= 1;
    tstlen= 8192;
    error= FALSE;

    while (TRUE)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'u':
		unit= strtol(optarg, NULL, 10);
		break;
	    case 'v':
		verbose++;
		break;
	    case 'c':
		cont= strtol(optarg, NULL, 10);
		break;
	    case 'n':
		noswitch= TRUE;
		break;
	    case 'L':
		tstlen= strtol(optarg, NULL, 10);
		if (tstlen < 0)   {
		    fprintf(stderr, "%s: -length (%d) must not be < 0\n",
			argv[0], tstlen);
		    error= TRUE;
		}
		break;
	    case 'l':
		loop= strtol(optarg, NULL, 10);
		if (loop < 0)   {
		    fprintf(stderr, "%s: -loop (%d) must not be < 0\n",
			argv[0], loop);
		    error= TRUE;
		}
		break;
	    case 's':
		suppress= TRUE;
		break;
	    case 'h':
	    case '?':
	    default:
		error= TRUE;
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }


    if (!suppress)   {
	printf("This program runs network DMA tests on the Myrinet card. Any "
	    "information on the\n");
	printf("card, including running programs (MCP). Will be destroyed.\n");
	printf("You will have to reload the MCP using the mcpload program\n");
	printf("    Do you want to continue? (Type y or Y. Anything else "
	    "aborts) ");
	ch= getchar();
	if ((ch != 'y') && (ch != 'Y'))   {
	    printf("Aborting...\n");
	    exit(0);
	}
    }

    if (map_lanai(argv[0], (verbose > 1), unit) != OK)   {
	exit(-1);
    }
    if (verbose)   {
	printf("Successfully mapped LANai[%d]\n", unit);
    }

    mcpmem_start= (unsigned int *) (&LANAI[unit][0]);
    offset= (unsigned long)mcpmem_start;
    mcpmem_len= lanai_memory_size(unit);
    mcpmem_end= (unsigned int *) ((unsigned long)mcpmem_start + mcpmem_len -
		    sizeof(int));

    /* Reset the LANai, but let it run afterwards */
    lanai_reset_unit(unit, 1);	/* Into reset */
    *mcpmem_start= htonl(0xE0000000);		/* bt 0 */
    *(mcpmem_start + 1)= htonl(0x00000001);	/* nop */
    lanai_reset_unit(unit, 0);			/* Out of reset */
    if (verbose)   {
	printf("Reset LANai[%d]\n", unit);
    }

    setup_lanai(unit, verbose);
    if (verbose)   {
	printf("Successfully initialized LANai[%d]\n", unit);
    }

    if (verbose)   {
	unsigned int *cur;
	printf("First few words of LANai memory:\n");
	cur= mcpmem_start;
	printf("    0: 0x%08lx   0x%08lx   0x%08lx   0x%08lx   0x%08lx\n",
	    ntohl(*cur++), ntohl(*cur++), ntohl(*cur++), ntohl(*cur++), ntohl(*cur++));
    }

    /* Default send and receive buffers */
    if (tstlen > ((mcpmem_len / 2) - 1024))   {
	fprintf(stderr, "ERROR: test length %d cannot be more than 1/2 of "
	    "available LANai memory - 1024 = %d\n", tstlen, (mcpmem_len / 2) - 1024);
	exit(-1);
    }
    sendbuf= mcpmem_start + (1024 / sizeof(int));
    recvbuf= mcpmem_start + ((mcpmem_len / 2) / sizeof(int));

    printf("Mappings:\n");
    printf("    MCP memory start             0x%08x (%12d)   is   %p\n",
	0, 0, mcpmem_start);
    printf("    MCP memory end               0x%08x (%12d)   is   %p\n",
	mcpmem_len - 4, mcpmem_len - 4, mcpmem_end);
    printf("    Send buffer start            0x%08lx (%12ld)   is   %p\n",
	(unsigned long)sendbuf - offset, (unsigned long)sendbuf - offset,
	sendbuf);
    printf("    Receive buffer start         0x%08lx (%12ld)   is   %p\n",
	(unsigned long)recvbuf - offset, (unsigned long)recvbuf - offset,
	recvbuf);
    printf("    Test length is               %d\n", tstlen);


    printf("Tests:\n");

    if (loop == 0)   {
	loop_cnt= 1;
    } else   {
	loop_cnt= loop;
    }

    iter= 0;
    error= FALSE;
    while (loop_cnt)   {
	printf("    All zeros fills memory with 0's and verifies correctness\n");
	if (all0s(sendbuf, recvbuf, offset, cont, verbose, noswitch, tstlen,
		unit))   {
	    printf("    All 0's test PASSED\n");
	} else   {
	    printf("    All 0's test FAILED\n");
	    error= TRUE;
	}

	printf("    All ones fills memory with 1's and verifies correctness\n");
	if (all1s(sendbuf, recvbuf, offset, cont, verbose, noswitch, tstlen,
		unit))   {
	    printf("    All 1's test PASSED\n");
	} else   {
	    printf("    All 1's test FAILED\n");
	    error= TRUE;
	}

	printf("    Alternate tests memory with 101010... and 010101...\n");
	if (alternate(sendbuf, recvbuf, offset, cont, verbose, noswitch, tstlen,
		unit))   {
	    printf("    Alternate test PASSED\n");
	} else   {
	    printf("    Alternate test FAILED\n");
	    error= TRUE;
	}

	printf("    Random test fills memory with pseudo random numbers\n");
	if (prnd(sendbuf, recvbuf, offset, cont, verbose, noswitch, tstlen,
		unit))   {
	    printf("    Random test PASSED\n");
	} else   {
	    printf("    Random test FAILED\n");
	    error= TRUE;
	}

	loop_cnt--;
	if ((loop_cnt == 0) && (loop == 0))   {
	    loop_cnt= 1;
	}
	iter++;
	if (iter == 1)   {
	    printf("Tests completed once\n");
	} else if (iter == 2)   {
	    printf("Tests completed twice\n");
	} else   {
	    printf("Tests completed %d times\n", iter);
	}
	if (error)   {
	    if (loop_cnt > 0)   {
		printf("Aborting program because of errors in tests\n");
	    }
	    break;
	}
    }

    return 0;

}  /* end of main() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

void
usage(char *pname)
{

    fprintf(stderr,
	"Usage: %s [-verbose] [-unit <num>] [-continue <cnt>] [-suppress]\n"
	    , pname);
    fprintf(stderr, "       [-loop <cnt>] [-noswitch] [-length <len>] [-help]\n");
    fprintf(stderr, "   -verbose   Display progress information. This option "
	"can be repeated for more\n");
    fprintf(stderr, "              detailed information\n");
    fprintf(stderr, "   -unit      Myrinet interface unit number. Default "
	"is 0\n");
    fprintf(stderr, "   -continue  Don't stop after first error in each "
	"test. Display up to <cnt> errors\n");
    fprintf(stderr, "   -suppress  Suppress warning message about overwriting "
	"LANai memory\n");
    fprintf(stderr, "   -loop      Repeat tests <cnt> times. 0 means infinte "
	"loop\n");
    fprintf(stderr, "   -noswitch  No Myrinet switch. Loop-back cable is being "
	"used\n");
    fprintf(stderr, "   -length    Test message is len bytes long. Default is "
	"8192 bytes\n");
    fprintf(stderr, "   -help      Display this usage information and exit\n");

}  /* end of usage() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

void
setup_lanai(int unit, int verbose)
{

    MYRINET(unit)= htonl(CRC_ENABLE_BIT);
    TIMEOUT(unit)= htonl(3);
    DMA_STS(unit)= htonl(15);
    LED(unit)= htonl(1);
    RTC(unit)= htonl(0);
    IT(unit)= htonl(0xffffffff);
    VERSION(unit)= htonl(3);

    while (ntohl(RTC(unit)) < 45000)   {
	/* Spec says to wait > 10ms for SAN link to settle in */
    }

    EIMR(unit)= htonl(0);
    ISR(unit)= htonl(0xffffffff);

    if (verbose)   {
	printf("    setup_lanai() ISR is now 0x%08lx\n", ntohl(ISR(unit)));
    }

}  /* end of setup_lanai() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */
/*
** Wait for an MCP receive to finish
*/
void
wait_rcv(int unit, int verbose)
{

int i;


    /* Give it a little bit of time */
    for (i= 0; i < 15000; i++)   {
	if ((ntohl(ISR(unit)) & RECV_INT_BIT) == RECV_INT_BIT)   {
	    break;
	}
    }

    /* Check if the data is here */
    if ((ntohl(ISR(unit)) & RECV_INT_BIT) != RECV_INT_BIT)   {
	/* Give it another try */
	printf("    Waiting for the receive to complete...\n");
	for (i= 0; i < 10; i++)   {
	    if ((ntohl(ISR(unit)) & RECV_INT_BIT) == RECV_INT_BIT)   {
		break;
	    }

	    /* wait */
	    sleep(2);
	    printf("        ISR is now 0x%08lx (still waiting...)\n",
		ntohl(ISR(unit)));
	}
    }

    if ((ntohl(ISR(unit)) & RECV_INT_BIT) != RECV_INT_BIT)   {
	printf("    receive failed. ISR is now 0x%08lx\n", ntohl(ISR(unit)));
	exit(-1);
    } else   {
	if (verbose > 1)   {
	    printf("    receive complete. ISR is now 0x%08lx\n",
		ntohl(ISR(unit)));
	}
    }

}  /* end of wait_rcv() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
xfer(int unit, int no_switch, unsigned int rcvbuf, unsigned int sndbuf,
    unsigned int len, int verbose)
{

unsigned long offset;


    offset= (unsigned long) (&LANAI[unit][0]);

    /* Setup the receive DMA engine */
    RMP(unit)= htonl(rcvbuf);
    RML(unit)= htonl(rcvbuf + len + 4);


    /* Send the header and the route */
    if (!no_switch)   {
	while ((ntohl(ISR(unit)) & SEND_RDY_BIT) != SEND_RDY_BIT) ;
	SB(unit)= htonl(0x80);
    }
    while ((ntohl(ISR(unit)) & SEND_RDY_BIT) != SEND_RDY_BIT) ;
    SW(unit)= htonl(LOOP_HDR);


    /* send the data using the send DMA engine */
    while ((ntohl(ISR(unit)) & SEND_RDY_BIT) != SEND_RDY_BIT) ;
    SMP(unit)= htonl(sndbuf);
    SMLT(unit)= htonl(sndbuf + len - 4);

    /* Wait for the transfer to complete */
    if (verbose > 2)   {
	printf("    ISR is now 0x%08lx\n", ntohl(ISR(unit)));
	printf("    RMP is now 0x%08lx\n", ntohl(RMP(unit)));
	printf("    RML is now 0x%08lx\n", ntohl(RML(unit)));
	printf("    SMP is now 0x%08lx\n", ntohl(SMP(unit)));
	printf("    SML is now 0x%08lx\n", ntohl(SML(unit)));
    }

    wait_rcv(unit, verbose);

    while ((ntohl(RMP(unit)) != rcvbuf + len + 8) ||
	    (ntohl(*((unsigned int *)(rcvbuf + offset))) != LOOP_HDR))   {
	printf("    xfer() ***** Header is 0x%08lx, expected 0x%08x\n",
	    ntohl(*((unsigned int *)(rcvbuf + offset))), LOOP_HDR);

	printf("    xfer() ***** RMP is 0x%08lx, expected 0x%08x. Trying to "
	    "recover\n", ntohl(RMP(unit)), rcvbuf + len + 8);

	RMP(unit)= htonl(rcvbuf);
	RML(unit)= htonl(rcvbuf + len + 4);
	wait_rcv(unit, verbose);
    }

    return 0;

}  /* end of xfer() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
all0s(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit)
{

unsigned int *current;
unsigned int *end;
int errcnt;
int cnt;


    if (verbose > 2)   {
	printf("    all0s(sendbuf %p, recvbuf %p, offset %ld, cont %d, "
	    "noswitch %d, len %d\n", sendbuf, recvbuf, offset, cont,
	    noswitch, len);
    }

    /* Initialize the send buffer */
    current= sendbuf;
    end= sendbuf + (len / sizeof(int) - 1);
    cnt= 0;
    while (current <= end)   {
	*current= 0;
	cnt++;
	current++;
    }
    if (verbose > 1)   {
	printf("    all0s() Written %d words of zeros\n", cnt);
    }

    #ifdef DEBUG
	current -= 10;   *current= 1;
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */

    /* Transfer the data tfrom the send buffer to the receive buffer */
    printf("    rcv buf 0x%08lx, snd buf 0x%08lx\n", (unsigned long)recvbuf - offset,
	(unsigned long)sendbuf - offset);
    xfer(unit, noswitch, (unsigned int)recvbuf - offset,
	(unsigned int)sendbuf - offset, len, verbose);

    /* Check the data in the receiver buffer */
    current= recvbuf + 1; /* +1 for the Myrinet header */
    end= current + (len / sizeof(int) - 1);
    errcnt= 0;
    while (current <= end)   {
	if (*current != 0)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0);
	    }
	    errcnt++;
	}
	current++;
    }

    if (errcnt > 0)   {
	printf("    There were %d errors in this test\n", errcnt);
	return FALSE;
    }

    return TRUE;

}  /* end of all0s() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
all1s(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit)
{

unsigned int *current;
unsigned int *end;
int cnt;
int errcnt;


    if (verbose > 2)   {
	printf("    all1s(sendbuf %p, recvbuf %p, offset %ld, cont %d, "
	    "noswitch %d, len %d\n", sendbuf, recvbuf, offset, cont,
	    noswitch, len);
    }

    /* Initialize the send buffer */
    current= sendbuf;
    end= sendbuf + (len / sizeof(int) - 1);
    cnt= 0;
    while (current <= end)   {
	*current= 0xffffffff;
	cnt++;
	current++;
    }
    if (verbose > 1)   {
	printf("    all1s() Written %d words of ones\n", cnt);
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */


    /* Transfer the data tfrom the send buffer to the receive buffer */
    xfer(unit, noswitch, (unsigned int)recvbuf - offset,
	(unsigned int)sendbuf - offset, len, verbose);

    /* Check the data in the receiver buffer */
    current= recvbuf + 1; /* +1 for the Myrinet header */
    end= current + (len / sizeof(int) - 1);
    errcnt= 0;
    while (current <= end)   {
	if (*current != 0xffffffff)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0xffffffff);
	    }
	    errcnt++;
	}
	current++;
    }

    if (errcnt > 0)   {
	printf("    There were %d errors in this test\n", errcnt);
	return FALSE;
    }

    return TRUE;

}  /* end of all1s() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
alternate(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit)
{

unsigned int *current;
unsigned int *end;
int errcnt;
int cnt;


    if (verbose > 2)   {
	printf("    alternate(sendbuf %p, recvbuf %p, offset %ld, cont %d, "
	    "noswitch %d, len %d\n", sendbuf, recvbuf, offset, cont,
	    noswitch, len);
    }

    /* Initialize the send buffer */
    current= sendbuf;
    end= sendbuf + (len / sizeof(int) - 1);
    cnt= 0;
    while (current <= end)   {
	*current= 0xAAAAAAAA;
	current++;
	cnt++;
	*current= 0x55555555;
	current++;
	cnt++;
    }
    if (verbose > 1)   {
	printf("    alternate() Written %d words of 0xAAAAAAAA and "
	    "0x55555555\n", cnt);
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */


    /* Transfer the data tfrom the send buffer to the receive buffer */
    xfer(unit, noswitch, (unsigned int)recvbuf - offset,
	(unsigned int)sendbuf - offset, len, verbose);

    /* Check the data in the receiver buffer */
    current= recvbuf + 1; /* +1 for the Myrinet header */
    end= current + (len / sizeof(int) - 1);
    errcnt= 0;
    while (current <= end)   {
	if (*current != 0xAAAAAAAA)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0xAAAAAAAA);
	    }
	    errcnt++;
	}
	current++;
	if (*current != 0x55555555)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0x55555555);
	    }
	    errcnt++;
	}
	current++;
    }



    /* Initialize the send buffer */
    current= sendbuf;
    end= sendbuf + (len / sizeof(int) - 1);
    cnt= 0;
    while (current <= end)   {
	*current= 0x55555555;
	current++;
	cnt++;
	*current= 0xAAAAAAAA;
	current++;
	cnt++;
    }
    if (verbose > 1)   {
	printf("    alternate() Written %d words of 0x55555555 and "
	    "0xAAAAAAAA\n", cnt);
    }

    #ifdef DEBUG
	current -= 11023; *current= 0x0a00000;
	current -= 11024; *current= 5;
	current -= 110;   *current= 1;
	current -= 13333; *current= 0xffff0000;
    #endif /* DEBUG */


    /* Transfer the data tfrom the send buffer to the receive buffer */
    xfer(unit, noswitch, (unsigned int)recvbuf - offset,
	(unsigned int)sendbuf - offset, len, verbose);

    /* Check the data in the receiver buffer */
    current= recvbuf + 1; /* +1 for the Myrinet header */
    end= current + (len / sizeof(int) - 1);
    while (current <= end)   {
	if (*current != 0x55555555)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0x55555555);
	    }
	    errcnt++;
	}
	current++;
	if (*current != 0xAAAAAAAA)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, 0xAAAAAAAA);
	    }
	    errcnt++;
	}
	current++;
    }

    if (errcnt > 0)   {
	printf("    There were %d errors in this test\n", errcnt);
	return FALSE;
    }

    return TRUE;

}  /* end of alternate() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
prnd(unsigned int *sendbuf, unsigned int *recvbuf, unsigned long offset, int cont,
    int verbose, int noswitch, int len, int unit)
{

unsigned int *current;
unsigned int *end;
unsigned int value;
unsigned int seed;
int cnt;
int errcnt;


    if (verbose > 2)   {
	printf("    prnd(sendbuf %p, recvbuf %p, offset %ld, cont %d, "
	    "noswitch %d, len %d\n", sendbuf, recvbuf, offset, cont,
	    noswitch, len);
    }

    seed= (unsigned int)time(NULL);
    srandom(seed);
    value= (unsigned int)random();
    if (verbose)   {
	printf("    prnd() Initial random value is 0x%08x (%d)\n", value, value);
    }

    /* Initialize the send buffer */
    current= sendbuf;
    end= sendbuf + (len / sizeof(int) - 1);
    cnt= 0;
    while (current <= end)   {
	*current= value;
	current++;
	cnt++;
	value= (unsigned int)random();
    }
    if (verbose > 1)   {
	printf("    prnd() Written %d words with random values\n", cnt);
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */


    /* Transfer the data tfrom the send buffer to the receive buffer */
    xfer(unit, noswitch, (unsigned int)recvbuf - offset,
	(unsigned int)sendbuf - offset, len, verbose);

    /* Check the data in the receiver buffer */
    current= recvbuf + 1; /* +1 for the Myrinet header */
    end= current + (len / sizeof(int)) - 1;

    srandom(seed);
    value= (unsigned int)random();
    errcnt= 0;
    while (current <= end)   {
	if (*current != value)   {
	    if ((errcnt == 0) || (errcnt < cont))   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, value);
	    }
	    errcnt++;
	}
	current++;
	value= (unsigned int)random();
    }

    if (errcnt > 0)   {
	printf("    There were %d errors in this test\n", errcnt);
	return FALSE;
    }

    return TRUE;

}  /* end of prnd() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */
