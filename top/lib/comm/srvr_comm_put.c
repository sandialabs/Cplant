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
** $Id: srvr_comm_put.c,v 1.16 2001/12/17 22:10:01 rbbrigh Exp $
*/
#include "time.h"

#include <srvr_lib.h>
#include <srvr_err.h>

/*
** Functions to perform a server library PUT.
**
** A PUT operation is originated by a process when it wishes to
** put a buffer in the memory of a remote process or processes.
**
** To perform a put operation:
**
**  (1)   srvr_comm_put_req()     initiate the put request
**
**  (2)   srvr_test_read_buf()    to determine when the remote processes
**                                 have picked up the data
**
**  (3)   srvr_delete_buf()       to release the library resources
**                                 allocated to the put operation
** 
** To reply to a put operation:
**
**  (1)  srvr_get_next_control_msg()   to receive the put request
**
**  (2)  srvr_comm_put_reply()         to retreive the put buffer
**           OR
**  (3)  srvr_comm_put_reply_partial()    to retreive it in partial blocks
**
*/

static ptl_handle_eq_t recv_event_queue = SRVR_INVAL_EQ_HANDLE;

/*
** Send a put request, return value is a handle to use when
** checking for completion, or -1 on error.
**
** Frequently, the portal process ID and control portal ID for all
** receivers are the same.  If the second value in the pidlist
** or ptllist is -1, we'll use the first value for all receivers.
*/
int
srvr_comm_put_req( char *buf,       /* put buffer */
		  int len,         /* put buffer length */
                  int type,         /* a tag passed along to receiver */
		  char *user_data,    /* data accompanying control message */
                  int user_data_len,  /* length of user data */
                  int ntargets,     /* number of remote processes     */
		  int *nidlist,     /* node ID of remote processes    */
		  int *pidlist,    /* Portal ID of remote processes   */
		  int *ptllist)    /* Control portal at remote process */
		 
{
int i, rc, slot;
int rpid, rptl, pid, ptl;

    slot = srvr_add_data_buf(buf, len, READ_DATA_BUFFER);

    if (slot < 0){
        return -1;
    }

    if ((ntargets >= 2) && (pidlist[1] == -1)){
        rpid = pidlist[0];
    }
    else{
        rpid = -1;
    }
    if ((ntargets >= 2) && (ptllist[1] == -1)){
        rptl = ptllist[0];
    }
    else{
        rptl = -1;
    }

    for (i=0; i<ntargets; i++){

        if (rpid >= 0) pid = rpid;
        else pid = pidlist[i];

        if (rptl >= 0) ptl = rptl;
        else ptl = ptllist[i];

        rc = srvr_send_it(nidlist[i], pid, ptl,
	            type, user_data, user_data_len,
		    DATAPORTALS, len, slot);


        if (rc){
	    srvr_delete_buf(slot);
	    return -1;
	}
    }

    return slot;
}
/*
** Reply to a put request
*/
static int
do_put_reply(control_msg_handle *mh, void *rbuf, int len, int offset)
{
ptl_md_t mddef;
ptl_handle_md_t md;
ptl_process_id_t id;
ptl_event_t ev;
time_t t1;
int rc, status, req_len;

    req_len = SRVR_HANDLE_TRANSFER_LEN(*mh);

    if ((offset + len) > req_len){ 
        len = req_len - offset;
    }
    if (len < 1){
        CPerrno = EINVAL;
        return -1;
    }
    if (recv_event_queue == SRVR_INVAL_EQ_HANDLE){
        rc = PtlEQAlloc(__SrvrLibNI, 3, &recv_event_queue);

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
    mddef.eventq = recv_event_queue;

    rc = PtlMDBind(__SrvrLibNI, mddef, &md);

    if (rc != PTL_OK){
        P3ERROR;
    }
    
    rc = PtlGet(md, id, DATAPORTALS, SRVR_ACL_ANY, 
                SRVR_HANDLE_MATCHBITS(*mh), offset);

    if (rc != PTL_OK){
	PtlMDUnlink(md);
        P3ERROR;
    }

    t1 = time(NULL);

    status = 0;

    while (1){

        rc = PtlEQGet(mddef.eventq, &ev);

	if ((rc == PTL_OK) || (rc == PTL_EQ_DROPPED)){

	    if (rc == PTL_EQ_DROPPED){
	        log_msg("warning - events dropped on event queue in do_put_reply");
	    }

	    if (ev.type == PTL_EVENT_REPLY){
	        break;   /* got it */
	    }
	    else if (ev.type == PTL_EVENT_SENT){
	        continue;
            }
	    else{
	        CPerrno = EPROTOCOL;
		status = -1;
		break;
	    }
	}
	else if (rc == PTL_EQ_EMPTY){

	    if ((time(NULL) - t1) > PUT_REPLY_TIMEOUT){

	        CPerrno = ERECVTIMEOUT;

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
srvr_comm_put_reply(control_msg_handle *mh, void *rbuf, int len)
{
    if (!SRVR_IS_VALID_HANDLE(*mh)){
        CPerrno = EINVAL;
	return -1;
    }
    return do_put_reply(mh, rbuf, len, 0);
}
int
srvr_comm_put_reply_partial(control_msg_handle *mh, void *rbuf, int len, 
                            int offset)
{
    if (!SRVR_IS_VALID_HANDLE(*mh)){
        CPerrno = EINVAL;
	return -1;
    }
    return do_put_reply(mh, rbuf, len, offset);
}

