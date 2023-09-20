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
** $Id: mcpload.c,v 1.35 2002/02/06 01:08:25 pumatst Exp $
** Simple MCP loader. It can load MCPs in LANai executable format or the dat
** format. It handshakes with the MCP once it is up and reports whether the
** MCP has been succesfully started.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lanai_device.h"
#include "myriInterface.h"
#include "../MCP/MCPshmem.h"
#include "defines.h"
#include "common.h"
#include "route_util.h"

#ifdef MCP_FROM_ARRAY
/* in this case, link "-lLanaiDev" instead of "-lLanaiDevice -lbfd" */
/* mcp_array.h:  % gendat --array mcpname > mcp_array.h */
#include "MyrinetPCI.h"
#include "mcp_array.h"
int local_lanai_load_reset(const unsigned unit);
#endif


/*
** Command line options
*/
static struct option opts[]=
{
    {"mcp",		required_argument,	0, 'm'},
    {"unit",		required_argument,	0, 'u'},
    {"verbose",		no_argument,		0, 'v'},
    {"pnid",		required_argument,	0, 'p'},
    {"route",		required_argument,	0, 'r'},
    {"bench",		no_argument,		0, 'b'},
    {"dma",		required_argument,      0, 'd'},
    {0, 0, 0, 0}
};


/*
** Local functions
*/
void usage(char *pname);
void test_rtc(int verbose, int unit, int btype);
void do_bench(int unit, mcpshmem_t *mcpshmem, int verbose, int btype);
void start(void);
double stop(void);
void tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0);
void check_mcp_access(char **mcp_name, int btype, char *pname, int verbose);
char *memstr(char *mem, int n, char *str);

/******************************************************************************/

