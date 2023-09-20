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
** $Id: fyod_comm.c,v 1.14 2001/02/16 05:21:48 lafisk Exp $
**
**   This is FYOD_COMM
**	Modified February 4th, 1998 to use Lee Ann's new srvr_comm Stuff
**	1.  First set of mods will be to simple use 
**		srvr_init_control_ptl   and
**		srvr_get_next_control_msg
**
*/
#include "srvr_comm.h"
#include "sys_limits.h"

extern int Dbglevel;

static int ptl_app_procs = SRVR_INVAL_PTL; 

/***********************************************************************
** Work buffer operations: data is pulled from node into work buffer,
** or pushed out to node from work buffer.
**
** These functions originally defined in Puma in host_msg.c, using Puma 
** message passing module features.  Our work buffer is a single block 
** portal, and we use portal message passing features.
**
** NOTE: this may not be the most efficient way to handle compute node
** service requests, but these are the calls defined for the Paragon
** Puma message passing module interface, and we maintain the api because
** the CMDhandler* files all use them.  We are in a hurry here. 
***********************************************************************/

int
get_app_srvr_portal()
{
    return ptl_app_procs;
}

INT32
get_next_cmd( control_msg_handle *mh)
{
int rc;

  SRVR_CLEAR_HANDLE(*mh);

  rc = srvr_get_next_control_msg(ptl_app_procs , mh, NULL, NULL, NULL) ;

  return rc;

}

/*	JOHN'S  Addition    */
void
free_cmd(control_msg_handle *mh)
{
   srvr_free_control_msg(ptl_app_procs, mh);
}
 
#define N_MSG_BUF MAX_NODES
int
host_cmd_init(void)
{
int rc;

  if (Dbglevel > 1 )
     printf(" JOHNs host_cmd_init entered (Version Feb 4 98 )\n");

  ptl_app_procs = 0;   /* this is hard coded in libgrp_io.a */

  rc = srvr_init_control_ptl_at ( (INT32) N_MSG_BUF, ptl_app_procs );

  if (rc){
     printf(" Can't create a portal\n");
     return -1;
     }

  if (Dbglevel > 1 )
     printf(" Portal allocated is %d\n", ptl_app_procs );

  if (initialize_work_bufs() != 0 ) {
     printf(" Failed to Initialize Work Buffers\n");
     return -1;
     }

  if (Dbglevel > 1 )
     printf("\n\t\tCompleted initialization\n\n");
  return 0;
}
 
