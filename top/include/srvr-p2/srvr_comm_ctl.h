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
** $Id: srvr_comm_ctl.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COMM_CTL_H
#define SRVR_COMM_CTL_H

#include "puma.h"
#include "portal.h"
#include "srvr_comm.h"

typedef struct{
    PTL_MSG_HDR *msg_hdr;
    data_xfer_msg *msg;
}control_msg_handle;

/*
** Macros to access the handle are defined for both P3 and P2 since
** the control_msg_handles differ.  And caller shouldn't have to
** know the fields anyway.
*/

#define SRVR_CLEAR_HANDLE(h) { \
    (h).msg_hdr = (PTL_MSG_HDR *)NULL; \
    (h).msg = (data_xfer_msg *)NULL;    \
  }

#define SRVR_IS_VALID_HANDLE(h)   ((h).msg_hdr != NULL)

#define SRVR_HANDLE_NID(h)           ((h).msg_hdr->src_nid)
#define SRVR_HANDLE_PID(h)           ((h).msg_hdr->src_pid)
#define SRVR_HANDLE_TYPE(h)          ((h).msg->msg_type)
#define SRVR_HANDLE_RET_PTL(h)       ((h).msg->ret_ptl)
#define SRVR_HANDLE_TRANSFER_LEN(h)  ((h).msg->req_len)
#define SRVR_HANDLE_USERDEF(h)       ((h).msg->user_data)
#define SRVR_HANDLE_MATCHBITS(h)     ((h).msg->mbits)
#define SRVR_HANDLE_MSG(h)           ((h).msg)

#define SEND_TIMEOUT   60.0

int srvr_await_send_completion(volatile unsigned int *flag);
int srvr_init_control_ptl(INT32 max_num_msgs);
int srvr_init_control_ptl_at(INT32 max_num_msgs, int ptl);

INT32 srvr_send_to_control_ptl(INT32 nid, INT32 pid, int ptl, 
               INT32 msg_type, CHAR *user_data, INT32 len);
INT32 srvr_get_next_control_msg(int ptl, control_msg_handle *handle,
              INT32 *msg_type, INT32 *xfer_len, CHAR **user_data);
INT32 srvr_free_control_msg(int ptl, control_msg_handle *handle);
INT32 srvr_release_control_ptl(int ptl);
VOID srvr_display_control_msg(control_msg_handle *h);
int srvr_reset_control_ptl(int ptl);



#endif
