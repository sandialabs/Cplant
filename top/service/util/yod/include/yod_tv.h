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
** $Id: yod_tv.h,v 1.2 2001/11/02 17:25:30 lafisk Exp $
**
*/

#ifndef YOD_TV_H
#define YOD_TV_H

#include<string.h>
#include"yod_data.h"

#define SLASH               47

#define MPIR_DEBUG_SPAWNED   1
#define MPIR_DEBUG_ABORTING  2

typedef struct {
  char *host_name;
  char *executable_name;
  int   pid;
} MPIR_PROCDESC;

extern MPIR_PROCDESC *MPIR_proctable;
extern int            MPIR_proctable_size;
extern volatile int   MPIR_debug_state;
extern volatile int   MPIR_debug_gate;
extern char          *MPIR_debug_abort_string;
extern int            MPIR_being_debugged;
extern int            MPIR_I_am_starter;
extern char          *MPIR_dll_name;

extern void MPIR_Breakpoint( void );
extern void dump_mpir_proctable( int );

#ifdef _FOR_LIBJOB
#include "job_private.h"
void init_TotalView_symbols( int nprocs, int *nmap, spid_type *pidmap, 
                             loadMbrs *mbrs, int nmembers);
#else
extern void init_TotalView_symbols( int, int *, spid_type * );
#endif


#endif

