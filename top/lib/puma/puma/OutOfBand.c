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
** $Id: OutOfBand.c,v 1.1 2001/11/17 01:23:08 lafisk Exp $
**
** Functions to help a Cplant application make use of 
** the server library collective operations.  Server
** library is linked with the app for startup and
** I/O purposes, but it has group formation/communication
** operations as well giving a (rather limited) communication
** capability independent of MPI.
**
** Once the CplantInitCollective call is made by all the
** processes, they can use these calls:
**
**  dsrvr_bcast dsrvr_barrier dsrvr_reduce dsrvr_gather
**
** They all allow sublists of processes, so you can still
** use them after a node in the application dies.
*/
#include <stdlib.h>
#include "puma.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "srvr_err.h"
#include "rpc_msgs.h"
#include "cplant.h"

int
CplantInitCollective(int itmout)
{
int rc, i;

    if (___proc_type != APP_TYPE){
        fprintf(stderr,
         "CplantInitCollective called by non-Cplant application.\n");
        return -1;
    }

    if (dsrvrMembersInited){
        return 0;
    }

    /*
    ** This should only be called once by an application
    ** that wants to use server library collective ops.
    **
    ** All members of the application must call this.
    ** It contains a barrier.
    **
    ** Only apps loaded with "yod2" can make this call since
    ** the processes synchronize through yod2 after setting
    ** up the server library collective portals.
    */

    rc = server_coll_init();   

    if (rc){
        fprintf(stderr,"Failure in server_coll_init (%s)\n",
                       CPstrerror(CPerrno));
        return -1;
    }

    rc = dsrvr_member_init(_my_nnodes, _my_rank, _my_gid);

    if (rc != DSRVR_OK){
        fprintf(stderr,"Failure in dsrvr_member_init(%d,%d,%d) (%s)\n",
                 _my_nnodes, _my_rank, _my_gid, CPstrerror(CPerrno));
        return -1;
    }
 
    for (i=0; i<_my_nnodes; i++){

        if (i == _my_rank) continue;

        rc = dsrvr_add_member((int)_my_nid_map[i], (int)_my_pid_map[i], i);

        if (rc != DSRVR_OK){
            fprintf(stderr,"Failure in dsrvr_add_member(%d,%d,%d) (%s)\n",
                    _my_nid_map[i], _my_pid_map[i], i, CPstrerror(CPerrno));
            return -1;
        }
    }

    /*
    ** The commit function does a barrier operation on the
    ** portal established in server_coll_init().   So we
    ** must synchronize with yod here.  Then we know every process
    ** has completed the call to server_coll_init();
    */
    if (_my_nnodes > 1){
        rc = CplantSynchWithYod(itmout);

        if (rc){
            fprintf(stderr,
             "Timed out in synchronization step in CplantInitCollective\n");
            return -1;
        }
    }

    rc = dsrvr_membership_commit((double)itmout);

    if (rc != DSRVR_OK){
        fprintf(stderr,"Failure in dsrvr_membership_commit (%s)\n",
                  CPstrerror(CPerrno));
        return -1;
    }

    return 0;
}
void
CplantDoneCollective()
{
    dsrvr_member_done();
}
int
CplantSynchWithYod(int itmout)
{
int rc, recvtype, sendmsg[2];
time_t t1;
control_msg_handle mh;

    rc = init_dyn_alloc();

    if (rc == -1){
        return -1;
    }

    sendmsg[0] = _my_dna_ptl;
    sendmsg[1] = _my_ppid;

    rc = srvr_send_to_control_ptl(YOD_NID, YOD_PID, YOD_PTL,
                 INTRA_JOB_BARRIER, (char *)sendmsg, sizeof(sendmsg));

    if (rc){
        return -1;
    }

    /*
    ** Now sit back and wait.  yod2 will send message
    ** when all members of the application have sent in
    ** this command.
    */

    t1 = time(NULL);

    while (1){

        SRVR_CLEAR_HANDLE(mh);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mh,
                        &recvtype, NULL, NULL);

        if (rc == 1) break;

        if (rc == -1){
            return -1;
        }

        if (rc == 0){
             if ( (time(NULL) - t1) > itmout){
                 end_dyn_alloc(); /* so msg will be dropped if it comes in later */
                 return -1;
             }
        }
    }

    srvr_free_control_msg(_my_dna_ptl, &mh);

    if (recvtype != INTRA_JOB_BARRIER){
        return -1;
    }

    return 0;
}
