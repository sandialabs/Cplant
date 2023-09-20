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
** $Id: ts_srvr_put.c,v 1.5 2001/07/24 06:59:54 lafisk Exp $
**
** Two node test of server library (libsrvr.a) PUT function.  Also
** tests control portals, since they carry the put request.
**
** Sender sends put request to receiver.  Receiver sends put reply 
** to pick up the data.   There are three ways to build/run this:
**
** Build as a Cplant application and run two single node jobs, one
** sender and one receiver. (Build in top/compute/test/current/general.)
**
** Build as a Cplant application and run it as a two node application.
** They'll figure out who is sender, who is receiver, and run the test.
**
** Build as a Cplant server and run two instances, one sender and
** one receiver.  (Build in top/service/util/test.)
**
** The first and third case require command line arguments:
**
**   -s    if process is sender
**   -r    if process is receiver
**
**   In addition, sender needs nid/pid of receiver
**           -nid {nid} -pid {pid} 
**
**   Default is "process is receiver".
**
**   Receiver prints it's nid/pid after startup.
*/

#include<getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ts_srvr.h"

#define CPLANT_SERVER          1
#define CPLANT_SINGLE_NODE_APP 2
#define CPLANT_APP_GROUP       3

#define NTESTS 4

#define SENDER   1
#define RECEIVER 2

#define REQUEST_PORTAL 20

#define TIMEOUT     120.00

typedef struct _testdef{
   int msglen;
   int blocklen;
   int status;
}testdef;

testdef putTest[NTESTS]={
{ (2*1024), (2*1024),  0},
{ (2*1024), (512),  0},
{ (2*1024), (500),  0},
{ (1000), (400),  0}
};

static void usage(void);
static void receiver(void);
static void sender(void);
static void process_control_message(control_msg_handle *mh, int msg_type,
                                     int xfer_len);
static void set_data(char *buf, int len);
static int check_data(char *buf, int len, int i1);

static int iam, testmode; 
static char *myname, verbose;
static int rpid, rnid, rptl;
extern int SrvrDbg;
static FILE *outf;

#define receiver_error(s) \
    log_error("%d/%d RECEIVER error (%s) FAILED\n", _my_pnid, _my_ppid, s);

#define sender_error(s) \
    log_error("%d/%d SENDER error (%s) FAILED\n", _my_pnid, _my_ppid, s);

static int
n_blocks(int bufsize, int blocksize)
{
int ac;

    ac = (bufsize / blocksize);
    if ( (ac*blocksize) < bufsize) ac++;

    return ac;
}
static int
get_options(int argc, char *argv[])
{
int opttype;

    while (1){
        opttype = getopt(argc, argv, "hsrdvp:n:");

        if (opttype == EOF) {
            break;
        }

        switch (opttype){

            case 'h':
                usage();
		exit(0);

            case 'v':
                verbose = 1;   /* display data buffer */
                break;

            case 'd':
                SrvrDbg = 1;
                break;

            case 's':
                iam = SENDER;
                break;

            case 'r':
                iam = RECEIVER;
                break;

            case 'p':
                rpid = atoi(optarg);
                break;

            case 'n':
                rnid = atoi(optarg);
                break;

            default:
                fprintf(outf,"%d/%d ignoring %c\n",
                      _my_pnid, _my_pid, (char)opttype);
                break;
        }
    }
    if (iam == SENDER){
        if ( (rpid == SRVR_INVAL_PID) ||
             (rnid == SRVR_INVAL_NID) ||
             (rptl == SRVR_INVAL_PTL)    ){

            return -1;
        }
    }
    return 0;
}

main(int argc, char *argv[])
{
int rc;

    myname = argv[0];
    iam = RECEIVER;
    SrvrDbg = 0;
    rpid = SRVR_INVAL_PID;  
    rnid = SRVR_INVAL_NID;
    rptl = REQUEST_PORTAL;
    verbose = 0;
    outf = stdout;

    INIT_SRVRLIB_TEST;

    if (_my_rank >= 2){
        log_quit("Rank %d: This is a two node test.  I'm exiting.\n",_my_rank);
    }
    if (_my_nnodes > 1){
        if (_my_rank == 0){
            iam = RECEIVER;
        }
        else{
            iam = SENDER;
            rnid = _my_nid_map[0];
            rpid = _my_pid_map[0];

            outf = fopen("/dev/null", "w");
        }
        testmode = CPLANT_APP_GROUP;
    }
    else{
        rc = get_options(argc, argv);

        if (rc){
            usage();
            exit(0);
        }
        if (___proc_type == SERV_TYPE){
            testmode = CPLANT_SERVER;
        }
        else{
            testmode = CPLANT_SINGLE_NODE_APP;
        }
    }
    if (iam == SENDER){
        sender();
    }
    else if (iam == RECEIVER){
        receiver(); 
    }
    
    return 0;
}
/*********************************************************************/
/*   send side: we request remote process to pull data from us       */
/*********************************************************************/
static char myUserData[SRVR_USR_DATA_LEN];

