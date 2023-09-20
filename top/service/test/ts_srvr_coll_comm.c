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
** $Id: ts_srvr_coll_comm.c,v 1.5 2001/04/01 23:52:31 pumatst Exp $
**
** Test forming groups with the membership functions in 
** srvr_coll_membership.c.
**
** Then test the collective communication functions in
** srvr_coll_comm.c and srvr_coll_util.c.  (dsrvr_gather, 
** dsrvr_barrier, dsrvr_bcast, and dsrvr_vote).  Test both
** with the entire group and with subgroups.
**
** Build this as a Cplant server, not as a Cplant application.
**
**   Requires environment variables:
**
**     TOTALNODES         number of processes, one per node
**
**     N0, N1, N2, N3 ...     physical node numbers
**
**   The tests have a fixed ppid value of PPID_TEST, so:
**
**     ->  Run no more than one test on each node.
**
**     ->  Run the test as superuser so you can get the fixed ppid.
**
**   Processes will wait 10 seconds before starting the test, to
**   ensure that all other members have called srvr_comm_init.  
**   You can specify on the command line a different waiting period.
**
** Should test:
**
**      entire group and sub groups
**      disbanding group and setting up a new group
**      failure of a group member and recovery by the others
**      test reseting coll portal structures to recover from error
*/

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "sys_limits.h"
#include "puma.h"
#include "ppid.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "srvr_err.h"

#define EVEN 1
#define ODD  2

#define COLLTIMEOUT 300

int nidMap[MAX_NODES], subNidMap[MAX_NODES];
int ranks[MAX_NODES], subRanks[MAX_NODES];
int ppidMap[MAX_NODES];
int my_rank, my_sub_rank;
int parity;
unsigned int totnodes, subGroupNodes;

int broadcast_test();
int barrier_test();
int membership_test();
int gather_test();

static void
usageError(char *s)
{
    printf("Usage error: %s\n\n",s);
    printf("Options: -v         (verbose mode)\n\n");
    printf("And you must set environment variables:\n");
    printf("TOTALNODES             number of processes, one per node\n");
    printf("N0, N1, N2, N3 ...     the physical node numbers\n");

    exit(-1);
}
#if 0
static void
systemError(char *s)
{
    printf("System error: %s\n",s);
    exit(-1);
}
#endif
static void
coll_lib_error(int rc, char *s)
{

    server_library_done();

    if (rc == DSRVR_ERROR){
        log_error("Coll library general error: %s\n",s);
    }
    else if (rc == DSRVR_RESOURCE_ERROR){
        log_error("Coll library internal error: %s\n",s);
    }
    else if (rc == DSRVR_EXTERNAL_ERROR){
        log_error("Coll library external error: %s - %s\n",s,dsrvr_who_failed());
    }
}

static void
get_options(int argc, char *argv[])
{
int opttype;

    __SrvrDebug = 0;

    while (1){
        opttype = getopt(argc, argv, "w:v");

        if (opttype == EOF) {
            break;
        }

        switch (opttype){

            case 'v':
                __SrvrDebug = 1;   /* display data buffer */
                break;

            default:
                printf("%d/%d ignoring %c\n",
                      _my_pnid, _my_pid, (char)opttype);
                break;
        }
    }
}

