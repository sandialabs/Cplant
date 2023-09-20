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
** $Id: srvr_comm.h,v 1.13 2002/03/06 21:35:11 ktpedre Exp $
**
** Objects that may be needed by users of server library.
*/

#include <stdio.h>
#include <time.h>
#include <puma.h>
#include <lib-p30.h>
#include <p30/types.h>

#ifndef SRVR_COMM_H
#define SRVR_COMM_H

#define SRVR_USR_DATA_LEN   128 



typedef struct {

    int flag;

    int src_nid;
    int src_pid;

    int msg_type;
    int ret_ptl;   /* actually mbits for entry in master PUT or GET ptl */
    int req_len;
    ptl_match_bits_t mbits;

    char user_data[SRVR_USR_DATA_LEN];

}data_xfer_msg;

struct _recvList{

    data_xfer_msg msg;

    struct _recvList *next;
    struct _recvList *prev;

};

extern int __SrvrDebug;

/*
** Change in server library interface from Portals 2 to Portals 3.
** control_msg_handle has src_nid and src_pid fields.  Caller
** can set these to request a message from a particular source.
** The src_nid and src_pid used to be in the msg_hdr field of
** the control_msg_handle.
**
** The control message contents is all contained in the handle, so
** control message can be freed, and put or get request satisfied
** with this handle later on.  Before, handle only had a pointer into
** the contents, so if message was freed, handle was useless.
*/
 
typedef struct{
    int match_nid;
    int match_pid;
    int match_type;
    struct _recvList *msg;
}control_msg_handle;

/*
** Macros to access the handle are defined for both P3 and P2 since
** the control_msg_handles differ
*/

#define SRVR_CLEAR_HANDLE(h) { \
    (h).msg = NULL;            \
    (h).match_nid = SRVR_INVAL_NID; \
    (h).match_pid = SRVR_INVAL_PID; \
    (h).match_type = SRVR_INVAL_TYPE; \
    }

#define SRVR_IS_VALID_HANDLE(h)    ((h).msg != NULL)

#define SRVR_HANDLE_NID(h)      ((h).msg ? (h).msg->msg.src_nid : (h).match_nid)
#define SRVR_HANDLE_PID(h)      ((h).msg ? (h).msg->msg.src_pid : (h).match_pid)
#define SRVR_HANDLE_TYPE(h)     ((h).msg ? (h).msg->msg.msg_type : (h).match_type)
#define SRVR_HANDLE_RET_PTL(h)       ((h).msg->msg.ret_ptl)
#define SRVR_HANDLE_TRANSFER_LEN(h)  ((h).msg->msg.req_len)
#define SRVR_HANDLE_USERDEF(h)       ((h).msg->msg.user_data)
#define SRVR_HANDLE_MATCHBITS(h)     ((h).msg->msg.mbits)

#define BLOCKING     (1)
#define NONBLOCKING  (0)

#define SRVR_INVAL_NID (-1)
#define SRVR_INVAL_PID (0)
#define SRVR_INVAL_PTL (-1)
#define SRVR_INVAL_TYPE (-1)

#define SRVR_ANY_NID  SRVR_INVAL_NID
#define SRVR_ANY_PID  SRVR_INVAL_PID
#define SRVR_ANY_TYPE SRVR_INVAL_TYPE

#define SRVR_MAX_PORTAL  (MAX_MES-1)

#define SRVR_MINUSERPTL  0
#define SRVR_MAXUSERPTL  63
#define SRVR_MINSYSPTL   (SRVR_MAXUSERPTL+1)
#define SRVR_MAXSYSPTL   (MAX_MES-1)
 
#define SRVR_MINCTLPTL   SRVR_MINUSERPTL
#define SRVR_MAXCTLPTL   SRVR_MAXSYSPTL

#define SRVR_MINDATAPTL   SRVR_MINUSERPTL
#define SRVR_MAXDATAPTL   SRVR_MAXSYSPTL

/*
** from srvr_comm.c
*/
int server_library_init(void);
void server_library_done(void);
int srvr_p30_barrier(time_t tmout);
int srvr_p30_bcast(int *val, time_t tmout);


/*
** from srvr_comm_ctl.c
*/
int srvr_init_control_ptl_at(int max_num_msgs, ptl_pt_index_t ptl);
int srvr_init_control_ptl(int max_num_msgs);

int srvr_get_next_control_msg(ptl_pt_index_t ptl, control_msg_handle *handle,
	      int *msg_type, int *xfer_len, char **user_data);
int srvr_free_control_msg(ptl_pt_index_t ptl, control_msg_handle *handle);
int srvr_free_all_control_msgs(ptl_pt_index_t ptl);

int srvr_send_to_control_ptl(int nid, int pid, ptl_pt_index_t ptl,
	       int msg_type, char *user_data, int len);

int srvr_send_to_control_ptls(int num_targets, int *nid, int *pid, ptl_pt_index_t *ptl,
		              int msg_type, char *user_data, int len, int sflag, int *wait_for);

int srvr_release_control_ptl(int ptl);
void srvr_display_data_xfer_msg(data_xfer_msg *msg);


/*
** from srvr_comm_data.c
*/
int srvr_test_write_buf(int slot);
int srvr_test_read_buf(int slot, int count);
int srvr_delete_buf(int slot);

/*
** from srvr_comm_get.c
*/
int srvr_comm_get_req(char *buf, int len, int type, char *user_data,
      int user_data_len, int nid, int pid, int ptl,
      int blocking, int tmout);

int srvr_comm_get_reply(control_msg_handle *mh, void *rbuf, int len);
int srvr_comm_get_reply_partial(control_msg_handle *mh, void *rbuf, int len,
                            int offset);

/*
** from srvr_comm_put.c
*/
int srvr_comm_put_req(char *buf, int len, int type, 
	   char *user_data, int user_data_len,
           int ntargets, int *nidlist, int *pidlist, int *ptllist) ;

int srvr_comm_put_reply(control_msg_handle *mh, void *rbuf, int len);
int srvr_comm_put_reply_partial(control_msg_handle *mh, void *rbuf, 
                                int len, int offset);


#endif
