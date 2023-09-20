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
/* $Id: fyod_fyod_comm.c,v 1.17 2001/04/01 02:35:43 pumatst Exp $ */

#include <stdio.h>
#include <unistd.h>
#include "srvr_comm.h"
#include "rpc_msgs.h"
#include "string.h"
#include "host_msg.h"
#include "ppid.h"
#include "fyod.h"
#include "cplant.h"
#include "cplant_host.h"

/* linux EREMOTEIO */
#ifdef __osf__
#include <asm/errno.h>
#endif

static int Dbglevel;

/*  I_have(unit)
    see if the requested unit is among those attached
    to this node.   Return  TRUE or FALSE
*/
int I_have(int unit)
{
   int i;

   for (i=0; i<MAX_DISKS_PER_FYOD_NODE; i++) {
      if ( unit == fyod_units[i] ) {
          return TRUE;
      }
   }

   return FALSE;
}
