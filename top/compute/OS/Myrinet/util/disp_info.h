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
** $Id: disp_info.h,v 1.5 2001/08/22 16:45:14 pumatst Exp $
** A set of utilities to display registers and Puma/MCP/myrptl data
** structures.
*/

#ifndef DISP_INFO_H
#define DISP_INFO_H

#include "../MCP/hstshmem.h"


void printISR(int isr, int unit, int type);
void printCounters(mcpshmem_t *mcpshmem, int verbose, int type);
void printBench(mcpshmem_t *mcpshmem);
void printFault(int fault, mcpshmem_t *mcpshmem, int verbose, int unit,
	int type);
void printOwner(mcpshmem_t *mcpshmem);
void printMisc(mcpshmem_t *mcpshmem, int verbose);
void printSndXfer(mcpshmem_t *mcpshmem, int full_list);
void printRcvXfer(mcpshmem_t *mcpshmem, int full_list);
void printRegs(mcpshmem_t *mcpshmem, int unit);
void printStateMstat(mcpshmem_t *mcpshmem);
void printHstMisc(hstshmem_t *hstshmem);
void printHstCounters(hstshmem_t *hstshmem);
void printEvents(mcpshmem_t *mcpshmem, int unit, int type);

#endif /* DISP_INFO_H */