int
main(int argc, char *argv[] )
{

mcpshmem_t *mcpshmem;
int unit;
int rc;
int verbose;
int bench;
char *mcp_name;
char *dma_test_type="usual";
int dma_test_repeat=1;
int my_pnid;
int escape_cnt;
char *route_fname;
FILE *route_fp;
char *route;
char *mcp_type_str;
char *mod_type_str;
int btype;

extern char *optarg;
extern int optind;
int error;
int index;
int ch;


    /* Set the defaults */
    verbose= 0;
    unit= 0;
    mcp_name= NULL;
    error= FALSE;
    bench= FALSE;
    my_pnid= -1;
    route_fname= NULL;

    while (TRUE)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'm':
		mcp_name= optarg;
		break;
	    case 'r':
		route_fname= optarg;
		break;
	    case 'u':
		unit= strtol(optarg, NULL, 10);
		break;
	    case 'b':
		bench= TRUE;
		break;
	    case 'd':
		dma_test_type = "integrity";
		dma_test_repeat= strtol(optarg, NULL, 10);
		break;
	    case 'v':
		verbose++;
		break;
	    case 'p':
		my_pnid= strtol(optarg, NULL, 10);
		break;
	    default:
		error= TRUE;
	}
    }

    if (route_fname == NULL)   {
	fprintf(stderr, "You must supply a route file name "
	    "(use - for stdin)\n");
	usage(argv[0]);
	exit(-1);
    }

    if ((route_fname[0] == '-') && (route_fname[1] == '\0'))   {
	route_fp= stdin;
    } else   {
	if (access(route_fname, R_OK) != 0)   {
	    fprintf(stderr, "Can't access route file \"%s\"\n", route_fname);
	    exit(-1);
	} else   {
	    route_fp= fopen(route_fname, "r");
	    if (route_fp == NULL)   {
		perror("Problem with route file");
		exit(-1);
	    }
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }

    if (my_pnid < 0)   {
	fprintf(stderr, "Need to know my node ID! (-pnid <num>)\n");
	usage(argv[0]);
	exit(-1);
    }

    /* Read in the route */
    if ((route= read_route(route_fp, verbose)) == NULL)   {
	fprintf(stderr, "Error reading route\n");
	exit(-1);
    }

    if (map_lanai(argv[0], (verbose > 1), unit) != OK)   {
	exit(-1);
    }

    if (verbose)   {
	printf("unit %d, mcp \"%s\"\n", unit, mcp_name);
    }

    printf("Found a LANai type ");
    btype= get_lanai_type(unit, TRUE);

    printf(" with %d bytes (%dkB) of memory unit %d\n",
	    lanai_memory_size(unit), lanai_memory_size(unit) / 1024, unit);

    if ((btype != L4) && (btype != L7) && (btype != L9))   {
	fprintf(stderr, "%s only supports LANai types 4.x, 7.x, and 9.x\n",
	    argv[0]);
	exit(-1);
    }

    lanai_clear_memory(unit);

#ifndef MCP_FROM_ARRAY
    check_mcp_access(&mcp_name, btype, argv[0], verbose);

    rc= lanai_load_and_reset(unit, mcp_name);
#else
    rc= local_lanai_load_reset(unit);
#endif
    if (rc != 1)   {
	fprintf(stderr, "load and reset of LANai board unit %d failed\n", unit);
	exit(-1);
    }

    if (verbose)   {
	printf("LANai reset and MCP loaded\n");
    }


    /*
    ** Now wait for the LANai to tell us where the shared structure resides
    */
    if ((verbose > 1))   {
	printf("Waiting for ISR host_sig_bit...");
    }
    escape_cnt= 0;
    while (!(getISR(unit, btype) & HOST_SIG_BIT))   {
	usleep(10000); /* I don't think these are usec on the Miata! */
	if (++escape_cnt > 50000)   {
	    fprintf(stderr,
		"ERROR MCP never came alive (did not get HOST_SIG_BIT)\n");
	    exit(-1);
	}
    }
    if ((verbose > 1))   {
	printf("got it, ISR is 0x%08x\n", getISR(unit, btype));
    }

    /* Clear the HOST_SIG_BIT bit and enable interrupts */
    lanai_interrupt_unit(unit, 1);
    setISR(unit, btype, HOST_SIG_BIT);
    setEIMR(unit, btype, HOST_SIG_BIT);


    /*
    ** The address of the shared data structure has been put into the SMP
    ** register by the MCP. This works, because the SMP register is not
    ** used during initialization.
    ** We then convert this address into a host address.
    */
    mcpshmem= (mcpshmem_t *)(&LANAI[unit][getSMP(unit, btype) >> 1]);
    if ((verbose > 1))   {
	printf("mcpshmem is at %p\n", (void *)mcpshmem);
    }

    if (mcpshmem != get_mcpshmem((verbose > 1), unit, argv[0], FALSE))   {
	fprintf(stderr, "Could not get access to shared data structure!\n");
	fprintf(stderr, "This could be a version mismatch between MCP and "
	    "mcpload,\n");
	exit(-1);
    }

    /* Fill in the user and the start time */
    mcpshmem->uid= getuid();
    mcpshmem->stime= time(NULL);

    test_rtc(verbose, unit, btype);

    if (bench)   {
	printf("WARNING!!! Running the benchmarks may require a reboot!\n");
	printf("Do you want to proceed? ");
	fflush(stdout);
	ch= getchar();
	if ((ch == 'y') || (ch == 'Y'))   {
	    do_bench(unit, mcpshmem, verbose, btype);
	} else   {
	    printf("Skipping benchmarks...\n");
	}
    }

    /* Download the route map */
    dnld_route(mcpshmem, route, verbose);

    if ( strstr(dma_test_type, "int") != NULL ) {
      mcpshmem->DMA_test_type = htonl(INTEGRITY);
      mcpshmem->DMA_test_repeat = htonl(dma_test_repeat);
    }

    /* Tell the LANai what node we are. This activates the MCP */
    mcpshmem->my_pnid= htonl(my_pnid);
    if (verbose)   {
	printf("We are phys nid %d\n", (int)ntohl(mcpshmem->my_pnid));
    }

    /* Now see if the MCP got the hstshmem */
    escape_cnt= 0;
    while (ntohl(mcpshmem->hstshmem) == 0)   {
	usleep(10000); /* I don't think these are usec on the Miata! */
	if (++escape_cnt > 50000)   {
	    fprintf(stderr, "ERROR MCP not properly initialized! "
		"(Did not get hstshmem)\n");
	    exit(-1);
	}
    }

    while (ntohl(mcpshmem->mod_type) == 0)   {
	usleep(10000); /* I don't think these are usec on the Miata! */
	if (++escape_cnt > 50000)   {
	    fprintf(stderr, "ERROR MCP not properly initialized! "
		"(MCP could not DMA to hstshmem)\n");
	    exit(-1);
	}
    }

    if (mcpshmem->mcp_type != mcpshmem->mod_type)   {
	switch ((int)ntohl(mcpshmem->mod_type))   {
	    case MCP_TYPE_PORTAL: mod_type_str= "portal module"; break;
	    case MCP_TYPE_PKT: mod_type_str= "packet module"; break;
	    case MCP_TYPE_TEST: mod_type_str= "test module"; break;
	    case MCP_TYPE_RTSCTS: mod_type_str= "rtscts module"; break;
	    default: mod_type_str= "unknown module"; break;
	}
	switch ((int)ntohl(mcpshmem->mcp_type))   {
	    case MCP_TYPE_PORTAL: mcp_type_str= "portal MCP"; break;
	    case MCP_TYPE_PKT: mcp_type_str= "packet MCP"; break;
	    case MCP_TYPE_TEST: mcp_type_str= "test MCP"; break;
	    case MCP_TYPE_RTSCTS: mcp_type_str= "rtscts MCP"; break;
	    default: mcp_type_str= "unknown MCP"; break;
	}

	fprintf(stderr, "\"%s\" type (%d) and \"%s\" type (%d) don't match\n",
	    mcp_type_str, (int)ntohl(mcpshmem->mcp_type),
	    mod_type_str, (int)ntohl(mcpshmem->mod_type));
	exit(-1);
    }

    if ( strstr(dma_test_type, "int") != NULL ) {
      escape_cnt= 0;
      while (ntohl(mcpshmem->DMA_test_failed) == DMA_TEST_NOTDONE)   {
	usleep(1000000); /* I don't think these are usec on the Miata! */
	fprintf(stderr, "MCPLOAD: waiting for integrity test to complete\n");
      }

      if (ntohl(mcpshmem->DMA_test_failed) != DMA_TEST_OK) {
	fprintf(stderr, "ERROR looks like the DMA test FAILED, "                                "(Check the system log)\n");
        exit(-1);
      }
    }

    printf("MCP loaded, initialized, and running\n");

    return 0;

}  /* end of main() */

