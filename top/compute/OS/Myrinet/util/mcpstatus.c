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
** $Id: mcpstatus.c,v 1.43 2001/06/12 16:59:27 rolf Exp $
** This program can be used to interrogate the status of the MCP. It displays
** the ISR register of the MCP and some key fields of the shred data
** structure.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "lanai_device.h"
#include "defines.h"
#include "../MCP/MCPshmem.h"
#include "common.h"
#include "disp_info.h"


/*
** Command line options
*/
static struct option opts[]=
{
    {"unit",		required_argument,	0, 'u'},
    {"isr",		required_argument,	0, 'i'},
    {"LANai",		required_argument,	0, 't'},
    {"type",		required_argument,	0, 't'},
    {"verbose",		no_argument,		0, 'v'},
    {"force",		no_argument,		0, 'f'},
    {"bench",		no_argument,		0, 'b'},
    {0, 0, 0, 0}
};


/*
** Local functions
*/
void usage(char *pname);


/******************************************************************************/

int
main(int argc, char *argv[] )
{

mcpshmem_t *mcpshmem;
int unit;
int verbose;
int force;
int bench;
int type;
int mapped;

extern char *optarg;
extern int optind;
int error;
int index;
int ch;
int isr;


    /* Set the defaults */
    isr= 0;
    verbose= 0;
    unit= 0;
    error= FALSE;
    force= FALSE;
    bench= FALSE;
    type= L0;
    mapped= FALSE;

    while (TRUE)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 't':
		if (strtol(optarg, NULL, 0) == 7)   {
		    type= L7;
		} else if (strtol(optarg, NULL, 0) == 4)   {
		    type= L4;
		} else   {
		    fprintf(stderr, "Unknown LANai type. Use 4 or 7\n");
		    error= TRUE;
		}
		break;
	    case 'i':
		isr= strtol(optarg, NULL, 0);
		break;
	    case 'u':
		unit= strtol(optarg, NULL, 10);
		break;
	    case 'v':
		verbose++;
		break;
	    case 'b':
		bench= TRUE;
		break;
	    case 'f':
		force= TRUE;
		break;
	    default:
		error= TRUE;
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }

    if (type == L0)   {
	if (map_lanai(argv[0], (verbose > 1), unit) != OK)   {
	    exit(-1);
	}
	mapped= TRUE;
    
	type= get_lanai_type(unit, FALSE);
    }

    if (type == L0)   {
        printf("Unsupported LANai type. Decoding not possible\n");
        exit(-1);
    }

    if (isr)   {
	printISR(isr, unit, type);
	return 0;
    }

    if (!mapped)   {
	fprintf(stderr, "No LANai type specified\n");
	exit(-1);
    }

    mcpshmem= get_mcpshmem((verbose > 1), unit, argv[0], force);
    if (mcpshmem == NULL)   {
	exit(-1);
    }

    if ((int)ntohl(mcpshmem->mcp_type) == MCP_TYPE_PORTAL)   {
	printf("Status of Portal MCP (ver %d, LANai %d.x) running on LANai "
	    "unit %d\n", (int)ntohl(mcpshmem->version),
	    (int)ntohl(mcpshmem->LANai_vers), unit);
    } else if ((int)ntohl(mcpshmem->mcp_type) == MCP_TYPE_PKT)   {
	printf("Status of Packet MCP (ver %d, LANai %d.x) running on LANai "
	    "unit %d\n", (int)ntohl(mcpshmem->version),
	    (int)ntohl(mcpshmem->LANai_vers), unit);
    } else if ((int)ntohl(mcpshmem->mcp_type) == MCP_TYPE_TEST)   {
	printf("Status of Test MCP (ver %d, LANai %d.x) running on LANai "
	    "unit %d\n", (int)ntohl(mcpshmem->version),
	    (int)ntohl(mcpshmem->LANai_vers), unit);
    } else if ((int)ntohl(mcpshmem->mcp_type) == MCP_TYPE_RTSCTS)   {
	printf("Status of rts/cts pkt MCP (ver %d, LANai %d.x) running on "
	    "LANai unit %d\n", (int)ntohl(mcpshmem->version),
	    (int)ntohl(mcpshmem->LANai_vers), unit);
    } else   {
	printf("Status of UNKNOWN MCP (ver %d, LANai %d.x) running on LANai "
	    "unit %d\n", (int)ntohl(mcpshmem->version),
	    (int)ntohl(mcpshmem->LANai_vers), unit);
    }
    printf("-----------------------------------------------------------------"
	"---------------\n");
    printOwner(mcpshmem);
    if (!bench)   {
	printISR(getISR(unit, type), unit, type);
	printCounters(mcpshmem, verbose, type);
	printFault(ntohl(mcpshmem->fault), mcpshmem, verbose, unit, type);
	printMisc(mcpshmem, verbose);

	if (verbose)   {
	    printRegs(mcpshmem, unit);
	    printf("    snd buf A 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		mcpshmem->snd_buf_A[0], mcpshmem->snd_buf_A[1],
		mcpshmem->snd_buf_A[2], mcpshmem->snd_buf_A[3],
		mcpshmem->snd_buf_A[4], mcpshmem->snd_buf_A[5]);
	    printf("              0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ...\n",
		mcpshmem->snd_buf_A[6], mcpshmem->snd_buf_A[7],
		mcpshmem->snd_buf_A[8], mcpshmem->snd_buf_A[9],
		mcpshmem->snd_buf_A[10]);
	    printf("    snd buf B 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		mcpshmem->snd_buf_B[0], mcpshmem->snd_buf_B[1],
		mcpshmem->snd_buf_B[2], mcpshmem->snd_buf_B[3],
		mcpshmem->snd_buf_B[4], mcpshmem->snd_buf_B[5]);
	    printf("              0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ...\n",
		mcpshmem->snd_buf_B[6], mcpshmem->snd_buf_B[7],
		mcpshmem->snd_buf_B[8], mcpshmem->snd_buf_B[9],
		mcpshmem->snd_buf_B[10]);
	    printf("    rcv buf A 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		mcpshmem->rcv_buf_A[0], mcpshmem->rcv_buf_A[1],
		mcpshmem->rcv_buf_A[2], mcpshmem->rcv_buf_A[3],
		mcpshmem->rcv_buf_A[4], mcpshmem->rcv_buf_A[5]);
	    printf("              0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ...\n",
		mcpshmem->rcv_buf_A[6], mcpshmem->rcv_buf_A[7],
		mcpshmem->rcv_buf_A[8], mcpshmem->rcv_buf_A[9],
		mcpshmem->rcv_buf_A[10]);
	    printf("    rcv buf B 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		mcpshmem->rcv_buf_B[0], mcpshmem->rcv_buf_B[1],
		mcpshmem->rcv_buf_B[2], mcpshmem->rcv_buf_B[3],
		mcpshmem->rcv_buf_B[4], mcpshmem->rcv_buf_B[5]);
	    printf("              0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ...\n",
		mcpshmem->rcv_buf_B[6], mcpshmem->rcv_buf_B[7],
		mcpshmem->rcv_buf_B[8], mcpshmem->rcv_buf_B[9],
		mcpshmem->rcv_buf_B[10]);
	}
	printEvents(mcpshmem, unit, type);
    } else   {
	printBench(mcpshmem);
    }

    return 0;

}  /* end of main() */

/******************************************************************************/
void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-verbose] [-unit num] [-force] [-isr value] "
	"[-bench] [-LANai type] [-type type]\n", pname);
    fprintf(stderr, "   num       Myrinet interface unit number. Default "
	"is 0\n");
    fprintf(stderr, "   force     Continue, even if MCP seems to be dead or "
	"corrupted\n");
    fprintf(stderr, "   value     Print ISR bits for a given value (and "
	"exit)\n");
    fprintf(stderr, "   -bench    Print benchmark results from MCP boot-up\n");
    fprintf(stderr, "   -type     Manual override of LANai type. Currently 4 "
	"or 7\n");
    fprintf(stderr, "   -LANai    Same as -type\n");
    fprintf(stderr, "   -verbose  can be repeated for more info\n");

}  /* end of usage() */

/******************************************************************************/