static void
sender(void)
{
char *databuf, done;
int buflen, put_handle[NTESTS];
int tst, accesses[NTESTS];
time_t t1;

    memset(myUserData,0,SRVR_USR_DATA_LEN);

    /*
    ** Allocate a buffer for data to put to remote process
    */
    for (tst=0,buflen=0; tst<NTESTS; tst++){
        if (putTest[tst].msglen > buflen) buflen = putTest[tst].msglen;
    }

    databuf = (char *)malloc(buflen);

    if (!databuf){
        sender_error("malloc data buf");
    }
    set_data(databuf, buflen);

    if (_my_nnodes > 1){
        fprintf(stdout,"(sender waiting to ensure receiver has started)\n");
        sleep(10);   /* wait for receiver to set up control portal */
    }

    for (tst=0; tst<NTESTS; tst++){

        put_handle[tst] = srvr_comm_put_req(
                         databuf, putTest[tst].msglen, tst, 
                         myUserData, SRVR_USR_DATA_LEN,
                           1, &rnid, &rpid, &rptl);

        if (put_handle[tst] < 0){
            sender_error("srvr_comm_put_req");
        }

        accesses[tst] = n_blocks(putTest[tst].msglen, putTest[tst].blocklen);

        fprintf(outf,"SENDER: test %d:  size %d, blocksz %d, %d transfers\n",
            tst, putTest[tst].msglen, putTest[tst].blocklen,accesses[tst]);
    }

    done = 0;

    t1 = time(NULL);

    while (done < NTESTS){

        if (time(NULL) - t1 > 300){
            fprintf(outf,"SENDER: timed out waiting for put replies\n");
            break;
        }

        for (tst=0; tst<NTESTS; tst++){
      
            if (putTest[tst].status) continue;

            putTest[tst].status = 
              srvr_test_read_buf(put_handle[tst], accesses[tst]);

            if (putTest[tst].status){   /* got it or error occured */
                 done++;
            }
        }
    }
    for (tst=0; tst<NTESTS; tst++){
        if (putTest[tst].status == 1){
            fprintf(outf,"%d/%d SENDER passed test %d\n",_my_pnid,_my_ppid,tst);
        }
        else{
            fprintf(outf,"%d/%d SENDER failed test %d\n",_my_pnid,_my_ppid,tst);
            sender_error("srvr_test_read_buf");
        }
        srvr_delete_buf(put_handle[tst]);
    }
    
    fprintf(outf,"%d/%d SENDER: done PASSED\n", _my_pnid, _my_ppid);

}
/*********************************************************************/
/*********************************************************************/
static void
receiver(void)
{
int rc, xfer_len, msg_type;
int ctl_ptl, nmsgs;
control_msg_handle mhandle;
double tstart;

    rc = srvr_init_control_ptl_at(NTESTS, REQUEST_PORTAL);

    if (rc){
        receiver_error("srvr_init_control_ptl_at");
    }
    else{
        ctl_ptl = REQUEST_PORTAL;
    }

    if (_my_nnodes == 1){
        fprintf(outf,"RECEIVER: nid/pid/portal %d / %d / %d\n",
                _my_pnid, _my_ppid, ctl_ptl);

        if (testmode == CPLANT_SINGLE_NODE_APP){
            fprintf(outf,"Start sender with: yod -sz 1 %s -s -n %d -p %d\n",
              myname, _my_pnid, _my_ppid);
        }
        else{
            fprintf(outf,"Start sender with: %s -s -n %d -p %d\n",
              myname, _my_pnid, _my_ppid );
        }
    }

    /*
    ** Wait around for requests
    */
    nmsgs = 0;
    tstart = time(NULL);

    while (nmsgs < NTESTS){

        if (time(NULL) - tstart > TIMEOUT){
            receiver_error("timeout waiting for put requests");
        }

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(ctl_ptl, &mhandle, &msg_type,
                                   &xfer_len, NULL);

        if (rc == -1){
            receiver_error("srvr_get_next_control_msg");
        }

        if (rc == 0) continue;   /* nothing yet */

        /*
        ** GOT ONE
        ** go retrieve data
        */
        nmsgs++;

        process_control_message(&mhandle, msg_type, xfer_len);

        /*
        ** Free control message slot
        */
        rc = srvr_free_control_msg(ctl_ptl, &mhandle);

        if (rc == -1){
            receiver_error("srvr_free_control_msg");
        }
    }

    /*
    ** release control portal when done receiving requests
    */
    rc = srvr_release_control_ptl(ctl_ptl);

    if (rc == -1){
        receiver_error("srvr_release_control_ptl");
    }

    fprintf(outf,"%d/%d RECEIVER: done PASSED\n", _my_pnid, _my_ppid);
}
static void
process_control_message(control_msg_handle *mh, int msg_type, int xfer_len)
{ 
char *c, *databuf;
int rc, nbytes, remaining, blocks, blocksz, i;
testdef *td;

    if ((msg_type < 0) || (msg_type >= NTESTS)){
        receiver_error("got a bad message type");
    }

    td = putTest + msg_type;
    nbytes = td->msglen;
    blocksz = td->blocklen;

    blocks = n_blocks(nbytes, blocksz);

    fprintf(outf,"RECEIVER: test %d:  recv PUT of size %d, blocksz %d, %d transfers\n",
            msg_type, nbytes, blocksz, blocks);

    if (verbose){
        srvr_display_data_xfer_msg(&(mh->msg->msg));
    }

    /*
    ** prepare buffer to receive data
    */
    databuf = (char *)calloc(1, nbytes);
    if (!databuf){
        receiver_error("calloc");
    } 

    /*
    ** go get the data
    */
    if (blocks == 1){
        rc = srvr_comm_put_reply(mh, databuf, nbytes);

        if (rc == -1){
            receiver_error("srvr_comm_put_reply");
        }
        rc = check_data(databuf, nbytes, 0);

        if (rc){
            receiver_error("check_data");
        }
    }
    else{
        for (i=0, c=databuf, remaining=nbytes; 
             i<blocks; 
             i++, c+=blocksz, remaining-=blocksz){

            xfer_len = ((blocksz <= remaining) ? blocksz : remaining);

            rc = srvr_comm_put_reply_partial(mh, c, xfer_len, c-databuf);

            if (rc == -1){
                receiver_error("srvr_comm_put_reply_partial");
            }
            rc = check_data(c, xfer_len, (c-databuf)%128);

            if (rc){
                receiver_error("check_data");
            }

        }
    }

    free(databuf);

    fprintf(outf,"%d/%d RECEIVER passed test %d\n",_my_pnid,_my_ppid,msg_type);
}
/*********************************************************************/
/*********************************************************************/
/*********************************************************************/
static void
set_data(char *buf, int len)
{
int i;
    for (i=0; i<len; i++){
        buf[i] = (i % 128);
    }

    if (verbose){
        fprintf(outf,"SET DATA:\n\t");
        for (i=0; i<len; i++){
            fprintf(outf,"%02x ",(int)buf[i]);
            if (((i+1)%16) == 0) fprintf(outf,"\n\t");
        }
        fprintf(outf,"\n");
    }

}
static int
check_data(char *buf, int len, int i1)
{
int i, status, val;

    status = 0;

    if (verbose){

        fprintf(outf,"CHECK DATA\n\t");

        for (i=0, val=i1; i<len; i++, val++){

            fprintf(outf,"%02x ",(int)buf[i]);

            if (((i+1)%16) == 0) fprintf(outf,"\n\t");

            if (*(buf+i) != (val % 128)){

                fprintf(outf,"<<<< ERROR\n");
                status = -1;
                break;
            }
        }
        fprintf(outf,"\n");
    }
    else{
        for (i=0, val=i1; i<len; i++, val++){
            if (buf[i] != (val%128)){
                status = -1;
                break;
            }
        }
    }

    return status;
}
static void
usage(void)
{
fprintf(outf,"**   -s   {means this is a sender}\n");
fprintf(outf,"**   -r   {means this is a receiver}\n");
fprintf(outf,"**\n");
fprintf(outf,"**   In addition, sender needs nid/pid/ptl of receiver\n");
fprintf(outf,"**         -n {nid} -p {pid}\n");
fprintf(outf,"**   -verbose will print out the data PUT to remote process\n");
fprintf(outf,"**\n");
}