/******************************************************************************/

void
usage(char *pname)
{

    fprintf(stderr,
	"Usage: %s -mcp mcp_name [-verbose] [-unit num]\n", pname);
    fprintf(stderr, "                  [-dma type] [-repeat rnum] [-bench] -pnid nid ");
    fprintf(stderr, "-route rf \n");
    fprintf(stderr, "  num    Myrinet interface unit number. Default is 0\n");
    fprintf(stderr, "  nid    Physcial nid of this node. Default is 0\n");
    fprintf(stderr, "  bench  Perform some shared memory benchmarks\n");
    fprintf(stderr, "  rf     Route file name. Use - for stdin\n");
    fprintf(stderr, "  type   Type of dma test: usual (default) | integrity\n");
    fprintf(stderr, "  rnum   For dma integrity test; times each pattern test is repeated\n");

}  /* end of usage() */

/******************************************************************************/

/*
** Do a little test on the real-time clock. We expect it to be at 500ns
** per tick.
*/
void
test_rtc(int verbose, int unit, int btype)
{

int rtc1, rtc2, rtc_diff;
double host_diff;
double tics, us;


    /* Read the clock, and sleep for about a second */
    rtc1= getRTC(unit, btype);
    start();
    usleep(50000000);

    /* Now read it again and figure out the tics per second */
    rtc2= getRTC(unit, btype);
    host_diff= stop();
    rtc_diff= rtc2 - rtc1;

    if ((rtc_diff == 0) || (host_diff == 0.0))   {
	printf("RTC real time clock on LANai or host timer is not running!\n");
	us= 0.0;
	tics= 0.0;
    } else   {
	tics= 1.0 / host_diff * rtc_diff;
	us= (host_diff / rtc_diff) * 1000000.0;
    }

    if ((us > 0.6) || (us < 0.4))   {
	printf("WARNING real time clock on LANai is not 500 ns / tic\n");
    }

    if (verbose || ((us > 0.6) || (us < 0.4)))   {
	printf("RTC:  %6.0f ticks per second: 1 tic = %7.3f us\n", tics, us);
    }

}  /* end of test_rtc() */
 
