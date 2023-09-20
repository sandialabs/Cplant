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
** $Id: common.c,v 1.19 2001/06/12 16:59:27 rolf Exp $
** This file contains functions that are shared by the programs in this
** directory.
*/

#include <stdio.h>
#include <stdlib.h>
#include "lanai_device.h"
#include "defines.h"
#include "../MCP/MCPshmem.h"
#include "../MCP/MCPSHMEM_ADDR.h"	/* For MCPSHMEM_ADDR */
#include "common.h"
#include "arch_asm.h"			/* Import mb() and wmb() */


/******************************************************************************/
/*
** map_lanai()
** open the copy block and make sure the unit we want exists.
** Return OK is successful, ERROR otherwise.
*/
int
map_lanai(char *pname, int verbose, int unit)
{

int units;


    /* Mmap LANai board memory into our address space */
    mb();
    units = open_lanai_device();

    if (units > 0)   {
	if (verbose)   {
	    printf("Successfully opened LANai device. Found %d interface(s).\n",
		units);
	}
    } else   {
	fprintf(stderr, "No LANai devices available\n");
	return ERROR;
    }

    if ((unit < 0) || (unit >= units))   {
	if (units > 1)   {
	    fprintf(stderr, "Invalid unit number. Must be between 0 and %d\n",
		units);
	} else   {
	    fprintf(stderr,
		"Invalid unit number. Must be 0 (Only 1 LANai in system)\n");
	}

	return ERROR;
    }

    return OK;

}  /* end of map_lanai() */

/******************************************************************************/

/*
** get_mcpshmem()
** The MCP puts the address of the shared data structure at address
** MCPSHMEM_ADDR. Go get it.
*/
mcpshmem_t *
get_mcpshmem(int verbose, int unit, char *pname, int force)
{

int *mcpshmem_addr;
mcpshmem_t *mcpshmem;


    mb();
    mcpshmem_addr= (int *)(&LANAI[unit][MCPSHMEM_ADDR >> 1]);
    mcpshmem= (mcpshmem_t *)(&LANAI[unit][ntohl(*mcpshmem_addr) >> 1]);

    if (verbose)   {
	printf("mcpshmem (from mcpshmem_addr %p) is at %p\n",
	    (void *)(mcpshmem_addr), (void *)(mcpshmem));
    }

    if (mcpshmem == NULL)   {
	fprintf(stderr, "Can't get address of shared data structure\n");
	return NULL;
    }

    if (mcpshmem == (mcpshmem_t *)0xaaaaaaaa)   {
	fprintf(stderr, "MCP has not initialized mcpshmem\n");
	return NULL;
    }

    if ((unsigned long)mcpshmem & ALIGN_64b)   {
	/* mcpshmem is unaligned, don't use it! */
	fprintf(stderr, "mcpshmem is unaligned (%p)! MCP probably died.\n",
	    (void *)mcpshmem);
	if (!force)   {
	    return NULL;
	}
    }

    if ((int)ntohl(mcpshmem->ID) != MCP_ID)   {
	fprintf(stderr, "MCP ID is wrong (0x%08x != 0x%08x). MCP not "
	    "initialized, other type, or mcpshmem alignment problem\n",
	    (unsigned int)ntohl(mcpshmem->ID), MCP_ID);
	if (!force)   {
	    return NULL;
	}
    }

    if (((unsigned long)&mcpshmem->stime & ALIGN_64b) != 0) {
	fprintf(stderr, "mcpshmem is unaligned!\n");
	if (!force)   {
	    return NULL;
	}
    }

    if (ntohl(mcpshmem->version) != MCP_version)   {
	fprintf(stderr, "MCP version does not match version of %s\n", pname);
	if (!force)   {
	    return NULL;
	}
    }

    return mcpshmem;

}  /* end of get_mcpshmem() */

/******************************************************************************/
/*
** get_mcp_mcpshmem()
** Get the address the MCP uses for mcpshmem.
*/
int
get_mcp_mcpshmem(int unit)
{

int *mcpshmem_addr;


    mb();
    mcpshmem_addr= (int *)(&LANAI[unit][MCPSHMEM_ADDR >> 1]);
    return ntohl(*mcpshmem_addr);

}  /* end of get_mcp_mcpshmem() */

/******************************************************************************/

int
get_lanai_type(int unit, int print) 
{

int btype;


  //printf("lanai board type: ");

  btype= L0;	/* default, unsupported type */
  switch (lanai_board_type(unit))   {
	case lanai_2_3: if (print) printf("2.3"); break;
	case lanai_3_0: if (print) printf("3.0"); break;
	case lanai_3_1: if (print) printf("3.1"); break;
	case lanai_3_2: if (print) printf("3.2"); break;
	case lanai_4_0: if (print) printf("4.0"); break;
	case lanai_4_1: if (print) printf("4.1"); btype= L4; break;
	case lanai_4_2: if (print) printf("4.2"); btype= L4; break;
	case lanai_4_3: if (print) printf("4.3"); btype= L4; break;
	case lanai_4_4: if (print) printf("4.4"); btype= L4; break;
	case lanai_4_5: if (print) printf("4.5"); btype= L4; break;
	case lanai_5_0: if (print) printf("5.0"); break;
	case lanai_5_1: if (print) printf("5.1"); break;
	case lanai_5_2: if (print) printf("5.2"); break;
	case lanai_5_3: if (print) printf("5.3"); break;
	case lanai_6_0: if (print) printf("6.0"); break;
	case lanai_6_1: if (print) printf("6.1"); break;
	case lanai_6_2: if (print) printf("6.2"); break;
	case lanai_6_3: if (print) printf("6.3"); break;
	case lanai_7_0: if (print) printf("7.0"); btype= L7; break;
	case lanai_7_1: if (print) printf("7.1"); btype= L7; break;
	case lanai_7_2: if (print) printf("7.2"); btype= L7; break;
	case lanai_7_3: if (print) printf("7.3"); btype= L7; break;
	case lanai_9_0: if (print) printf("9.0"); btype= L9; break;
	case lanai_9_1: if (print) printf("9.1"); btype= L9; break;
	case lanai_9_2: if (print) printf("9.2"); btype= L9; break;
	case lanai_9_3: if (print) printf("9.3"); btype= L9; break;
	case lanai_9_4: if (print) printf("9.4"); btype= L9; break;
	case unknown:   if (print) printf("unknown"); break;
  }

  return btype;

}  /* end of get_lanai_type() */
