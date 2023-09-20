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
/* $Id: host_read.c,v 1.7 2001/09/26 07:05:46 lafisk Exp $ */



#include "puma.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "host_read.h"
#include "positionFilePtr.h"
#include "host_msg.h"
#include "fileHandle.h"
#include "util.h"
#include "srvr_comm.h"
#include "rpc_msgs.h"


#ifdef PIPELINED
   /*
   ** this is faster, but occasionally hangs, and I don't have
   ** time to debug it now
   */
static int check_workbufs(int *slots);

ssize_t
host_read(fileHandle_t *fh, control_msg_handle *mh)
{
#undef CMD
#define CMD     ( cmd->info.readCmd )

ssize_t nbytes, numRead;
BIGGEST_OFF startPos;
UINT32 fileStatusFlags;
int nid, pid, slots[NIOBUFS];
int remaining, bufnum, numReadTotal, transferSize;
int i, rc;
hostReply_t ack;
hostCmd_t   *cmd;

    nid = SRVR_HANDLE_NID(*mh);
    pid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    nbytes = CMD.nbytes;
    startPos = CMD.curPos;
    fileStatusFlags = CMD.fileStatusFlags;
         
    if (DBG_FLAGS(DBG_IO_1)) {
        fprintf(stderr, "host_read(");
        fprintf(stderr, " fh %p,", fh);
        fprintf(stderr, " nbytes %li,", nbytes);
        fprintf(stderr, " startPos %li,", startPos);
        fprintf(stderr, " fileStatusFlags %x,", fileStatusFlags);
        fprintf(stderr, " nid %i,", nid);
        fprintf(stderr, " pid %i,", pid);
    }

    /* adjust the file pointer for the read */
    fh->curPos = positionFilePtr(fh, startPos, nid, pid);

    /* the node's O_NONBLOCK flag doesn't match the current version */
    if ((INT32) fileStatusFlags  != fh->otherFlags) {
        fh->otherFlags = fileStatusFlags;
        fcntl(fh->fd, F_SETFL, fh->otherFlags);
    }

    bufnum = 0;   /* we assume all work buffers are available for use */
    remaining = nbytes;
    numReadTotal = 0;

    ack.your_seq_no = cmd->my_seq_no;

    for (i=0; i<NIOBUFS; i++){
         slots[i] = -1;
    }

    while (1){

        transferSize = ((remaining > IOBUFSIZE) ? IOBUFSIZE : remaining);

        numRead = read(fh->fd, workBufData(bufnum), transferSize);

        if (numRead > 0){

            remaining -= numRead;
            fh->curPos += numRead;
            numReadTotal += numRead;
        }

        if (numRead < 0){   /* error */

            ack.retVal = -1;
            ack.hostErrno = errno;
            ack.info.readAck.curPos = startPos;

            fh->curPos = startPos;

            send_ack_to_app(mh, &ack);
            break;
         
        }
        else if ( (remaining==0) || (numRead < transferSize)){  /* last block */

            ack.retVal = numReadTotal;
            ack.info.readAck.curPos = fh->curPos;

            send_workbuf_and_ack(mh, bufnum, numRead, &ack);
            break;
        }
        else{

            /*
            ** send put request, check later for completion
            */
            slots[bufnum] = send_workbuf(mh, bufnum, numRead);

            if (slots[bufnum] == -1){
                break;
            }
        }

        bufnum++;

        if (bufnum >= NIOBUFS){
            rc = check_workbufs(slots);  /* make sure they can be reused */

            if (rc){
                ack.retVal = -1;
                ack.hostErrno = EIO;
                ack.info.readAck.curPos = startPos;

                fh->curPos = startPos;

                send_ack_to_app(mh, &ack);

                break;  /* some error - app didn't pull buffer */
            }
            else{
                 for (i=0; i<NIOBUFS; i++){
                     slots[i] = -1;
                 }
                 bufnum = 0;
            }
        }
   
    }

    /*
    ** Test that app has pulled all data, and buffers can be
    ** reused, free slots in the data portal.
    */
    check_workbufs(slots);   

    for (i=0; i<NIOBUFS; i++){
         free_work_buf(i);
    }

    if ((numReadTotal != nbytes) && (DBG_FLAGS(DBG_IO_1))) {
        fprintf(stderr, "host_read(): host_read2 failed\n");
    }


    return (numReadTotal);
}
/*
** Make sure that the workbuffers can be reused, that the data
** has been picked up by the app process.
*/
static int
check_workbufs(int *slots)
{
int i, status, rc;
time_t t1;

    status = 0;

    for (i=0; (i<NIOBUFS) && (status==0); i++){

        if (slots[i] != -1){

            t1 = time(NULL);

            while (1){

                /* 
                ** will release data portal slot
                ** if put request has completed
                */

                rc = check_workbuf_to_app(slots[i]);  

                if (rc == 1){
                    break;
                }
                else if (rc == -1) {
                    status = -1; 
                    break;
                }
                else if ((time(NULL) - t1) > daemonWaitLimit){
                    ioErrno = ESENDTIMEOUT;
                    status = -1;
                    break;
                }
            }
        }
    }
    return status;
}
#else
ssize_t
host_read(fileHandle_t *fh, control_msg_handle *mh)
{
#undef CMD
#define CMD     ( cmd->info.readCmd )

ssize_t nbytes, numRead;
BIGGEST_OFF startPos;
UINT32 fileStatusFlags;
int nid, pid;
int remaining, bufnum, numReadTotal, transferSize;
int rc;
hostReply_t ack;
hostCmd_t   *cmd;

    nid = SRVR_HANDLE_NID(*mh);
    pid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    nbytes = CMD.nbytes;
    startPos = CMD.curPos;
    fileStatusFlags = CMD.fileStatusFlags;
         
    if (DBG_FLAGS(DBG_IO_1)) {
        fprintf(stderr, "host_read(");
        fprintf(stderr, " fh %p,", fh);
        fprintf(stderr, " nbytes %li,", nbytes);
        fprintf(stderr, " startPos %li,", startPos);
        fprintf(stderr, " fileStatusFlags %x,", fileStatusFlags);
        fprintf(stderr, " nid %i,", nid);
        fprintf(stderr, " pid %i,", pid);
    }

    /* adjust the file pointer for the read */
    fh->curPos = positionFilePtr(fh, startPos, nid, pid);

    /* the node's O_NONBLOCK flag doesn't match the current version */
    if ((INT32) fileStatusFlags  != fh->otherFlags) {
        fh->otherFlags = fileStatusFlags;
        fcntl(fh->fd, F_SETFL, fh->otherFlags);
    }

    bufnum = get_work_buf();

    if (bufnum < 0){
        ioErrno = ERESOURCE;
	return -1;
    }
    remaining = nbytes;
    numReadTotal = 0;

    ack.your_seq_no = cmd->my_seq_no;

    while (1){

        transferSize = ((remaining > IOBUFSIZE) ? IOBUFSIZE : remaining);

        numRead = read(fh->fd, workBufData(bufnum), transferSize);

        if (numRead > 0){

            remaining -= numRead;
            fh->curPos += numRead;
            numReadTotal += numRead;
        }

        if (numRead < 0){   /* error */

            ack.retVal = -1;
            ack.hostErrno = errno;
            ack.info.readAck.curPos = startPos;

            fh->curPos = startPos;

            rc = send_ack_to_app(mh, &ack);
            break;
         
        }
        else if ( (remaining==0) || (numRead < transferSize)){  /* last block */

            ack.retVal = numReadTotal;
            ack.info.readAck.curPos = fh->curPos;

            rc = send_workbuf_and_ack(mh, bufnum, numRead, &ack);
            break;
        }
        else{

            /*
            ** send put request, check later for completion
            */
            rc = send_workbuf_to_app(mh, bufnum, numRead);

        }

	if (rc < 0){
            fh->curPos -= numRead;
            numReadTotal -= numRead;
	    break;
	}

    }
    free_work_buf(bufnum);

    if ((numReadTotal != nbytes) && (DBG_FLAGS(DBG_IO_1))) {
        fprintf(stderr, "host_read(): host_read2 failed\n");
    }

    return (numReadTotal);
}
#endif