/******************************************************************************/
#define NUM_READS	(10000)

/*
** WARNING!!!
** Running these benchmarks may lock up your Alpha to the point where
** you'll have to reboot!
*/
void
do_bench(int unit, mcpshmem_t *mcpshmem, int verbose, int btype)
{

register int i;
int start, end;
int diff;
int rtc;
int *ptr;
int dummy;
long *lptr;
long ldummy;
int my_buf[DBL_BUF_SIZE];


    /* How long does it take to read the RTC 10000 times? */
    start= getRTC(unit, btype);
    for (i= 0; i < NUM_READS; i++)   {
	rtc= getRTC(unit, btype);
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read RTC is %d ns\n", diff * 500 / NUM_READS);
    }

    /* ---------------------------------------------------------------------- */
    /* How long does it take to write 8kB to shared memory? */
    ptr= mcpshmem->snd_buf_A;
    start= getRTC(unit, btype);
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(int)); i++)   {
	*ptr= i;
	ptr++;
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to write %d words to shared memory is %d ns "
	    "per word\n", (int)(DBL_BUF_SIZE / sizeof(int)),
	    (int)(diff * 500 / (DBL_BUF_SIZE / sizeof(int))));
    }

    /* ---------------------------------------------------------------------- */
    /* How long does it take to read 8kB from shared memory? */
    ptr= mcpshmem->snd_buf_A;
    start= getRTC(unit, btype);
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(int)); i++)   {
	dummy= *ptr;
	ptr++;
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read %d words from shared memory is %d ns "
	    "per word\n", (int)(DBL_BUF_SIZE / sizeof(int)),
	    (int)(diff * 500 / (DBL_BUF_SIZE / sizeof(int))));
    }

    /* ---------------------------------------------------------------------- */
    /* How long does it take to write 8kB to shared memory using longs? */
    lptr= (long *)mcpshmem->snd_buf_A;
    start= getRTC(unit, btype);
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(long)); i++)   {
	*lptr= i;
	lptr++;
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to write %d words to shared memory is %d ns per "
	    "long\n", (int)(DBL_BUF_SIZE / sizeof(long)),
	    (int)(diff * 500 / (DBL_BUF_SIZE / sizeof(long))));
    }

    /* ---------------------------------------------------------------------- */
    /* How long does it take to read 8kB from shared memory using longs? */
    lptr= (long *)mcpshmem->snd_buf_A;
    start= getRTC(unit, btype);
    for (i= 0; i < (DBL_BUF_SIZE / sizeof(long)); i++)   {
	ldummy= *lptr;
	lptr++;
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read %d words from shared memory is %d ns "
	    "per long\n", (int)(DBL_BUF_SIZE / sizeof(long)),
	    (int)(diff * 500 / (DBL_BUF_SIZE / sizeof(long))));
    }

    /* ---------------------------------------------------------------------- */
    /* How about using array indexing? (Use all four buffers.) */
    start= getRTC(unit, btype);
    for (i= 0; i < DBL_BUF_SIZE; i++)   {
	mcpshmem->snd_buf_A[i]= i;
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to write %d words to an array is %d ns per "
	    "word\n", (int)DBL_BUF_SIZE, (int)(diff * 500 / DBL_BUF_SIZE));
    }

    /* ---------------------------------------------------------------------- */
    /* How about using array indexing? (Use all four buffers.) */
    start= getRTC(unit, btype);
    for (i= 0; i < DBL_BUF_SIZE; i++)   {
	dummy= mcpshmem->snd_buf_A[i];
    }
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read %d words from an array is %d ns per "
	    "word\n", (int)DBL_BUF_SIZE, (int)(diff * 500 / DBL_BUF_SIZE));
    }

    /* ---------------------------------------------------------------------- */
    /* How about using memcpy? */
    start= getRTC(unit, btype);
    memcpy(mcpshmem->snd_buf_A, my_buf, DBL_BUF_SIZE * sizeof(int));
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to write %d words using memcpy is %d ns per "
	    "word\n", (int)DBL_BUF_SIZE, (int)(diff * 500 / DBL_BUF_SIZE));
    }

    /* ---------------------------------------------------------------------- */
    /* How about using memcpy? */
    start= getRTC(unit, btype);
    memcpy(my_buf, mcpshmem->snd_buf_A, DBL_BUF_SIZE * sizeof(int));
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read %d words using memcpy is %d ns per "
	    "word\n", (int)DBL_BUF_SIZE, (int)(diff * 500 / DBL_BUF_SIZE));
    }

    /* ---------------------------------------------------------------------- */
    /* Using memcpy to go to and from shared memory? */
    start= getRTC(unit, btype);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    memcpy(mcpshmem->snd_buf_B, mcpshmem->snd_buf_A, DBL_BUF_SIZE);
    end= getRTC(unit, btype);

    if (end > start)   {
	diff= end - start;
    } else   {
	diff= (INT_MAX - start) + (end - INT_MIN);
    }

    if (verbose)   {
	fprintf(stderr, "Time to read/write %d words using memcpy is %d ns per "
	    "word\n", 50 * (int)DBL_BUF_SIZE,
	    (int)(diff * 500 / (50 * DBL_BUF_SIZE)));
    }

}  /* end of do_bench() */

