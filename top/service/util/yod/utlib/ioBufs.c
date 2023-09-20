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
#include <stdlib.h>
#include <time.h>

#include "util.h"
#include "rpc_msgs.h"
#include "srvr_comm.h"

/*
** $Id: ioBufs.c,v 1.4 2001/09/26 07:05:49 lafisk Exp $
*/

int ioErrno;

static char *iobufs[NIOBUFS];
static int iobufBusy[NIOBUFS];

int
initialize_work_bufs()
{
int i;

    for (i=0; i<NIOBUFS; i++){
        iobufs[i] = (char *)malloc(IOBUFSIZE);

	if (!iobufs[i]){
	     CPerrno = ENOMEM;
	     return -1;
	}
        if (DBG_FLAGS(DBG_MEMORY)){
	    fprintf(stdout,"memory: 0x%p (%d) IO work buffer\n",iobufs[i], IOBUFSIZE);
	}

	iobufBusy[i] = 0;
    }

    return 0;
}
void
takedown_work_bufs()
{
int i;

    for (i=0; i<NIOBUFS; i++){
        free(iobufs[i]);

        if (DBG_FLAGS(DBG_MEMORY)){
	    fprintf(stdout,"memory: 0x%p IO work buffer FREED\n",iobufs[i]);
	}

        iobufs[i] = NULL;
        iobufBusy[i] = 1;
    }
}
void
free_work_buf(int buf)
{
    if ((buf >= 0) && (buf < NIOBUFS) && iobufs[buf]){
         iobufBusy[buf] = 0;
    }
}
int
get_work_buf()
{
int i;

    for (i=0; i<NIOBUFS; i++){
        if (!iobufBusy[i] && iobufs[i]){
	    iobufBusy[i] = 1;
	    return i;
	}
    }
    return -1;
}
char *
workBufData(int buf)
{
    if ((buf >= 0) && (buf < NIOBUFS)){
        return iobufs[buf];
    }
    else{
        return NULL;
    }
}
/****************************************************************
**  Functions for sending work bufs up to application processes
****************************************************************/

/*
** send_workbuf - Does a put request to send a work buffer up to 
**    a remote process.  Doesn't wait for put to complete.  Doesn't
**    free slot in data portal used by put operation.  If you use
**    this call, you must use check_workbuf_to_app to test for
**    completion and to free the data portal slot.  
**
** check_workbuf_to_app - Checks for completion of put operation,
**    and frees the data portal slot on completion or on error.
**
** cancel_workbuf_to_app - Frees data portal slot used by a put operation.
**
** send_workbuf_and_ack - Sends a work buffer and an ack (in a put operation),
**    awaits completion and frees the data portal slot.
**
** send_workbuf_to_app	- Sends a work buffer (in a put operation),
**    awaits completion and frees the data portal slot.
**
** send_ack_to_app - Sends an ack in a control message.
**
*/

int
send_workbuf(control_msg_handle *mh, int bufnum, int len)
{
int slot;
int nid, pid, ptl;

    nid = SRVR_HANDLE_NID(*mh); 
    pid = SRVR_HANDLE_PID(*mh);
    ptl = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->ctl_ptl;

    slot = srvr_comm_put_req(workBufData(bufnum), len, IO_REPLY, 
               NULL, 0,
	       1, &nid, &pid, &ptl);

    if (slot < 0){
        ioErrno = CPerrno;
        return -1;
    }
    return slot;
}
/*
**   1 - app picked up buffer, 0 - app hasn't been by yet, -1 - error
*/
int
check_workbuf_to_app(int slot)
{
int rc;

    rc = srvr_test_read_buf(slot, 1);

    if (rc == 1){
        srvr_delete_buf(slot);
        return 1;
    }
    if (rc < 0){
        ioErrno = CPerrno;
        srvr_delete_buf(slot);
        return -1;
    }
    return 0;
}
void
cancel_workbuf_to_app(int slot)
{
    srvr_delete_buf(slot);
    return;
}
static int
send_to_app(int type,
            control_msg_handle *mh,
            int bufnum, int len,    /* work buffer full of data */
            hostReply_t *ack)       /* ack tucked into user data field */
{
int slot, rc;
time_t t1;
int nid, pid, ptl;

    if ((len == 0) && (ack == NULL)) return 0;

    nid = SRVR_HANDLE_NID(*mh); 
    pid = SRVR_HANDLE_PID(*mh);
    ptl = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->ctl_ptl;

    if (len == 0){

         rc = srvr_send_to_control_ptl(nid, pid, ptl,
                  type, (char *)ack, sizeof(hostReply_t));

         if (rc){
             ioErrno = CPerrno;
         }
         return rc;
    }

    if (ack != NULL){
        slot = srvr_comm_put_req(workBufData(bufnum), len, type,
                  (char *)ack, sizeof(hostReply_t),
                  1, &nid, &pid, &ptl);
    }
    else{
        slot = srvr_comm_put_req(workBufData(bufnum), len, type,
                  NULL, 0,
                  1, &nid, &pid, &ptl);
    }

    if (slot < 0){
        ioErrno = CPerrno;
        return -1;
    }

    t1 = time(NULL);

    while (1){ 
        rc = check_workbuf_to_app(slot);

        if (rc == 1)  break; 

        if (rc < 0){
            ioErrno = CPerrno;
            return -1;
        }
        if ( (time(NULL) - t1) > daemonWaitLimit){
            ioErrno = ESENDTIMEOUT;
            cancel_workbuf_to_app(slot);
            return -1;
        }
    }

    return 0;
}
/*
** These return 0 on success, -1 on error.
** On error ioErrno is set to the right thing.  (hopefully)
*/
int
send_workbuf_and_ack(control_msg_handle *mh, 
                    int bufnum, int len, hostReply_t *ack)
{
    return send_to_app(IO_REPLY_DONE, mh, bufnum, len, ack);
}
int
send_workbuf_to_app(control_msg_handle *mh, int buf, int len)
{
    return send_to_app(IO_REPLY, mh, buf, len, NULL);
}
int
send_ack_to_app(control_msg_handle *mh, hostReply_t *ack)
{
    return send_to_app(IO_REPLY_DONE, mh, -1, 0, ack);
}
