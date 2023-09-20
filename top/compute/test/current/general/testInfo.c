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
** $Id: testInfo.c,v 1.3 2001/11/25 00:38:36 lafisk Exp $
**  test the new information functions
**
**     Optional argument: The Cplant job ID for a job to query about
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "puma.h"
#include "srvr/srvr_err.h"
#include "config/cplant.h"

extern int get_bebopd_id(int *bnid, int *bpid, int *bptl, int newLookUp);

static int bebopdnid, bebopdpid, bebopdptl;
static int jobID=-1;

static time_t t1, elapsed;

static int *nid_map;
static int *ppid_map;

main(int argc, char *argv[])
{
int i, rc, sz;
nid_type *nids;
ppid_type *pids;

    log_to_file(0);
    log_to_stderr(1);

    if (argc > 1){
        jobID = atoi(argv[1]);
    }

    rc = get_bebopd_id(&bebopdnid, &bebopdpid, &bebopdptl, 1);

    if (rc == 0){

    printf("The bebopd nid/pid/ptl is %d/%d/%d\n",
	      bebopdnid, bebopdpid, bebopdptl);
    }
    else{
	log_error("get_bebopd_id");
    }

    printf("MY INFO:\n--------\n");

    printf("My node ID: %d\n",CplantMyNid());
    printf("My portal process ID: %d\n",CplantMyPPid());
    printf("My rank: %d\n",CplantMyRank());
    printf("My system process ID: %d\n",CplantMyPid());
    printf("My application size: %d\n",(sz = CplantMySize()));
    printf("My Cplant job ID: %d\n",CplantMyJobId());

    nids = CplantMyNidMap();
    pids = CplantMyPidMap();

    printf("My application nid/pid map:\n");

    for (i=0; i<sz; i++){
        printf("%d/%d\n",nids[i],pids[i]);
    }
    printf("\n");

    printf("My PBS job ID: %d\n",CplantMyPBSid());

    if (jobID == -1){
        exit(0);
    }

    printf("INFO on Cplant job %d:\n--------------------\n",jobID);

    t1 = time(NULL);

    sz = CplantJobSize(jobID);

    if ( elapsed = (time(NULL) - t1)){
        printf("Job size request took %ld seconds\n",elapsed);
    }
    printf("Job size is %d\n",sz);

    rc = CplantJobNidMap(jobID, &nid_map);

    if (rc > 0){

        printf("Node ID map:\n");
        for (i=0; i < sz; i++){
	    printf("%d\n",nid_map[i]);
	}
        printf("\n");
    }
    else{
        printf("CplantJobNidMap returned %d\n",rc);
    }

    rc = CplantJobPidMap(jobID, &ppid_map);

    if (rc > 0){

        printf("Portal ID map:\n");
        for (i=0; i < sz; i++){
	    printf("%d\n",ppid_map[i]);
	}
        printf("\n");
    }
    else{
        printf("CplantJobPidMap returned %d\n",rc);
    }
}