/******************************************************************************/

void
check_mcp_access(char **mcp_name, int btype, char *pname, int verbose)
{

static char real_mcp_name[1024];
struct stat stat_buf;
char *buf;
int fd;
long file_len;


    if (*mcp_name == NULL)   {
	fprintf(stderr, "You must supply a mcp file name\n");
	usage(pname);
	exit(-1);
    }

    memset(real_mcp_name, 0, 1024);
    if (btype == L7)   {
	sprintf(real_mcp_name, "%s.7", *mcp_name);
    } else if (btype == L4)   {
	sprintf(real_mcp_name, "%s.4", *mcp_name);
    } else if (btype == L9)   {
	sprintf(real_mcp_name, "%s.9", *mcp_name);
    } else   {
	sprintf(real_mcp_name, "%s", *mcp_name);
    }

    if (access(real_mcp_name, R_OK) != 0)   {
	/* Not there, try the original name and hope it is the right type MCP */
	if (verbose)   {
	    fprintf(stderr, "    Can't find MCP file %s, trying %s\n",
		real_mcp_name, *mcp_name);
	}
	sprintf(real_mcp_name, "%s", *mcp_name);
	if (access(real_mcp_name, R_OK) != 0)   {
	    fprintf(stderr, "%s: Can't access MCP file \"%s{.[479]}\"\n",
		pname, real_mcp_name);
	    exit(-1);
	}
    }

    if (verbose)   {
	fprintf(stderr, "    MCP file %s is accessible\n", real_mcp_name);
    }
    *mcp_name= real_mcp_name;

    /* Now check if it is the right type of MCP */
    fd= open(real_mcp_name, O_RDONLY);
    if (fd < 0)   {
	fprintf(stderr, "%s open() of %s failed\n", pname, real_mcp_name);
	perror("");
	exit(-1);
    }

    if (fstat(fd, &stat_buf) != 0)   {
	fprintf(stderr, "%s fstat() on %s failed\n", pname, real_mcp_name);
	perror("");
	exit(-1);
    }
    file_len= stat_buf.st_size;

    buf= (char *)malloc(file_len);
    if (buf == NULL)   {
	fprintf(stderr, "%s out of memory\n", pname);
	exit(-1);
    }

    if (read(fd, buf, file_len) != file_len)   {
	fprintf(stderr, "%s: Could not read %ld bytes from MCP file %s\n",
	    pname, file_len, real_mcp_name);
	exit(-1);
    }

    if (verbose)   {
	fprintf(stderr, "    Reading %ld bytes of MCP file %s\n", file_len,
	    real_mcp_name);
    }

    switch(btype) {
      case L7:
	if (memstr(buf, file_len, MCP_7_CHECK_STR) == NULL)   {
	    fprintf(stderr, "%s: The MCP file %s is not for a LANai 7.x\n",
		pname, real_mcp_name);
	    exit(-1);
	}
	if (verbose)   {
	    fprintf(stderr, "    MCP file %s is OK for LANai 7.x\n",
		real_mcp_name);
	}
        break;
      case L4:
	if (memstr(buf, file_len, MCP_4_CHECK_STR) == NULL)   {
	    fprintf(stderr, "%s: The MCP file %s is not for a LANai 4.x\n",
		pname, real_mcp_name);
	    exit(-1);
	}
	if (verbose)   {
	    fprintf(stderr, "    MCP file %s is OK for LANai 4.x\n",
		real_mcp_name);
	}
        break;
      case L9:
	if (memstr(buf, file_len, MCP_9_CHECK_STR) == NULL)   {
	    fprintf(stderr, "%s: The MCP file %s is not for a LANai 9.x\n",
		pname, real_mcp_name);
	    exit(-1);
	}
	if (verbose)   {
	    fprintf(stderr, "    MCP file %s is OK for LANai 9.x\n",
		real_mcp_name);
	}
        break;
    }

    close(fd);
    free(buf);
    return;

}  /* end of check_mcp_access() */