main(int argc, char *argv[])
{
char ename[32];
char *c;
int i, rc;

    log_to_stderr(1);
    log_to_file(0);

    get_options(argc, argv);
    /*
    ** get the group info from the environment
    */

    c = getenv("TOTALNODES");

    if (!c){
        usageError("total nodes");
    }
    totnodes = (int)atoi(c);

    if ((totnodes < 2) || (totnodes > MAX_NODES)){
        usageError("set TOTALNODES variable");
    }

    for (i=0; i<totnodes; i++){
        sprintf(ename,"N%d", i);
	c = getenv(ename);

	if (!c){
	    usageError("set node number variables N0, N1, N2, etc.");
	}
	nidMap[i] = (nid_type)atoi(c);
	ranks[i] = i;
	ppidMap[i] = PPID_TEST;

	if ((nidMap[i] < 0) || (nidMap[i] >= MAX_NODES)){
	    usageError("bad node number variables");
	}
    }
    my_rank = -1;

    for (i=0; i<totnodes; i++){
        if (nidMap[i] == _my_pnid){
            my_rank = i;
            break;
        }  
    }

    if (my_rank < 0){
        usageError("My node was not in the node number list");
    }

    /*
    **  Set my portal ID - init the server library
    */
    _my_ppid = register_ppid(&_my_taskInfo, PPID_TEST, GID_TEST, "ts_srvr_coll_comm");

    if (_my_ppid != PPID_TEST){
	log_error("Can not register myself as PPID=%d\n", PPID_TEST);
    }

    if (server_library_init()){
         log_error("Can't init server library");
    }

    /********************************************************
    ** TEST 1
    ** Initialize collective communications of server library
    *********************************************************/

    rc = server_coll_init();

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "server_coll_init");
    }

    /*
    ** Create a group.
    */
    rc = dsrvr_member_init(totnodes, my_rank, GID_TEST);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_member_init");
    }

    for (i=0; i<totnodes; i++){

        rc = dsrvr_add_member(nidMap[i], ppidMap[i], i);

	if (rc != DSRVR_OK){
	     coll_lib_error(rc, "dsrvr_add_member");
	}
    }

    printf("Server library collective communication test: I am %d of %d\n",
                  my_rank, totnodes);

    printf("I'm ready to commit group membership.\n");
    printf("Press enter when all members are ready to go into commit.\n");

    getchar();

    rc = dsrvr_membership_commit(COLLTIMEOUT);

    if (rc != DSRVR_OK){
        coll_lib_error(rc, "dsrvr_membership_commit");
    }

    printf("PASSED!!! - out of membership commit TEST 1\n");

    /********************************************************
    ** TEST 2 and TEST 3
    **   test forming sub groups and reforming original group
    *********************************************************/

    rc = membership_test();

    if (rc){
        server_library_done();
        exit(-1);
    }

    /********************************************************
    ** TEST 4
    ** Do some of barriers
    *********************************************************/

    rc = barrier_test();

    if (rc){
        server_library_done();
        exit(-1);
    }

    /********************************************************
    ** TEST 5
    ** Do a broadcast
    *********************************************************/

    rc = broadcast_test();

    if (rc){
        server_library_done();
	exit(-1);
    }
    printf("PASSED!!! - broadcast test TEST 5\n");

    /********************************************************
    ** TEST 6 
    ** Test gather function and voting (which uses dsrvr_gather)
    *********************************************************/

    rc = gather_test();

    if (rc){
        server_library_done();
	exit(-1);
    }

    /********************************************************
    ** TEST 7 
    ** Test resetting the collective library.  We do this
    ** when there are problems communicating with group
    ** members.  This doesn't effect the group membership
    ** data.
    ********************************************************/

    rc = srvr_reset_coll();

    if (rc != DSRVR_OK){
        coll_lib_error(rc, "srvr_reset_coll");
    }
    printf("I have reset collective library.  I want to re-run some tests.\n");
    printf("Press enter when all members have reset.\n");

    getchar();

    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");
    rc = gather_test();

    if (rc){
        server_library_done();
	exit(-1);
    }

    rc = broadcast_test();

    if (rc != DSRVR_OK){
        server_library_done();
    }
    printf("|++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|\n");

    printf("PASSED!!! - resetting of collective library passed.\n");


    /********************************************************
    ** DONE
    *********************************************************/

    server_library_done();
   
    exit(0);
}
/*******************************************************************/
/*   dsrvr_barrier test                                            */
/*******************************************************************/
int
barrier_test()
{
int rc, i;

    rc = dsrvr_barrier(COLLTIMEOUT, NULL, 0);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_barrier: all nodes (no list specified)");
    }
    printf("Test 4: barrier on all nodes (no list specified) passed\n");

    rc = dsrvr_barrier(COLLTIMEOUT, ranks, totnodes);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_barrier: all nodes (list specified)");
    }
    printf("        barrier on all nodes (list specified) passed\n");

    rc = dsrvr_barrier(COLLTIMEOUT, subRanks, subGroupNodes);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_barrier: sub group");
    }
    printf("        barrier on sub group passed\n");

    /*
    ** Only the even nodes now
    */
    if (parity == EVEN){
        rc = dsrvr_barrier(COLLTIMEOUT, subRanks, subGroupNodes);

        if (rc != DSRVR_OK){
             coll_lib_error(rc, "dsrvr_barrier: sub group even only");
        }
        printf("        barrier on EVEN sub group only passed\n");
    }
    else{
        printf("        I'm ODD - I sit out this barrier\n");
    }

    for (i=0; i<10; i++){
        rc = dsrvr_barrier(COLLTIMEOUT, NULL, 0);

        if (rc != DSRVR_OK){
             coll_lib_error(rc, "dsrvr_barrier: all nodes (no list specified)");
        }
        printf("        barrier %d on all nodes (no list specified) passed\n",i);
    }

    printf("PASSED!!! - barrier test TEST 4\n");

    return 0;
}
/*******************************************************************/
/*   group formation test                                          */
/*******************************************************************/
int
membership_test()
{
int i, rc;

    dsrvr_member_done(); /* can create a new group now */

    /********************************************************
    ** TEST 2
    ** Now form two sub groups - even ranks and odd ranks
    *********************************************************/

    parity = (my_rank % 2) ? ODD : EVEN;

    if (parity == EVEN){
        subGroupNodes = 0;
        for (i=0; i<totnodes; i+= 2){
            subRanks[subGroupNodes] = i;
            subNidMap[subGroupNodes++] = nidMap[i];
        }
    }
    else{
        subGroupNodes = 0;
        for (i=1; i<totnodes; i+= 2){
            subRanks[subGroupNodes] = i;
            subNidMap[subGroupNodes++] = nidMap[i];
        }
    }
    for (i=0; i<subGroupNodes; i++){
        if (subNidMap[i] == _my_pnid){
            my_sub_rank = i;
            break;
        }
    }

    rc = dsrvr_member_init(subGroupNodes, my_sub_rank, GID_TEST);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_member_init sub group");
    }

    for (i=0; i<subGroupNodes; i++){

        rc = dsrvr_add_member(subNidMap[i], ppidMap[i], i);

        if (rc != DSRVR_OK){
             coll_lib_error(rc, "dsrvr_add_member sub group");
        }
    }

    printf("        (sleep 5 seconds for %s to form subgroup)\n",
                    (parity == EVEN) ? "evens" : "odds");

    sleep(5);

    rc = dsrvr_membership_commit(10);

    if (rc != DSRVR_OK){
        coll_lib_error(rc, "dsrvr_membership_commit subgroup");
    }

    printf("PASSED!!! - out of subgroup membership commit - %s group TEST 2\n",
            parity==EVEN ? "even" : "odd");

    dsrvr_member_done(); /* can create a new group now */

    /********************************************************
    ** TEST 3
    ** Rebuild original group
    *********************************************************/

    rc = dsrvr_member_init(totnodes, my_rank, GID_TEST);

    if (rc != DSRVR_OK){
         coll_lib_error(rc, "dsrvr_member_init rebuild original");
    }

    for (i=0; i<totnodes; i++){

        rc = dsrvr_add_member(nidMap[i], ppidMap[i], i);

        if (rc != DSRVR_OK){
             coll_lib_error(rc, "dsrvr_add_member rebuild original");
        }
    }

    printf("        (sleep 5 seconds for all to reform original group)\n");

    sleep(5);

    rc = dsrvr_membership_commit(10);

    printf("PASSED!!! - original group reformed TEST 3\n");

    return 0;
}

