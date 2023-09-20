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
** $Id: srvr_comm_put.c,v 1.1 2000/11/04 03:56:32 lafisk Exp $
*/
#include <string.h>
#if !defined(__GLIBC__) || (__GLIBC__ < 2) \
  || (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0)
typedef long int intptr_t;
#else
#include <stdint.h>
#endif
#include "puma.h"
#include "sysptl.h"
#include "srvr_comm.h"
#include "srvr_comm_put.h"
#include "srvr_comm_ctl.h"
#include "srvr_comm_data.h"
#include "srvr_coll_util.h"

/*
** To push data to a remote process:
**
**    Initially, before any transfers -
**
**    need to have nid/pid of remote process
**    need to have portal on which remote process listens for requests
**      of type data_xfer_msg
**
**    open a data portal to hold match list entries pointing to
**      local data buffers containing data to go
**
**    For each xfer -
**
**    open a read memory descriptor for this operation
**
**    send data_xfer_msg requesting remote process to come get data
**
**    test read memory descriptor for count of accesses to buffer
**
**    free the read memory descriptor
*/

/*
** Set up a reply portal over the buffer you want pulled from
** remote nodes.
**
** buf & len: where data resides
** myptl :  the data portal opened for transfers
**
** Return value is a match list entry number.  Use it when
** sending put messages to remote processes.
** Error return is -1.
*/

static UINT32 start_key  = 0x00000001;
static UINT32 unique_key = 0x00000001;
static UINT32 max_key    = 0x80000000;

INT32
srvr_comm_put_buffer(VOID *buf, INT32 len, int __myptl)
{
PORTAL_INDEX myptl=(PORTAL_INDEX)__myptl;
INT32 mle;
CHAMELEON mbits, ibits;
unsigned long __mbits, __ibits;

    ibits.ints.i0 = 0x0000;
    ibits.ints.i1 = 0x0000;

    mbits.ints.i0 = 0;
    mbits.ints.i1 = unique_key;

    unique_key++;
    if (unique_key > max_key) unique_key = start_key;

    UNMKCHAMELEON(__mbits, mbits);
    UNMKCHAMELEON(__ibits, ibits);

    mle = srvr_add_data_buf(buf, len, READ_BUFFER, myptl, 
                       __ibits, __mbits);

/*
printf("srvr_comm_put_buffer: mle %d mbits 0x%08x 0x%08x\n",
mle, mbits.ints.i0, mbits.ints.i1);
*/

    if (mle == -1){
        return -1;
    }

    return mle;
}
/*
** Send a request to a remote process to pull data from
** a local reply portal.
**
** myptl : my data transfer portal, from srvr_init_data_ptl
** mle : the handle returned by srvr_comm_put_buffer
** msg_type : identifier sent along to remote processes
** nid,pid,ptl : of remote process
** user_data: optional user_data_len bytes to tuck in user_data field
**
*/
INT32
srvr_comm_put_req(INT32 msg_type,  int __myptl, INT32 mle,
                  UINT16 nid, UINT16 pid, UINT16 ptl,
                  CHAR *user_data, INT32 user_data_len)
{
PORTAL_INDEX myptl=(PORTAL_INDEX)__myptl;
data_xfer_msg req_msg;
INT32 rc;
SEND_INFO_TYPE   s_info;


    if ((myptl > MAX_PORTAL) ||
        (PTL_DESC(myptl).mem_op != MATCHING) ||
        (mle <= 0) ||
        (mle >= (INT32) MATCH_MD(myptl).list_len) ) {

         CPerrno = EINVAL;
         return -1;
    }
    req_msg.msg_type = msg_type;

    if (user_data){

        if ((user_data_len > 0) && (user_data_len <= SRVR_USR_DATA_LEN)){
            memcpy(req_msg.user_data, user_data, user_data_len);
        }
        else{
            CPerrno = EINVAL;
            return -1;
        }
    }

    req_msg.mbits.ints.i0 = MLE(myptl, mle).must_mbits.ints.i0;
    req_msg.mbits.ints.i1 = MLE(myptl, mle).must_mbits.ints.i1;

    req_msg.ret_ptl  = myptl;
    req_msg.req_len  = MLE(myptl, mle).mem_desc.single.buf_len;

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    s_info.dst_matchbits.ints.i0 = msg_type;

    rc = send_user_msg_phys((char *)&req_msg, sizeof(data_xfer_msg),
                                    nid, pid, ptl, &s_info);

    if (rc){
	CPerrno = EPORTAL;
        return -1;
    }

    rc = srvr_await_send_completion(&(s_info.send_flag));

    if (rc){
	CPerrno = ESENDTIMEOUT;
	return -1;
    }

    return 0;
}
/*
** Reply to a read memory request from a remote process.  Return 0
** when data has been received locally.  Return -1 on error.
*/
static PORTAL_INDEX put_recv_portal = SRVR_INVAL_PTL;
static INT32 init_put_recv_portal(void);

