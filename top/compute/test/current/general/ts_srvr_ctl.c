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
** $Id: ts_srvr_ctl.c,v 1.2 2001/07/24 06:58:06 lafisk Exp $
**
** Test of server library (libsrvr.a) control messaging.
**
** All nodes send a message to the rank 0 process.  Rank 0
** fans out a nid map and a pid map.  More tests of all nodes
** sending control messages to rank 0.  Tests of a barrier
** function.
** 
** There are three ways to build/run this:
**
** Build as a parallel Cplant application.  No arguments necessary 
** since the startup code gave everyone a map.  (Build in 
** top/compute/test/current/general.)
**
** Build as a Cplant application and run it as several single node
** applications.  Arguments identify rank 0 application and tell
** the other applications which "rank" they are.  (Build in 
** top/compute/test/current/general.)
**
** Build as a Cplant server and run several of them.  Arguments
** identify "rank" 0 process, etc. (Build in top/service/util/test.)
**
** The second and third cases require command line arguments:
**
** -r {my rank} -s {num nodes}
**
** In addition, if "rank" is not zero, provide the "rank" zero
** identiying information:
**
** -n {nid} -p {pid} 
**
** Default is "I'm rank 0".  The "rank" zero process
** prints it's nid/pid after startup.
*/

#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "puma.h"
#include "sys_limits.h"
#include "srvr_comm.h"
#include "ts_srvr.h"
#include "p30.h"

#define CPLANT_SERVER          1
#define CPLANT_SINGLE_NODE_APP 2
#define CPLANT_APP_GROUP       3

#define MTYPE1   0x0001
#define MTYPE2   0x0002
#define MTYPE3   0x0003
#define MTYPE4   0x0004
#define MTYPE5   0x0005

#define TEST_PORTAL 20
#define BARRIER_PORTAL 22
#define FANOUT_PORTAL 24

#define TIMEOUT     120.00

static void usage(void);
static void rank0(void);
static void otherguy(void);

static int testmode, myrank, size, log2size; 
static int fsrc, fdest1, fdest2;
static char *myname, verbose;
static int rpid, rnid;
extern int SrvrDbg;
static FILE *outf;

nid_type nidList[MAX_NODES];
ppid_type pidList[MAX_NODES];

static int barrier(int type);
static void fanout(char *buf, int len, int type);
static void check_data(char *data);
static void set_data(char *data);

#define exit_error(s) \
    log_error("%d/%d Rank %d error (%s) FAILED\n", _my_pnid, _my_ppid, myrank, s);