/*******************************************************************/
/*   broadcast test                                                */
/*******************************************************************/
#define BSIZE 1024*10
#define BCASTMSGTYPE   12

static int
check_data(char *buf, int len)
{
int i;

    for (i=0; i<len; i++){
	if (buf[i] != i%128){
	    return -1;
	}
    }
    return 0;
}
int
broadcast_test()
{
char *buf;
int i, rc;

    buf = malloc(BSIZE);

    if (!buf){
	printf("broadcast_test: out of memory\n");
	return -1;
    }

    if (my_rank == 0){

	for (i=0; i<BSIZE; i++){
	    buf[i] = i%128;
	}
    }
    printf("Test 5: do some broadcasts\n");

    rc = dsrvr_bcast(buf, BSIZE, COLLTIMEOUT, BCASTMSGTYPE, NULL, 0);

    if (rc){
        coll_lib_error(rc, "dsrvr_bcast - all nodes (no list specifed)");
    }

    if (check_data(buf, BSIZE)){
        printf("FAILURE: dsrvr_bcast, all nodes, no list - bad data\n");
	return -1;
    }

    printf("        broadcast to all nodes, no list specified passed\n");

    if (my_rank > 0){
        memset(buf, 0, BSIZE);
    }

    rc = dsrvr_bcast(buf, BSIZE, COLLTIMEOUT, BCASTMSGTYPE, ranks, totnodes);

    if (rc){
        coll_lib_error(rc, "dsrvr_bcast - all nodes (list specifed)");
    }

    if (check_data(buf, BSIZE)){
        printf("FAILURE: dsrvr_bcast, all nodes, list specified - bad data\n");
	return -1;
    }
    printf("        broadcast to all nodes, list specified passed\n");

    if (my_rank > 1){
        memset(buf, 0, BSIZE);
    }
    if (my_rank == 1){
        memset(buf, 1, BSIZE);
    }
    if (my_rank == 0){
        memset(buf, 2, BSIZE);
    }

    rc = dsrvr_bcast(buf, BSIZE, COLLTIMEOUT, BCASTMSGTYPE, subRanks, subGroupNodes);

    if (rc){
        coll_lib_error(rc, "dsrvr_bcast - all nodes (list specifed)");
    }

    if (parity == EVEN){
        for (i=0; i<BSIZE; i++){
	    if (buf[i] != 2){
                printf( "FAILURE: dsrvr_bcast, even nodes only\n");
		return -1;
	    }
	}
        printf("        broadcast of even nodes passed\n");
    }
    else{
        for (i=0; i<BSIZE; i++){
	    if (buf[i] != 1){
                printf( "FAILURE: dsrvr_bcast, odd nodes only\n");
		return -1;
	    }
	}
        printf("        broadcast of odd nodes passed\n");
    }

    return 0;
}
/*******************************************************************/
/*   dsrvr_gather test                                             */
/*******************************************************************/
int *gather_ints;
long *gather_longs;
char *gather_chars;

