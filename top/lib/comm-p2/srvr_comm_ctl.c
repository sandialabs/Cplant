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
**  $Id: srvr_comm_ctl.c,v 1.1 2000/11/04 03:56:31 lafisk Exp $
*/

#include <string.h>
#include "puma.h"
//#include "portal.h"
#include "sysptl.h"
#include "srvr_comm.h"
//#include "errorUtil.h"

//#include "srvr_comm_ctl.h"
#include "srvr_coll_util.h"

#include <malloc.h>


/******************************************************************
** Small control messages are sent to request data puts or gets.
** The control messages can also be used for acks, or to transmit
** a small amount of status information.
**
** Control messages come into a circular independent block list.
*******************************************************************
*/
int
srvr_await_send_completion(volatile unsigned int *flag)
{
double t1;
extern double dclock(void);
 
   t1 = dclock();
 
   while (*flag == 0){
       if ((dclock() - t1) > SEND_TIMEOUT){
           return -1;
       }
   }
   return 0;
}

static int
control_portal_create(int ptl, INT32 max_num_msgs)
{
INT32 rc, i;
IND_MD_BUF_DESC *buf_table;
CHAR *msg_bufs, *pbuf;

    /*
    ** allocate and initialize buffer descriptor table and message
    ** buffers
    */

    buf_table = (IND_MD_BUF_DESC *)calloc(sizeof(IND_MD_BUF_DESC), 
                                          max_num_msgs);

    if (!buf_table){
	CPerrno = ENOMEM;
        return -1;
    }

    msg_bufs = (CHAR *)malloc(sizeof(data_xfer_msg) * max_num_msgs);

    if (!msg_bufs){
	CPerrno = ENOMEM;
        free(buf_table);
        return -1;
    }

    rc = portal_lock_buffer(buf_table, sizeof(IND_MD_BUF_DESC) * max_num_msgs);

    if (rc == -1){
        free(buf_table);
        free(msg_bufs);
        CPerrno = ELOCK;
        return -1;
    }

    rc = portal_lock_buffer(msg_bufs, sizeof(data_xfer_msg) * max_num_msgs);

    if (rc == -1){
        free(buf_table);
        free(msg_bufs);
        CPerrno = ELOCK;
        return -1;
    }

    for (i=0, pbuf = msg_bufs; i<max_num_msgs; i++){

         buf_table[i].stage = 0;
         buf_table[i].hdr.msg_len = -1;
   
         buf_table[i].buf = pbuf;
         buf_table[i].buf_len = sizeof(data_xfer_msg);
         pbuf += sizeof(data_xfer_msg); 
    }

    buf_table[0].first_read = 0;
    buf_table[0].last_probe = -1;
    buf_table[0].next_free  = 0;
    buf_table[0].ref_count  = 1;

    /*
    ** attach list of independent blocks to portal
    */
    IND_MD(ptl).buf_desc_table = buf_table;
    IND_MD(ptl).num_buf_desc   = max_num_msgs;

    PTL_DESC(ptl).mem_op = IND_CIRC_SV_HDR_BDY;

    /*
    ** control portal is ready to go
    */
    SPTL_ACTIVATE(ptl);

    return 0;
}

