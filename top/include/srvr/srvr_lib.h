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
** $Id: srvr_lib.h,v 1.5 2001/09/26 00:01:54 rbbrigh Exp $
**
** Objects private to server library.
*/

#ifndef SRVR_LIB_H
#define SRVR_LIB_H

#include <lib-p30.h>
#include <myrnal.h>
#include <srvr_comm.h>
#include <srvr_err.h>

extern char             __SrvrLibInit;
extern ptl_handle_ni_t  __SrvrLibNI;

/*
** Portals: p30 library uses 0 for PtlNIBarrier
**   MPI uses 5, 6, and 7.
*/
#define CONTROLPORTALS    10
#define DATAPORTALS       11 
#define COLLPORTALS       12

#define SRVR_ACL_ANY          1

#define SEND_TIMEOUT    10
#define PUT_REPLY_TIMEOUT   SEND_TIMEOUT
#define GET_REPLY_TIMEOUT   SEND_TIMEOUT

#define EVENTQUEUE_LENGTH   4096

#define SRVR_INVAL_ME_HANDLE    (-1)
#define SRVR_INVAL_MD_HANDLE    (-1)
#define SRVR_INVAL_EQ_HANDLE    (-1)

#define READ_DATA_BUFFER    1
#define WRITE_DATA_BUFFER   2

/*
** Flags for control portal structures
*/
#define CTLMSG_NOMSG       0
#define CTLMSG_UNPROCESSED 1
#define CTLMSG_PROCESSED   2    /* copied from portal to linked list */
#define CTLMSG_UNREAD      3
#define CTLMSG_READ        4    /* returned to caller of get_next_ctrl_msg */

extern double dclock(void);

#define P3ERROR {srvr_p30errno = rc; CPerrno = EPORTAL; return -1;}
#define COLLP3ERROR {srvr_p30errno = rc; CPerrno = EPORTAL; return DSRVR_RESOURCE_ERROR;}

/*
** from srvr_comm_data.c - used by server library
*/
void srvr_null_dp(); 
int srvr_add_data_buf(void *buf, int len, int type);

/*
** from srvr_comm_ctl.c
*/
void srvr_lib_list_ctlPtl(FILE *fp);
void release_all_control_portals();
int srvr_send_it(int nid, int pid, ptl_pt_index_t ptl,
             int msg_type, char *user_data, int len,
             int retptl, int reqlen, int retbits);

int srvr_await_send_completion(ptl_md_t *mddef);

/*
** in srvr_comm.c
*/
int srvrHandleSendTimeout(ptl_md_t mddef, ptl_handle_md_t md);


#endif