#define INTTYPE    12
#define LONGTYPE   13
#define CHARTYPE   14

#define TESTVOTE   15

static void
clear_gather_buffers()
{

    memset((void *)gather_ints, 0, sizeof(int) * totnodes);
    memset((void *)gather_longs, 0, sizeof(long) * totnodes);
    memset((void *)gather_chars, 0,  totnodes);
}

static void
set_gather_buffers()
{
    gather_ints[dsrvrMyGroupRank] = dsrvrMyGroupRank;
    gather_longs[dsrvrMyGroupRank] = dsrvrMyGroupRank * 100000;
    gather_chars[dsrvrMyGroupRank] = dsrvrMyGroupRank % 128;
}

int
gather_test()
{
int rc, i, myvote;

    gather_ints = (int *)calloc(sizeof(int) , totnodes);
    gather_longs = (long *)calloc(sizeof(long) , totnodes);
    gather_chars = (char *)calloc(sizeof(char) , totnodes);

    if (!gather_ints || !gather_longs || !gather_chars){
        perror("gather_test malloc problem");
        return -1;
    }

    set_gather_buffers();

    printf("Test 6: do some gathers\n");

    /****   all nodes participate, no list specified ****/

    rc = dsrvr_gather((char *)gather_ints, sizeof(int), totnodes,
             COLLTIMEOUT,  INTTYPE, NULL, 0);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, no list, ints\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_ints[i] != i){
		printf("        FAILURE: dsrvr_gather, no list, ints\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, ints, no list specified, passed\n");

    rc = dsrvr_gather((char *)gather_longs, sizeof(long), totnodes,
             COLLTIMEOUT,  LONGTYPE, NULL, 0);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, no list, longs\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_longs[i] != i*100000){
		printf("        FAILURE: dsrvr_gather, no list, longs\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, longs, no list specified, passed\n");

    rc = dsrvr_gather((char *)gather_chars, sizeof(char), totnodes,
             COLLTIMEOUT,  CHARTYPE, NULL, 0);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, no list, chars\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_chars[i] != i%128){
		printf("        FAILURE: dsrvr_gather, no list, chars\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, chars, no list specified, passed\n");

    /****   all nodes participate, rank list specified ****/

    clear_gather_buffers();
    set_gather_buffers();

    rc = dsrvr_gather((char *)gather_ints, sizeof(int), totnodes,
             COLLTIMEOUT,  INTTYPE,  ranks, totnodes);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, list of all members specified, ints\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_ints[i] != i){
		printf(
		"        FAILURE: dsrvr_gather, list of all members specified, ints\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, ints, list of all members specified, passed\n");

    rc = dsrvr_gather((char *)gather_longs, sizeof(long), totnodes,
             COLLTIMEOUT,  LONGTYPE, ranks, totnodes);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, list of all members specified, longs\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_longs[i] != i*100000){
		printf(
		"        FAILURE: dsrvr_gather, list of all members specified, longs\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, longs, list of all members specified, passed\n");

    rc = dsrvr_gather((char *)gather_chars, sizeof(char), totnodes,
             COLLTIMEOUT,  CHARTYPE, ranks, totnodes);

    if (rc){
	printf( "        FAILURE: dsrvr_gather, list of all members specified, chars\n");
        coll_lib_error(rc, "dsrvr_gather");
    }

    if (my_rank == 0){
        for (i=0; i<totnodes; i++){
	    if (gather_chars[i] != i%128){
		printf(
		"        FAILURE: dsrvr_gather, list of all members specified, chars\n");
	        return -1;
	    }
	}
    }
    printf("        gather of all nodes, chars, list of all members specified, passed\n");

    /****   even nodes only ****/

    if (parity == EVEN){
        clear_gather_buffers();
	gather_ints[my_sub_rank]  = my_rank;
	gather_longs[my_sub_rank] = my_rank * 100000;
	gather_chars[my_sub_rank] = my_rank % 128;

	rc = dsrvr_gather((char *)gather_ints, sizeof(int), subGroupNodes,
		 COLLTIMEOUT,  INTTYPE,  subRanks, subGroupNodes);

	if (rc){
	    printf( "        FAILURE: dsrvr_gather, even members only, ints\n");
	    coll_lib_error(rc, "dsrvr_gather");
	}

	if (my_rank == 0){
	    for (i=0; i<subGroupNodes; i++){
		if (gather_ints[i] != i*2){
		    printf(
		    "        FAILURE: dsrvr_gather, even members only, ints\n");
		    return -1;
		}
	    }
	}
	printf("        gather, ints, even members only, passed\n");

	rc = dsrvr_gather((char *)gather_longs, sizeof(long), subGroupNodes,
		 COLLTIMEOUT,  LONGTYPE, subRanks, subGroupNodes);

	if (rc){
	    printf( "        FAILURE: dsrvr_gather, even members only, longs\n");
	    coll_lib_error(rc, "dsrvr_gather");
	}

	if (my_rank == 0){
	    for (i=0; i<subGroupNodes; i++){
		if (gather_longs[i] != i*2*100000){
		    printf(
		    "        FAILURE: dsrvr_gather, even members only, longs\n");
		    return -1;
		}
	    }
	}
	printf("        gather longs, even members only, passed\n");

	rc = dsrvr_gather((char *)gather_chars, sizeof(char), subGroupNodes,
		 COLLTIMEOUT,  CHARTYPE, subRanks, subGroupNodes);

	if (rc){
	    printf( "        FAILURE: dsrvr_gather, even members only, chars\n");
	    coll_lib_error(rc, "dsrvr_gather");
	}

	if (my_rank == 0){
	    for (i=0; i<subGroupNodes; i++){
		if (gather_chars[i] != (i*2)%128){
		    printf(
		    "        FAILURE: dsrvr_gather, even members only, chars\n");
		    return -1;
		}
	    }
	}
	printf("        gather, chars, even members only, passed\n");
    }
    else{
        printf("        gather tests for even nodes only, I sit out...\n");
    }

    /*** test vote function, which uses dsrvr_gather ***/

    myvote = my_rank * 55;

    rc = dsrvr_vote(myvote, COLLTIMEOUT, TESTVOTE, NULL, 0);

    if (rc){
	printf( "        FAILURE: dsrvr_vote, no list specified.\n");
        coll_lib_error(rc, "dsrvr_vote");
    }
    for (i=0; i<totnodes; i++){
        if (DSRVR_VOTE_VALUE(i) != i * 55){
	    printf( 
	    "        FAILURE: dsrvr_vote, no list specified, rank %d member vote value.\n",i);
	    return -1;
	}
    }
    printf("        vote, no list specified, passed\n");

    rc = dsrvr_vote(myvote, COLLTIMEOUT, TESTVOTE, ranks, totnodes);

    if (rc){
	printf( "        FAILURE: dsrvr_vote, all nodes, list specified.\n");
        coll_lib_error(rc, "dsrvr_vote");
    }
    for (i=0; i<totnodes; i++){
        if (DSRVR_VOTE_VALUE(i) != i * 55){
	    printf( 
	    "        FAILURE: dsrvr_vote, all nodes, list specified, rank %d member vote value.\n",i);
	    return -1;
	}
    }

    printf("        vote, all nodes, list specified, passed\n");

    rc = dsrvr_vote(myvote, COLLTIMEOUT, TESTVOTE, subRanks, subGroupNodes);

    if (rc){
	printf( "        FAILURE: dsrvr_vote, even/odd sub group.\n");
        coll_lib_error(rc, "dsrvr_vote");
    }
    for (i=0; i<subGroupNodes; i++){
        if (DSRVR_VOTE_VALUE(i) != subRanks[i] * 55){
	    printf( 
	    "        FAILURE: dsrvr_vote, all nodes, list specified, rank %d member vote value.\n",subRanks[i]);
	    return -1;
	}
    }

    printf("        vote, %s nodes only, passed\n",(parity==EVEN)?"even":"odd");

    printf("PASSED!!! - gather/vote test TEST 6\n");

    return 0;
}
