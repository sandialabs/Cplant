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
** $Id: srvr_comm_get.c,v 1.1 2000/11/04 03:56:31 lafisk Exp $
*/
#include "string.h"
#include "puma.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "srvr_comm_get.h"
#include "srvr_comm_data.h"
 
/*
** To pull data from remote processes:
**
**    Initially, before any transfers -
**
**    need to have nid/pid of remote process
**    need to have portal on which remote process listens for requests
**      of type data_xfer_msg
**
**    open a data portal to hold match list entries pointing to
**      local data buffers
**
**    For each xfer -
**
**    open a write memory descriptor for this operation
**
**    send data_xfer_msg requesting data from remote process
**
**    test write memory descriptor for receipt of data
**
**    free the write memory descriptor
*/

/*
** Send a request to get data from a remote process.
**
** buf & len: where data is to be deposited
** msg_type : sent along to remote process
** myptl :  the data portal opened on my side for transfers
** nid,pid,ptl : of remote process
**
** If non-blocking:
** Return value is a handle.  Use it to check for arrival with 
** srvr_test_write_buf().  Error return is -1.  Remember to call
** srvr_comm_free_handle() after data comes in.
**
** If blocking:
** Returns 0 if data has arrived, -1 on error.  If waiting time
** exceeds timeout, return is -1.  Timeout value 0.0 means wait
** indefinitely.
*/
 
static UINT32 start_key  = 0x00000001;
static UINT32 unique_key = 0x00000001;
static UINT32 max_key    = 0x80000000;
 
INT32
srvr_comm_get_req(CHAR *buf, INT32 len, int __myptl,
                INT32 msg_type, CHAR *user_data, INT32 user_data_len,
                UINT16 nid, UINT16 pid, UINT16 ptl,
                INT32 blocking, double tmout)
{
PORTAL_INDEX myptl=(PORTAL_INDEX)__myptl;
data_xfer_msg req_msg;
INT32 mle, rc, status;
CHAMELEON mbits, ibits;
unsigned long __mbits, __ibits;
SEND_INFO_TYPE   s_info;
double t1;
extern double dclock(void);

    req_msg.msg_type = msg_type;

    if ((user_data_len > 0) && (user_data != NULL)){
        if (user_data_len > SRVR_USR_DATA_LEN){
            CPerrno = EINVAL;
            return -1;
        }
        memcpy(req_msg.user_data, user_data, user_data_len);
    }

    mbits.ints.i0 =  req_msg.mbits.ints.i0 = unique_key;
    mbits.ints.i1 =  req_msg.mbits.ints.i1 = 0;

    req_msg.ret_ptl  = myptl;
    req_msg.req_len  = len;

    ibits.ints.i0 = 0x0000;
    ibits.ints.i1 = 0x0000;

    unique_key++;
    if (unique_key > max_key) unique_key = start_key;

    UNMKCHAMELEON(__mbits, mbits);
    UNMKCHAMELEON(__ibits, ibits);

    mle = srvr_add_data_buf(buf, len, WRITE_BUFFER, myptl, 
                       __ibits, __mbits);

    if (mle == -1){
        return -1;
    }

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    rc = send_user_msg_phys((char *)&req_msg, sizeof(data_xfer_msg),
                                    nid, pid, ptl, &s_info);

    if (rc){
        srvr_delete_buf((int)myptl, mle);
        return -1;
    }


    if (blocking){

        if (tmout) t1 = dclock();

        status = 0;

        while (1){
            rc = srvr_test_write_buf(myptl, mle);

            if (rc == 1) break;   /* got it */

            if ((tmout && ((dclock() - t1) > tmout)) ||
                (rc < 0)                               ){

                status = -1;
                break;
            }
        }

        srvr_delete_buf((int)myptl, mle);

    }
    else{
        rc = srvr_await_send_completion(&(s_info.send_flag));
        if (rc){
            CPerrno = ESENDTIMEOUT;
            return -1;
        }

        status = mle;
    }

    return status;
}
VOID
srvr_comm_free_handle(int ptl, INT32 mle)
{
    srvr_delete_buf(ptl, mle);
}

/*
** reply to a request for data
*/
static INT32
do_srvr_comm_get_reply(control_msg_handle *handle,
                    VOID *reply_buf, INT32 reply_len, INT32 remoteOffset)
{
INT32 rc;
SEND_INFO_TYPE   s_info;
data_xfer_msg *req_msg;
PTL_MSG_HDR *hdr;

    req_msg = handle->msg;
    hdr     = handle->msg_hdr;

    if ((remoteOffset < 0) || (reply_len < 0)){
        return -1;
    }
    if (remoteOffset + reply_len > req_msg->req_len){
        return -1;
    }

    if ((req_msg->req_len == 0) || (reply_len == 0)){
        return 0;
    }

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    s_info.dst_offset= remoteOffset;
    s_info.dst_matchbits.ints.i0 = req_msg->mbits.ints.i0;
    s_info.dst_matchbits.ints.i1 = req_msg->mbits.ints.i1;

    rc = send_user_msg_phys((char *)reply_buf, reply_len,
             hdr->src_nid, hdr->src_pid, req_msg->ret_ptl,
             &s_info);

    if (rc){
        return -1;
    }

    rc = srvr_await_send_completion(&(s_info.send_flag));
    if (rc){
        CPerrno = ESENDTIMEOUT;
        return -1;
    }

    return 0;

}
INT32
srvr_comm_get_reply(control_msg_handle *handle,
                    VOID *reply_buf, INT32 reply_len)
{
   return do_srvr_comm_get_reply(handle, reply_buf, reply_len, 0);
}
INT32
srvr_comm_get_reply_partial(control_msg_handle *handle,
                    VOID *reply_buf, INT32 reply_len, INT32 offset)
{
   return do_srvr_comm_get_reply(handle, reply_buf, reply_len, offset);
}
