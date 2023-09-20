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
** $Id: pct_health.h,v 1.5 2001/06/01 17:51:08 rklundt Exp $
*/

#ifndef PCT_HEALTH_H
#define PCT_HEALTH_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pct.h"
#include "srvr_err.h"
#include "sys/defines.h"

/*
** for calls to check_host_health
*/

#define CHECK_ZOMBIES
#undef CHECK_MEMORY
/*
#define CHECK_SCRATCH
*/
#define MAX_SCRATCH_USED 50	  /* scratch used in M */

#ifdef __i386__
#define MEMORY_THRESHOLD 2	  /* percent free memory */
#else
#define MEMORY_THRESHOLD 50	
#endif

extern char *scratch_loc;

typedef struct {
  long total;
  long free;
  long shared;
  long buffers;
  long cached;
  long swap_total;
  long swap_free;
} MEMINFO;

INT32 check_host_health ();
INT32 check_host_memory ();
INT32 check_host_zombies ();
INT32 check_host_scratch();

#endif /* PCT_HEALTH_H */
