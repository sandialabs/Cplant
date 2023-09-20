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
** $Id: srvr_comm_get.c,v 1.10 2001/12/17 22:10:01 rbbrigh Exp $
*/
#include "time.h"
#include <srvr_lib.h>
#include <srvr_err.h>

/*
** Functions to perform a server library GET.
**
** A GET operation is originated by a process when it wishes to
** receive a buffer from a remote process.
**
** To perform a get operation:
**
**  (1)   srvr_comm_get_req()     initiate the get request
**
**  (2)   srvr_test_write_buf()   to determine when the requested data
**                                has arrived
**
**  (3)   srvr_delete_buf()       to release the library resources
**                                 allocated to the get operation
**
** To reply to a get operation:
**
**  (1)  srvr_get_next_control_msg()   to receive the get request
**
**  (2)  srvr_comm_get_reply()         to reply with the requested buffer
**           OR
**  (3)  srvr_comm_get_reply_partial()  to reply in partial blocks
**
*/

static ptl_handle_eq_t send_event_queue = SRVR_INVAL_EQ_HANDLE;

/*
** Send a get request.  If call is non-blocking, return a handle.  Otherwise,
** block until completion, and return 0 if OK, -1 on error.
*/
int
srvr_comm_get_req(char *buf,       /* get buffer */
                  int len,         /* get buffer length */
                  int type,        /* a tag passed along to receiver */
                  char *user_data,    /* data accompanying control message */
                  int user_data_len,  /* length of user data */
                  int nid,            /* node ID of remote process */
                  int pid,            /* Portal ID of remote process */
                  int ptl,            /* Control portal at remote process */
                  int blocking,   /* wait till data arrives? */
                  int tmout)      /* time out in seconds for blocking call, or 0 */
{
int rc, slot, status;
time_t t1;

    slot = srvr_add_data_buf(buf, len, WRITE_DATA_BUFFER);

    if (slot < 0){
        return -1;
    }

    rc = srvr_send_it(nid, pid, ptl,
                    type, user_data, user_data_len,
                    DATAPORTALS, len, slot);


    if (rc){
        srvr_delete_buf(slot);
        return -1;
    }

    if (!blocking){

        return slot;
    }
    /*
    ** wait for the requested data to come in
    */

    status = 0;
    t1 = time(NULL);

    while (status == 0){

        rc = srvr_test_write_buf(slot);

        if (rc == 1){  /* it's in */
            break;
        }
        else if (rc < 0){
            status = -1;
        }
        else if (  tmout && ((time(NULL) - t1) > tmout)){ 

            CPerrno = ERECVTIMEOUT;
            status = -1;
        }
    }

    srvr_delete_buf(slot);

    return status;
}
int
do_get_reply(control_msg_handle *mh, void *rbuf, int len, int offset)
{
ptl_md_t mddef;
ptl_handle_md_t md;
ptl_process_id_t id;
ptl_event_t ev;
time_t t1;
int rc, status, ack, sent, req_len;
ptl_hdr_data_t userData=0;

    req_len = SRVR_HANDLE_TRANSFER_LEN(*mh);

    if ((offset + len) > req_len){
        len = req_len - offset;
    }
    if (len < 1){
        CPerrno = EINVAL;
        return -1;
    }

    if (send_event_queue == SRVR_INVAL_EQ_HANDLE){
        rc = PtlEQAlloc(__SrvrLibNI, 3, &send_event_queue);

        if (rc != PTL_OK){
            P3ERROR;
        }
    }

    id.nid = SRVR_HANDLE_NID(*mh);
    id.pid = SRVR_HANDLE_PID(*mh);
    id.addr_kind = PTL_ADDR_NID;

    mddef.start = rbuf;
    mddef.length = len;
    mddef.threshold = 2;   /* send and reply */
    mddef.max_offset = len;
    mddef.options = 0;
    mddef.user_ptr = NULL;
    mddef.eventq = send_event_queue;

    rc = PtlMDBind(__SrvrLibNI, mddef, &md);

    if (rc != PTL_OK){
        P3ERROR;
    }

    rc = PtlPut(md, PTL_ACK_REQ, 
                id, DATAPORTALS, SRVR_ACL_ANY, 
		SRVR_HANDLE_MATCHBITS(*mh), offset, userData);

    if (rc != PTL_OK){
        PtlMDUnlink(md);
        P3ERROR;
    }

    t1 = time(NULL);

    status = 0;

    ack = sent = 0;

    while (1){

        rc = PtlEQGet(mddef.eventq, &ev);

	/*
	** According to Rolf, the ACK can strangely enough precede 
	** the SENT in the case where the receiving node's confirmation
	** that data has arrived gets lost, and then the ACK gets
	** sent, and then confirmation is re-sent.
	*/

        if ((rc == PTL_OK) || (rc == PTL_EQ_DROPPED)){

	    if (rc == PTL_EQ_DROPPED){
	        log_msg("warning - events dropped on queue in do_get_reply");
	    }

            if (ev.type == PTL_EVENT_ACK){
	        ack = 1;
		if (sent) break;
		continue;
            }
            else if (ev.type == PTL_EVENT_SENT){
	        sent = 1;
		if (ack) break;
                continue;
            }
            else{
                CPerrno = EPROTOCOL;
                status = -1;
                break;
            }
        }
        else if (rc == PTL_EQ_EMPTY){

            if ((time(NULL) - t1) > GET_REPLY_TIMEOUT){

                CPerrno = ESENDTIMEOUT;

		srvrHandleSendTimeout(mddef, md); /* does the PtlMDUnlink */

                return -1;
            }
        }
        else{
            PtlMDUnlink(md);
            P3ERROR;
        }
    }
    PtlMDUnlink(md);

    return status;
}
int
srvr_comm_get_reply(control_msg_handle *mh, void *rbuf, int len)
{
    if (!SRVR_IS_VALID_HANDLE(*mh)){
        CPerrno = EINVAL;
        return -1;
    }
    return do_get_reply(mh, rbuf, len, 0);
}
int
srvr_comm_get_reply_partial(control_msg_handle *mh, void *rbuf, int len,
                            int offset)
{
    if (!SRVR_IS_VALID_HANDLE(*mh)){
        CPerrno = EINVAL;
	return -1;
    }
    return do_get_reply(mh, rbuf, len, offset);
}
