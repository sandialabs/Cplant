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
** $Id: host_rpc.c,v 1.1 2001/11/17 01:23:08 lafisk Exp $
**
**  Note: Don't do log_warnings, etc. because these
**   resolve to opens (of /etc/localtime) which end
**   up right back here in _host_rpc.  For the same
**   reason, don't do printf's. etc.  You can use
**   PCT_DUMP to send a code to the PCT which will
**   be logged in /var/log/cplant.
*/
#include <time.h>
#include <string.h>
#include "rpc_msgs.h"
#include "srvr_comm.h"
#include "srvr_err.h"

static int the_seq_no=1;

/*
** Flag so we don't call host_rpc from a call made from host_rpc...
*/
static int host_rpc_on=0;

INT32
_host_rpc(hostCmd_t *cmd, hostReply_t *ack,
          INT8 cmd_type,
          int srvr_nid, int srvr_pid, int srvr_ptl,
    const CHAR *r_buf, INT32 r_len, CHAR *w_buf, INT32 w_len)
{

INT32 rc, type, status, rpcType;
int nmsgs, slot, rblocks, wblocks, len, totlen;
control_msg_handle mhandle;
char *c;
time_t t1;

    if (host_rpc_on == 1){
        return -1;
    }
    host_rpc_on = 1;

    if ((cmd_type >= FIRST_JOB_CMD_NUM) &&
        (cmd_type <= LAST_JOB_CMD_NUM)     ){

        rpcType = JOB_MANAGEMENT;
    }
    else if ((cmd_type >= FIRST_CMD_NUM) &&
        (cmd_type <= LAST_CMD_NUM)     ){

        rpcType = YO_ITS_IO;
    }
    else{
        ERRNO = EINVAL;
        host_rpc_on = 0;
        return -1;
    }

    if (( sizeof(hostCmd_t) > SRVR_USR_DATA_LEN) ||
        ( sizeof(hostReply_t) > SRVR_USR_DATA_LEN)  ){

        PCT_DUMP(4001,sizeof(hostCmd_t),sizeof(hostReply_t),
	         SRVR_USR_DATA_LEN,0,0,0);
        ERRNO = EINVAL;
        host_rpc_on = 0;
        return -1;
    }

    SRVR_CLEAR_HANDLE(mhandle);
    slot = -1;
    status = 0;

    /* the IO request */

    cmd->type    = cmd_type;
    cmd->ctl_ptl = _yod_io_ctl_ptl;
    cmd->my_seq_no = the_seq_no++;

    if (the_seq_no > 999999) the_seq_no = 1;

    /* yod has to throttle IO due to memory limitations */

    rblocks = wblocks = 0;

    if (r_buf){
        rblocks = NIOBLOCKS(r_len);
    }
    if (w_buf){
        wblocks = NIOBLOCKS(w_len);
    }

    /* Send the request */

    if (r_buf){

        slot = srvr_comm_put_req((void *)r_buf, r_len, rpcType,
                  (char *)cmd, sizeof(hostCmd_t),
                  1, &srvr_nid, &srvr_pid, &srvr_ptl);

        if (slot < 0){
	    PCT_DUMP(4002,slot,0,0,0,0,0);
            status = -1;
            goto finish;
        }

        t1 = time(NULL);

        while(1){
            rc = srvr_test_read_buf(slot, rblocks);

            if (rc == 1) break;    /* yod/fyod picked up r_buf */

            if (rc == -1){         /* error */
                PCT_DUMP(4003,rc,0,0,0,0,0);
                status = -1;
                goto finish;
            }
            /*
            ** do we want to time out here after awhile ?
            */
            if ((time(NULL) - t1) > (5*60)){
	        PCT_DUMP(4004,0,0,0,0,0,0);
                t1 = time(NULL);
            }
        }
        rc = srvr_delete_buf(slot);
        slot = -1;

        if (rc){
            PCT_DUMP(4005,0,0,0,0,0,0);
            status = -1;
            goto finish;
        }
    }
    else{
        rc = srvr_send_to_control_ptl(srvr_nid, srvr_pid, srvr_ptl,
                  rpcType, (char *)cmd, sizeof(hostCmd_t));

        if (rc){
            PCT_DUMP(4006,rc,0,0,0,0,0);
            status = -1;
            goto finish;
        }
    }
    /*
    ** Await the ack and possibly reply buffer - if reply buffer
    **  is sent in more than one block, last block contains the
    **  ack.
    */

    nmsgs = (wblocks ? wblocks : (ack ? 1 : 0));
    c = (w_buf ? w_buf : NULL);
    totlen = 0;

    while (nmsgs--){

        t1 = time(NULL);

        while (1){

            rc = srvr_get_next_control_msg(_yod_io_ctl_ptl, &mhandle,
                             NULL, NULL, NULL);

            if (rc == 1) break;    /* yod/fyod has replied */

            if (rc < 0){
                PCT_DUMP(4007,rc,0,0,0,0,0);
                SRVR_CLEAR_HANDLE(mhandle);
                status = -1;
                goto finish;
            }

            /*
            ** timeout maybe?
            */
            if ((time(NULL) - t1) > (5*60)){
                t1 = time(NULL);
            }
        }
        type = SRVR_HANDLE_TYPE(mhandle);
        len  = SRVR_HANDLE_TRANSFER_LEN(mhandle);

        if (type == IO_REPLY_DONE){

            /*
            ** The ack.  It may contain a buffer of data too.
            */

            memcpy(ack, SRVR_HANDLE_USERDEF(mhandle), sizeof(hostReply_t));

            if (ack->your_seq_no != cmd->my_seq_no){

                PCT_DUMP(4008,0,0,0,0,0,0);
                CPerrno = EPROTOCOL;
                status = -1;
                goto finish;
            }

            if (c && (len > 0) && (len + totlen <= w_len)){

                rc = srvr_comm_put_reply(&mhandle, (void *)c, len);

                if (rc){
                    PCT_DUMP(4009,rc,0,0,0,0,0);
                    status = -1;
                }
            }
            goto finish;
        }
        else if (type != IO_REPLY){
            PCT_DUMP(4010,type,0,0,0,0,0);
            CPerrno = EPROTOCOL;
            status = -1;
            goto finish;
        }

        if (w_buf){

            if ((len + totlen) > w_len){
                PCT_DUMP(4011,len,totlen,w_len,0,0,0);
                CPerrno = EPROTOCOL;
                status = -1;
                goto finish;
            }
            else{
                totlen += len;
            }

            rc = srvr_comm_put_reply(&mhandle, (void *)c, len);

            if (rc){
                PCT_DUMP(4012,rc,0,0,0,0,0);
                status = -1;
                goto finish;

            }

            c += len;
        }
        srvr_free_control_msg(_yod_io_ctl_ptl, &mhandle);
        SRVR_CLEAR_HANDLE(mhandle);
    }

    if (ack && (type != IO_REPLY_DONE)){
	PCT_DUMP(4013,type,0,0,0,0,0);
        status = -1;
    }

finish:

    if (SRVR_IS_VALID_HANDLE(mhandle)){
        srvr_free_control_msg(_yod_io_ctl_ptl, &mhandle);
    }
    if (slot >= 0){
        srvr_delete_buf(slot);
    }
    host_rpc_on = 0;

    return status;
}