/******************************************************************************/

/*
** Look for the string 'str' in memory 'mem'. 'str' must be \0 terminated,
** but 'mem' can contain \0. If 'str' is found, a pointer to it is
** returned. If 'str' is not in 'mem', then NULL is returned.
*/
char *
memstr(char *mem, int n, char *str)
{

char *start;
char *end;
char *str_ptr;
char *search_start;
int search_len;


    end= mem + n;
    search_len= n;

    while (TRUE)   {
	str_ptr= str;
	start= memchr(mem, *str_ptr, search_len);
	if (start == NULL)   {
	    return NULL;
	}

	/* Now see if the rest of the string matches */
	search_start= start;
	while ((*str_ptr != '\0') && (*str_ptr == *start) && (start < end))   {
	    str_ptr++;
	    start++;
	}

	if (*str_ptr == '\0')   {
	    /* We found it! */
	    return search_start;
	}

	/* Try further down the line */
	mem= start + 1;
	search_len= end - mem;
	if (search_len <= 0)   {
	    break;
	}
    }

    return NULL;

}  /* end of memstr() */

/******************************************************************************/
#ifdef MCP_FROM_ARRAY
int
local_lanai_load_reset(const unsigned unit)
{
    int i;
    char *buffer = NULL;
    unsigned size;

    size = lanai_memory_size(unit);

    if (!size)   {
      fprintf(stderr, "local_lanai_load_reset: size is 0!\n");
      return 0;
    }

    /* get buffer */
    if (!(buffer = calloc(sizeof(char), size))) {
        fprintf(stderr, "local_lanai_load_reset: could not calloc() buffer");
        return 0;
    }
    /* copy mcp array to buffer */
    for (i=0; i<lanai_executable_length; i++) {
      buffer[i] = lanai_executable[i];
    }

    /* freeze the lanai */
    lanai_reset_unit(unit, 1);

    /* write the buffer into the lanai. */
    lanai_put(unit, buffer, 0, size);

    /* now un-reset the lanai */
    lanai_reset_unit(unit, 0);

    /* and set interrupts on or off */
    lanai_interrupt_unit(unit, 0);

    /* set the LANai version register (only needed on L4) */
    switch (lanai_board_type(unit)) {
        case lanai_4_0:
            setVERSION(unit, L4, 0);
            break;

        case lanai_4_1:
        case lanai_4_2:
        case lanai_4_3:
        case lanai_4_4:
        case lanai_4_5:
            setVERSION(unit, L4, 3);
            break;

        case lanai_7_0:
        case lanai_7_1:
        case lanai_7_2:
            break;

        case lanai_9_0:
        case lanai_9_1:
        case lanai_9_2:
        case lanai_9_3:
            break;

        default:
            /* We don't support any other LANai types */
	    return 0;
    }

    /* free the buffer. */
    free(buffer);
    *(unsigned int *) &LANAI_CONTROL[unit][0] = (
	LANAI5_RESET_OFF | LANAI5_ERESET_OFF | LANAI5_BRESET_OFF |
	LANAI5_INT_ENABLE | LANAI5_FORCE_64BIT_MODE );

    #ifdef VERBOSE
    printf("Set control register to 0x%08x, now 0x%08x\n" ,
	LANAI5_RESET_OFF | LANAI5_ERESET_OFF | LANAI5_BRESET_OFF |
	LANAI5_INT_ENABLE | LANAI5_FORCE_64BIT_MODE,
	(*(unsigned int *) &LANAI_CONTROL[unit][0]));
    #endif /* VERBOSE */

    return 1;
}
#endif /* MCP_FROM_ARRAY */