#define activate_put_recv_portal(recv_buf, recv_len) \
{                                                     \
    SING_MD(put_recv_portal).msg_cnt  = 0;           \
    SING_MD(put_recv_portal).buf      = recv_buf;    \
    SING_MD(put_recv_portal).buf_len  = recv_len;    \
    SING_MD(put_recv_portal).rw_bytes = -1;          \
    SPTL_ACTIVATE(put_recv_portal);                  \
}
/*
** The timeout should really be an argument to srvr_comm_put_reply since
** the reasonable time to wait for data to come in is application
** dependent.  For now we set a large timeout, just so we don't get
** stuck forever in a while loop.
*/
#define PUT_REPLY_TIMEOUT  (3.0 * 60.0)
static INT32
do_srvr_comm_put_reply(control_msg_handle *mh,
                       VOID *recv_buf, INT32 buf_len, INT32 remoteOffset)
{
INT32 rc, status, offset, avail_len, trans_len;
SEND_INFO_TYPE s_info;
data_xfer_msg *req_msg;
PTL_MSG_HDR *hdr;
double t1;
extern double dclock(void);

    req_msg = mh->msg;
    hdr     = mh->msg_hdr;

    if ((remoteOffset < 0) || (remoteOffset >= req_msg->req_len)){
        return -1;
    }
    avail_len = req_msg->req_len - remoteOffset;

    trans_len = ((buf_len < avail_len) ? buf_len : avail_len);

    /*
    ** point the put_recv portal to the receive buffer
    */
    if (put_recv_portal == SRVR_INVAL_PTL){
        rc = init_put_recv_portal();

        if (rc < 0){
            return -1;
        }
    }

    offset = (intptr_t) recv_buf & 0x07;

    rc = portal_lock_buffer(((char *)recv_buf - offset), trans_len + offset);

    if (rc){
        CPerrno = ELOCK;
        return -1;
    }

    activate_put_recv_portal(recv_buf, trans_len);

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    s_info.dst_matchbits.ints.i0 = req_msg->mbits.ints.i0;
    s_info.dst_matchbits.ints.i1 = req_msg->mbits.ints.i1;

    s_info.dst_offset = remoteOffset;
    s_info.return_portal = put_recv_portal;
    s_info.reply_len    = trans_len;

    if (SrvrDbg){
        printf("get put data: send to %d/%d/%d 0x%08x 0x%08x\n",
                  hdr->src_nid, hdr->src_pid, req_msg->ret_ptl,
                  req_msg->mbits.ints.i0, req_msg->mbits.ints.i1);
        printf("              return to my portal %d, %d bytes\n\n",
                               put_recv_portal, avail_len);
    }

    rc = send_user_msg_phys((char *)NULL, 0,
             hdr->src_nid, hdr->src_pid, req_msg->ret_ptl,
             &s_info);

    if (rc){
        status = -1;
    }
    else{
        t1 = dclock();

        while (1){

            if ((INT32)SING_MD(put_recv_portal).rw_bytes >=
               (INT32)SING_MD(put_recv_portal).buf_len)      {
                status = 0;
                break;
            }

            if ( (dclock() - t1) > PUT_REPLY_TIMEOUT){
                CPerrno = ERECVTIMEOUT;
                status = -1;
                break;
            }
        }

    }

    rc = portal_unlock_buffer(((char *)recv_buf - offset), trans_len + offset);

    if (rc){
        CPerrno = EUNLOCK;
        status = -1;
    }

    SPTL_DEACTIVATE(put_recv_portal);

    return status;
}
INT32
srvr_comm_put_reply(control_msg_handle *mh, VOID *recv_buf, INT32 buf_len)
{
   return do_srvr_comm_put_reply(mh, recv_buf, buf_len, 0);
}
INT32
srvr_comm_put_reply_partial(control_msg_handle *mh,
                     VOID *recv_buf, INT32 buf_len, INT32 offset)
{
   return do_srvr_comm_put_reply(mh, recv_buf, buf_len, offset);
}

static INT32
init_put_recv_portal(void)
{
INT32 rc;

    rc = sptl_l_alloc(&put_recv_portal);

    if (rc){
        put_recv_portal = SRVR_INVAL_PTL;
        return -1;
    }
    rc = sptl_init(put_recv_portal);

    if (rc){
        put_recv_portal = SRVR_INVAL_PTL;
        sptl_dealloc(put_recv_portal);
        return -1;
    }

    PTL_DESC(put_recv_portal).mem_op = SINGLE_RCVR_OFF_SV_BDY;

    return 0;
}