int
srvr_init_control_ptl(INT32 max_num_msgs)
{
PORTAL_INDEX ptl;
INT32 rc;

    /*
    ** allocate a new portal
    */

    if (max_num_msgs <= 0){
        CPerrno = EINVAL;
        return SRVR_INVAL_PTL;
    }

    rc = sptl_l_alloc(&ptl);

    if (rc != ESUCCESS){
        return SRVR_INVAL_PTL;
    } 

    rc = sptl_init(ptl);

    if (rc != ESUCCESS){
        return SRVR_INVAL_PTL;
    } 

    rc = control_portal_create(ptl, max_num_msgs);

    if (rc){
	sptl_dealloc(ptl);
	return SRVR_INVAL_PTL;
    }
    return (int)ptl;
}
int
srvr_init_control_ptl_at(INT32 max_num_msgs, int __ptl)
{
PORTAL_INDEX ptl = (PORTAL_INDEX)__ptl;
INT32 rc;

    if ((max_num_msgs <= 0) || (ptl > MAX_PORTAL) || sptl_test_alloc(ptl)){
        CPerrno = EINVAL;
        return -1;
    }

    sptl_set_alloc(ptl);

    rc = sptl_init(ptl);

    if (rc != ESUCCESS){
	CPerrno = EPORTAL;
        return -1;
    } 

    rc = control_portal_create(ptl, max_num_msgs);

    if (rc){
	sptl_dealloc(ptl);
	return SRVR_INVAL_PTL;
    }

    return 0;
}
/*
** Control portals exist to send requests to GET data from a remote
** process' memory, or PUT data in a remote process' memory.  But
** they carry SRVR_USR_DATA_LEN bytes of user data, so they can also
** be used to send a short message, with no subsequent data transfer
** expected.
*/
INT32
srvr_send_to_control_ptl(INT32 nid, INT32 pid, int __ptl, 
               INT32 msg_type, CHAR *user_data, INT32 len)
{
PORTAL_INDEX ptl = (PORTAL_INDEX)__ptl;
data_xfer_msg msg;
SEND_INFO_TYPE s_info;
INT32 rc;

    if ( user_data && ((len > SRVR_USR_DATA_LEN) || (len < 0))) {

        CPerrno = EINVAL;
        return -1;
    }

    msg.msg_type = msg_type;
    msg.ret_ptl  = SRVR_INVAL_PTL;
    msg.req_len  = 0;
    msg.mbits.ints.i0 = 0;
    msg.mbits.ints.i1 = 0;

    if (len && user_data){
        memcpy(msg.user_data, user_data, len);
    }

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    s_info.dst_matchbits.ints.i0 = msg_type;   /* shows up on ptlDebug */

    rc = send_user_msg_phys((char *)&msg, sizeof(data_xfer_msg),
                       nid, pid, ptl, &s_info);

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

/*
** Returns 0 if no new control messages, -1 on error, 1 if
** a new message is found. 
**
** If handle is NULL, we just consume the message after setting
** the msg_type and xfer_len fields (if they're not NULL).
**
** Otherwise, we set handle to point to the handle required by 
** request processing routines.  Set msg_type to message type 
** in data_xfer_req.  Write the data xfer request length to xfer_len.
** Sets user_data to point to user data section in data_xfer_req.
** Note this (and handle) need not persist after control message is 
** freed.
** 
*/
INT32
srvr_get_next_control_msg(int __ptl, control_msg_handle *handle,
              INT32 *msg_type, INT32 *xfer_len, CHAR **user_data)
{
PORTAL_INDEX ptl = (PORTAL_INDEX)__ptl;
INT32 rc;
IND_MD_BUF_DESC *buf_hdr;
control_msg_handle mh;

    buf_hdr  = NULL;

    if ( (ptl > MAX_PORTAL) || 
         (PTL_DESC(ptl).mem_op != IND_CIRC_SV_HDR_BDY)){

        CPerrno = EINVAL;
        return -1;
    }

    rc = sptl_ind_msg_probe(&(IND_MD(ptl)), IND_CIRC_SV_HDR_BDY,
                 &buf_hdr, NONBLOCKING);

    if (rc != ESUCCESS){
        return -1;
    }

    if (buf_hdr == NULL){   /* no new messages to report */
        return 0;
    }


    mh.msg_hdr = &(buf_hdr->hdr);
    mh.msg     =  (data_xfer_msg *)(buf_hdr->buf);

/*
printf("srvr_get_next_control_msg: got one from %d\n",mh.msg_hdr->src_nid);
*/

    if (msg_type != NULL){
        *msg_type = mh.msg->msg_type;
    }
    if (xfer_len != NULL){
        *xfer_len = mh.msg->req_len;
    }

    if (handle != NULL){
        handle->msg_hdr = mh.msg_hdr;
        handle->msg     = mh.msg;

        if (user_data != NULL){
            *user_data = &(handle->msg->user_data[0]);
        }
    } 
    /*
    ** Just consume the control message since they can't
    ** free it without a handle.
    ** The user_data in the control message is invalid after
    ** message is freed, since it may be overwritten at any time.
    */

    else{
        if (user_data) *user_data = NULL;

        srvr_free_control_msg((int)ptl,  &mh);
    }

    return 1;
}
INT32
srvr_free_control_msg(int __ptl, control_msg_handle *handle)
{
PORTAL_INDEX ptl = (PORTAL_INDEX)__ptl;
INT32 rc;
PTL_MSG_HDR *msg_hdr;

    if ( (ptl > MAX_PORTAL) || 
         (PTL_DESC(ptl).mem_op != IND_CIRC_SV_HDR_BDY) ||
         (handle == NULL) ){

        CPerrno = EINVAL;
        return -1;
    }

    /*
    ** we use the fact that the PTL_MSG_HDR is the first
    ** field in the IND_MD_BUF_DESC
    */
    msg_hdr = handle->msg_hdr;

    if (msg_hdr == NULL){
        CPerrno = EINVAL;
        return -1;
    }
    rc = sptl_ind_msg_free(&(IND_MD(ptl)), IND_CIRC_SV_HDR_BDY,
                       (IND_MD_BUF_DESC *)msg_hdr);

    if (rc != ESUCCESS){
        return -1;
    }

    return 0;
}

INT32
srvr_release_control_ptl(int __ptl)
{
PORTAL_INDEX ptl = (PORTAL_INDEX)__ptl;

    if ((ptl > MAX_PORTAL) ||
        (PTL_DESC(ptl).mem_op != IND_CIRC_SV_HDR_BDY)){

        CPerrno = EINVAL;
        return -1;
    }

    free(IND_MD(ptl).buf_desc_table[0].buf);

    free(IND_MD(ptl).buf_desc_table);

    sptl_dealloc(ptl);

    return 0;
}
VOID
srvr_display_control_msg(control_msg_handle *h)
{
data_xfer_msg *msg;
PTL_MSG_HDR *msg_hdr;
int i;
char *ud;

   msg_hdr = h->msg_hdr;
   msg     = h->msg;

   printf("CONTROL_MESSAGE:\n");

   printf("   HDR: from nid %d pid %d group %d rank %d\n",
  msg_hdr->src_nid, msg_hdr->src_pid, msg_hdr->src_grp, msg_hdr->src_rank);

   printf("        to pid %d portal %d offset %d 0x%08x 0x%08x\n",
        msg_hdr->dst_pid, msg_hdr->dst_ptl, msg_hdr->dst_offset, 
        msg_hdr->dst_mbits.ints.i0, msg_hdr->dst_mbits.ints.i1);

   printf("  BODY: message type %d\n",msg->msg_type);
   printf("        return info: portal %d, len %d, 0x%08x 0x%08x\n",
      msg->ret_ptl, msg->req_len, msg->mbits.ints.i0, msg->mbits.ints.i1);

   printf("        user data bytes: 0x");
   for (i=1, ud=msg->user_data ; i<=SRVR_USR_DATA_LEN; i++){
       printf(" %02x",(int)*ud++);
       if ((i % 16) == 0) printf("\n                         0x");
   }
   
   printf("\n");
}
/*
** P3 compatibility
*/
int 
server_library_init()
{
   return 0;
}
void 
server_library_done()
{
    return;
}