static int
get_options(int argc, char *argv[])
{
int opttype, objsize;

    while (1){
        opttype = getopt(argc, argv, "r:n:p:s:hdv");

        if (opttype == EOF) {
            break;
        }

        switch (opttype){

            case 'v':
                verbose = 1;   /* display data buffer */
                break;

            case 'h':
                usage();
                exit(0);

            case 'd':
                SrvrDbg = 1;
                break;

            case 'r':
                myrank = atoi(optarg);

                break;

            case 's':
                size = atoi(optarg);
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
    if (myrank > 0){
        if ( (rpid == SRVR_INVAL_PID) ||
             (rnid == SRVR_INVAL_NID)    ){

            return -1;
        }
    }
    if (size < 2){
        return -1;
    }
    /*
    if (size > 65000){
        return -1;
    }
    */

    objsize = ((sizeof(nid_type) > sizeof(ppid_type)) ? 
                sizeof(nid_type) : sizeof(ppid_type));

    if (size*objsize > SRVR_USR_DATA_LEN){
         fprintf(outf,
	  "Test can't handle more than %d processes without recoding\n",
	      SRVR_USR_DATA_LEN/objsize);
	      /*
	      ** because we send the nid list and pid list in the
	      ** user data portion of a control message
	      */
         return -1;
    }
    return 0;
}

main(int argc, char *argv[])
{
int rc;

    myname = argv[0];
    size = 2;
    SrvrDbg = 0;
    rpid = SRVR_INVAL_PID;  
    rnid = SRVR_INVAL_NID;
    myrank = 0;
    verbose = 0;
    outf = stdout;

    INIT_SRVRLIB_TEST;

    if (_my_nnodes > 1){

        myrank = _my_rank;
        size = _my_nnodes;

        if (_my_rank > 0){
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

    for (log2size = 15; log2size > 0; log2size--){
        if (size & (1 << log2size)){
            break;
        }
    }

    fdest1 = (2*myrank) + 1;
    fdest2 = (2*myrank) + 2;

    if (myrank == 0){
        fsrc = -1;
        rank0();
    }
    else{ 
        fsrc = (myrank-1)/2;
        otherguy(); 
    }
    
    return 0;
}
/*********************************************************************/
/*   senders - pummel that control portal                            */
/*********************************************************************/

char udata[SRVR_USR_DATA_LEN];

static void
otherguy()
{
int  rc, lists, t1, i;
control_msg_handle mhandle;

    rc = srvr_init_control_ptl_at(2*log2size, BARRIER_PORTAL);

    if (rc){
        exit_error("srvr_init_control_ptl_at BARRIER_PORTAL");
    }

    rc = srvr_init_control_ptl_at(2, FANOUT_PORTAL);

    if (rc){
        exit_error("srvr_init_control_ptl_at FANOUT_PORTAL");
    }

    if (testmode == CPLANT_APP_GROUP){

        rc = PtlNIBarrier(__myr_ni_handle);
        if (rc){
            CPerrno = rc;
            exit_error("PtlNIBarrier");
        }
    }
    /*
    **    test 1
    */

   rc = srvr_send_to_control_ptl(rnid, rpid, TEST_PORTAL, MTYPE1,
                     (char *)&myrank, sizeof(int));

    if (rc){
        exit_error("srvr_send_to_control_ptl");
    }
    fprintf(outf,"%d/%d Rank %d: test 1 PASSED\n", _my_pnid, _my_ppid, myrank);

    /*
    **    get nid and pid lists
    */
    t1 = time(NULL);
    lists=0;

    while (1){

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(FANOUT_PORTAL, &mhandle, NULL, NULL, NULL);

        if (rc == -1){
            exit_error("srvr_get_next_control_msg");
        }

        if (rc == 0){
	    if (time(NULL) - t1 > TIMEOUT){
		exit_error("timeout waiting for nid pid lists");
	    }
	    else{
		continue;
	    }
        }

        if (SRVR_HANDLE_TYPE(mhandle) == MTYPE3){
            memcpy((char *)nidList, SRVR_HANDLE_USERDEF(mhandle),
                           sizeof(nid_type) * size);
        }
        else if (SRVR_HANDLE_TYPE(mhandle) == MTYPE4){
            memcpy((char *)pidList, SRVR_HANDLE_USERDEF(mhandle),
                           sizeof(ppid_type) * size);
        }
        else{
            exit_error("srvr_get_next_control_msg - message type");
        }
        lists++;

        srvr_free_control_msg(FANOUT_PORTAL, &mhandle);

        if (lists == 2) break;
    }

    /*
    **    test 2
    */
    for (i=0; i<25; i++){

	rc = srvr_send_to_control_ptl(rnid, rpid, TEST_PORTAL, MTYPE2,
				       NULL, 0);

	if (rc){
	    exit_error("srvr_send_to_control_ptl");
	}
	
	barrier(MTYPE3);
    }
    fprintf(outf,"%d/%d Rank %d: test 2 PASSED\n", _my_pnid, _my_ppid, myrank);

    /*
    **    test 3
    */

    for (i=0; i<25; i++){
	rc = srvr_send_to_control_ptl(rnid, rpid, TEST_PORTAL, MTYPE4,
					NULL, 0); 

	if (rc){
	    exit_error("srvr_send_to_control_ptl");
	}

	barrier(MTYPE5);
    }
    fprintf(outf,"%d/%d Rank %d: test 3 PASSED\n", _my_pnid, _my_ppid, myrank);
    /*
    **    test 4 - try a fanout
    */

    fanout(udata, SRVR_USR_DATA_LEN, MTYPE1);

    check_data(udata);

    fprintf(outf,"%d/%d Rank %d: done PASSED\n", _my_pnid, _my_ppid, myrank);

}
/*********************************************************************/
/*  "rank" 0 - receives control messages                             */
/*********************************************************************/
static void
rank0()
{
int rc, msg_type, i, xferlen, ii;
int ctl_ptl, nmsgs, nothers, srcrank;
control_msg_handle mhandle;
double tstart;
char *testdata;

    rc = srvr_init_control_ptl_at(2*log2size, BARRIER_PORTAL);

    if (rc){
        exit_error("srvr_init_control_ptl_at BARRIER");
    }

    rc = srvr_init_control_ptl_at(2, FANOUT_PORTAL);

    if (rc){
        exit_error("srvr_init_control_ptl_at FANOUT");
    }

    rc = srvr_init_control_ptl_at(size, TEST_PORTAL);

    if (rc){
        exit_error("srvr_init_control_ptl_at TEST");
    }
    else{
        ctl_ptl = TEST_PORTAL;
    }

    if (testmode != CPLANT_APP_GROUP){

        fprintf(outf,"RANK 0: nid/pid %d / %d \n",
                _my_pnid, _my_ppid);

        if (size==2){
                fprintf(outf,
                "Start other with: %s %s -s %d -n %d -p %d -r 1\n",
                (testmode==CPLANT_SINGLE_NODE_APP) ? "yod" : "",
                  myname, size, _my_pnid, _my_ppid);
        }
        else{
                fprintf(outf, "Start others with:\n");
                for (i=1; i<size; i++){
                    fprintf(outf, "\t%s %s -s %d -n %d -p %d -r %d\n",
                (testmode==CPLANT_SINGLE_NODE_APP) ? "yod" : "",
                            myname, size, _my_pnid, _my_ppid,i);
                }
        }
    }
    else{
        /*
        ** Cplant parallel apps must sychronize here to be sure
        ** rank0 guy has set up test portal.
        */
        rc = PtlNIBarrier(__myr_ni_handle);
        
        if (rc){
            CPerrno = rc;
            exit_error("PtlNIBarrier");
        }
    }
    /*
    ** Wait for all control messages to arrive.  After all have
    ** arrived, free them all.
    **
    ** Gather a nid/pid map while you are at it, so we can fan
    ** it out.
    */

    nmsgs = 0;
    tstart = time(NULL);
    nothers = size - 1;

    nidList[0] = _my_pnid;
    pidList[0] = _my_ppid;

    printf("Test 1:\n");

    while (nmsgs < nothers){

        if (time(NULL) - tstart > TIMEOUT){
            exit_error("timeout waiting for initial messages");
        }

        SRVR_CLEAR_HANDLE(mhandle);
	xferlen = -1;

        rc = srvr_get_next_control_msg(ctl_ptl, &mhandle, &msg_type,
                                              &xferlen, &testdata);

        if (rc == -1){
            exit_error("srvr_get_next_control_msg");
        }

        if (rc == 0) continue;   /* nothing yet */

        /*
        ** GOT ONE
        */

        printf("\tFrom node %d\n",SRVR_HANDLE_NID(mhandle));

	srcrank = *(int *)SRVR_HANDLE_USERDEF(mhandle);

	if ( (srcrank < 1) || (srcrank >= size)){
	    exit_error("bad control message rank");
	}
	if (xferlen != 0){
	    exit_error("srvr_get_next_control_msg: xferlen");
	}

	if (srcrank != *(int *)testdata){
	    exit_error("srvr_get_next_control_msg: user data field");
	}

	nidList[srcrank] = SRVR_HANDLE_NID(mhandle);
	pidList[srcrank] = SRVR_HANDLE_PID(mhandle);

        nmsgs++;
    }
    rc = srvr_free_all_control_msgs(TEST_PORTAL);

    if (rc){
        exit_error("srvr_free_all_control_msgs");
    }

    /*
    ** send nids and pids
    */
    for (i=1; i<size; i++){
        rc = srvr_send_to_control_ptl(nidList[i], pidList[i],
	                FANOUT_PORTAL, MTYPE3,
			(char *)nidList, sizeof(nid_type)*size);
        if (rc){
	    exit_error("srvr_send_to_control_ptl nidList");
	}
        rc = srvr_send_to_control_ptl(nidList[i], pidList[i],
	                FANOUT_PORTAL, MTYPE4,
			(char *)pidList, sizeof(ppid_type)*size);
        if (rc){
	    exit_error("srvr_send_to_control_ptl pidList");
	}
    }

    /*
    ** test 2
    **
    ** Wait for control messages to arrive.  Free each one
    ** before going to get the next.
    */
    printf("Test 2:\n");

    for (ii=0; ii<25; ii++){

	nmsgs = 0;
	tstart = time(NULL);


	while (nmsgs < nothers){

	    if (time(NULL) - tstart > TIMEOUT){
		exit_error("timeout waiting for test 2 messages");
	    }

	    SRVR_CLEAR_HANDLE(mhandle);

	    rc = srvr_get_next_control_msg(ctl_ptl, &mhandle, &msg_type,
					     NULL, NULL);

	    if (rc == -1){
		exit_error("srvr_get_next_control_msg - test 2");
	    }

	    if (rc == 0) continue;   /* nothing yet */

	    /*
	    ** GOT ONE
	    */

	    printf("\tFrom node %d\n",SRVR_HANDLE_NID(mhandle));

	    if (msg_type != MTYPE2){
		exit_error("bad message type in test 2");
	    }
	    rc = srvr_free_control_msg(ctl_ptl, &mhandle);

	    if (rc){
		 exit_error("srvr_free_control_msg in test 2");
	    }

	    nmsgs++;
	}
        barrier(MTYPE3);
	printf("\n");
    }

    /*
    ** test 3
    **
    ** Use matching criteria to find control messages.  Free
    ** them as they are found.
    */
    printf("Test 3:\n");

    for (ii=0; ii<25; ii++){

	for (i=1; i<size; i++){

	    SRVR_CLEAR_HANDLE(mhandle);
	    SRVR_HANDLE_NID(mhandle) = nidList[i];
	    SRVR_HANDLE_PID(mhandle) = pidList[i];
	    tstart = time(NULL);

	    while (1){

		rc = srvr_get_next_control_msg(ctl_ptl, &mhandle, &msg_type,
					     NULL, NULL);

		if (rc == -1){
		    exit_error("srvr_get_next_control_msg");
		}

		if (rc == 0){
		    if (time(NULL) - tstart > TIMEOUT){
			exit_error("timeout waiting for test 3 messages");
		    }
		    else{
			continue;
		    }
		}

		/*
		** GOT IT
		*/
		printf("\tFrom node %d\n",SRVR_HANDLE_NID(mhandle));

		if (msg_type != MTYPE4){
		    exit_error("bad message type in test 3");
		}
		if (SRVR_HANDLE_NID(mhandle) != nidList[i]){
		    exit_error("bad matching");
		}
		rc = srvr_free_control_msg(ctl_ptl, &mhandle);

		if (rc){
		    exit_error("srvr_free_control_msg in test 3");
		}
		break;
	    }
	}
	barrier(MTYPE5);
	printf("\n");
    }

    /*
    **    test 4 - try a fanout
    */
    printf("Test 4:\n");

    set_data(udata);

    fanout(udata, SRVR_USR_DATA_LEN, MTYPE1);

    /*
    ** release control portal when done receiving requests
    */
    rc = srvr_release_control_ptl(ctl_ptl);

    if (rc == -1){
        exit_error("srvr_release_control_ptl TEST");
    }
    rc = srvr_release_control_ptl(BARRIER_PORTAL);

    if (rc == -1){
        exit_error("srvr_release_control_ptl BARRIER");
    }
    rc = srvr_release_control_ptl(FANOUT_PORTAL);

    if (rc == -1){
        exit_error("srvr_release_control_ptl FANOUT");
    }

    fprintf(outf,"%d/%d Rank 0: done PASSED\n", _my_pnid, _my_ppid);
}
/*********************************************************************/
/*
** fan out a buffer that fits in control message
**
**               AND
**
** barrier for processes not part of a Cplant parallel app
**    (the other guys can use PtlNIBarrier)
*/

#define IN  1
#define OUT 2

static void
fanout(char *buf, int len, int type)
{
time_t t1;
control_msg_handle mhandle;
int mtype, rc;

   if (len > SRVR_USR_DATA_LEN){
       exit_error("fanout - test can't fanout big buffers without recoding");
   }

   if (fsrc >= 0){

       t1 = time(NULL);

       SRVR_CLEAR_HANDLE(mhandle);

       while (1){
            rc = srvr_get_next_control_msg(FANOUT_PORTAL, &mhandle,
                                        &mtype, NULL, NULL);

            if (rc == 1){
		break;
            }
            else if (rc < 0){
                exit_error("fanout - srvr_get_next_control_msg");
            }
            else if ((time(NULL) - t1) > TIMEOUT){
                exit_error("fanout - srvr_get_next_control_msg TIMEOUT");
            }
       }

       if (SRVR_HANDLE_TYPE(mhandle) !=type){
	    exit_error("fanout - incoming message type");
       }

       memcpy(buf, SRVR_HANDLE_USERDEF(mhandle), len);
       srvr_free_control_msg(FANOUT_PORTAL, &mhandle);
   }

   if (fdest1 < size){
        rc = srvr_send_to_control_ptl(nidList[fdest1], pidList[fdest1], 
	               FANOUT_PORTAL, type, buf, len);

        if (rc){
            exit_error("fanout - srvr_send_to_control_ptl 1");
        }
   }
   if (fdest2 < size){
        rc = srvr_send_to_control_ptl(nidList[fdest2], pidList[fdest2], 
	               FANOUT_PORTAL, type, buf, len);

        if (rc){
            exit_error("fanout - srvr_send_to_control_ptl 2");
        }
   }
}
static void
barrier_step(int me, int it, int type, int direction)
{
control_msg_handle mhandle;
int from, to, mtype, rc;
time_t t1;

    if (   ((direction == OUT) && (me > it)) ||
           ((direction == IN) && (me < it))     ){

	    to = me;
	    from = it;
    }
    else{
	    to = it;
	    from = me;
    }

    if (from == me){

        rc = srvr_send_to_control_ptl(nidList[it], pidList[it], BARRIER_PORTAL, 
                                       type, NULL, 0);

        if (rc){
            exit_error("barrier_in - srvr_send_to_control_ptl");
        }
    }
    else{
        t1 = time(NULL);

        SRVR_CLEAR_HANDLE(mhandle);

        SRVR_HANDLE_NID(mhandle) = nidList[it];  /* match criteria */
        SRVR_HANDLE_PID(mhandle) = pidList[it];
        SRVR_HANDLE_TYPE(mhandle) = type;

        while (1){

            rc = srvr_get_next_control_msg(BARRIER_PORTAL, &mhandle,
                                        &mtype, NULL, NULL);

            if (rc == 1){
                srvr_free_control_msg(BARRIER_PORTAL, &mhandle);
		break;
            }
            else if (rc < 0){
                exit_error("barrier_in - srvr_get_next_control_msg");
            }
            else if ((time(NULL) - t1) > TIMEOUT){
                exit_error("barrier_in - srvr_get_next_control_msg TIMEOUT");
            }
        }
    }
}
static int
barrier(int type)
{
int i, other;


    for (i = 0; i <= log2size; i++){

        other = myrank ^ (1<<i);

        if (other < size) barrier_step(myrank, other, type, OUT);
        
    }
    for (i = log2size; i >= 0 ; i--){

        other = myrank ^ (1<<i);

        if (other < size) barrier_step(myrank, other, type, IN);
        
    }

    return 0;
}
/*********************************************************************/
/*********************************************************************/
static void
usage()
{
printf("In this test, all processes send control messages to the rank 0 process.\n");
printf("\n");
printf("If running as a parallel application launched with yod, no command\n");
printf("line arguments are required.  The processes have a load map so they\n");
printf("know who they all are.\n");
printf("\n");
printf("If running as several single node Cplant applications, or if running\n");
printf("as standalone server processes, then these command line arguments are\n");
printf("required:\n");
printf("\n");
printf("All processes need to know their rank in the test (0 based) and the\n");
printf("total number of processes in the test:\n");
printf("\n");
printf("  -r {my rank} -s {num nodes}\n");
printf("\n");
printf("In addition, if \"rank\" is not zero, provide the \"rank\" zero\n");
printf("identifying information (the rank 0 process prints it for you):\n");
printf("\n");
printf("  -n {nid} -p {pid}\n");
printf("\n");
printf("Default is \"I'm rank 0 and there are two processes.\"\n");
}
static void
set_data(char *data)
{
int i;

    for (i=0; i<SRVR_USR_DATA_LEN; i++){
        data[i] = i%128;
    }
}
static void
check_data(char *data)
{
int i;

    for (i=0; i<SRVR_USR_DATA_LEN; i++){
        if (data[i] != i%128){
             exit_error("fanout data corrupt");
        }
    }
}
