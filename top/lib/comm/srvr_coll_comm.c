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
** $Id: srvr_coll_comm.c,v 1.21 2001/12/17 22:10:01 rbbrigh Exp $
**
** Fault recovering collective operations for temporary groups formed
** by srvr_coll_membership.c functions.
*/

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <srvr_lib.h>
#include <srvr_coll.h>
#include <sys_limits.h>

typedef struct _blockInfo{
    int offset;
    int len;
} blockInfo;

typedef struct _opInfo{
    int       eqsize;
    int       num_md;
    ptl_md_t  mddef;
    ptl_handle_me_t  me;
    ptl_handle_md_t  *md;
} opInfo;

static opInfo opTypes[NUM_OP_TYPES];

static ptl_match_bits_t barrierMbits = (ptl_match_bits_t)BARRIER_TYPE;

static int eventListSize;
static void freeEventList();
static int addEventList(ptl_event_t *ev);
static int searchEventList(int nid, int pid, ptl_event_t *evOut);

static ptl_process_id_t any_process;

static int collInit = 0;

static time_t t1;

static void
set_failure(int nid, int pid,  int routine, int op, int what)
{
    if (nid != SRVR_INVAL_NID){
        dsrvr_failInfo.last_nid = nid;
        dsrvr_failInfo.last_pid = pid;
    }
    if (routine != DSRVR_NOROUTINE) dsrvr_failInfo.dsrvrRoutine = routine;
    if (op != UNKNOWN_OP) dsrvr_failInfo.operation = op;
    if (what != WHAT_NOT_SET) dsrvr_failInfo.what      = what;
}
/*************************************************************
** Collective portal management
**   portal has 4 MEs in this order:
**
**        barrier
**        broadcast
**        gather
**        reduce
*************************************************************/
static int
log2x(x)
{
int i;

   for (i=0; 1<<i < x; i++);

   return i;
}
static void
init_coll_structs()
{
int i;

    for (i=0; i<NUM_OP_TYPES; i++){
        opTypes[i].mddef.eventq = SRVR_INVAL_EQ_HANDLE;
        opTypes[i].me = SRVR_INVAL_ME_HANDLE;
        opTypes[i].md = NULL;
    }
}
static void
clear_me(opInfo *info)
{
   if (info->me != SRVR_INVAL_ME_HANDLE){
       PtlMEUnlink(info->me);
       info->me = SRVR_INVAL_ME_HANDLE;
   }
}
static void
clear_eq(opInfo *info)
{
   if (info->mddef.eventq != SRVR_INVAL_EQ_HANDLE){
       PtlEQFree(info->mddef.eventq);
       info->mddef.eventq = SRVR_INVAL_EQ_HANDLE;
   }
}
static void
clear_buf(char *buf)
{
    if (buf){
        free(buf);
        buf = NULL;
    }
}
static int
reset_eq(int type)     
{
int rc;
ptl_handle_eq_t newq;

    /*
    ** Reset the event queue.  Need to do this on DSRVR_EXTERNAL_ERROR
    ** when you give up on an incomplete collective operation.
    */
    if ( (type <= NO_TYPE) || (type >= NUM_OP_TYPES)){
        CPerrno = EINVAL;
        return DSRVR_ERROR;
    }

    if (opTypes[type].eqsize > 0){

	rc = PtlEQAlloc(__SrvrLibNI, opTypes[type].eqsize, &newq);

        if (rc != PTL_OK){
            COLLP3ERROR;
        }

        clear_eq(opTypes + type);

	opTypes[type].mddef.eventq = newq;
    }

    if (type == BARRIER_TYPE){
        freeEventList();  /* saved barrier events */
    }

    return DSRVR_OK;
}
int
server_coll_init()
{
int nmsgs, log2max, rc, i;
ptl_match_bits_t mbits, ibits;
ptl_md_t *mddef;
ptl_handle_me_t *me;
ptl_handle_md_t *md;

    if (collInit) return DSRVR_OK;

    init_coll_structs();

    any_process.nid = PTL_ID_ANY;
    any_process.pid = PTL_ID_ANY;
    any_process.addr_kind = PTL_ADDR_NID;

    log2max = log2x(MAX_NODES);
    nmsgs = 3 * log2max;

    opTypes[BARRIER_TYPE].eqsize = nmsgs;
    opTypes[GATHER_TYPE].eqsize = nmsgs;
    opTypes[REDUCE_TYPE].eqsize = nmsgs;
    opTypes[BCAST_TYPE].eqsize = 3;
    opTypes[SEND_TYPE].eqsize = 3;

    opTypes[BARRIER_TYPE].num_md = 1;
    opTypes[GATHER_TYPE].num_md = 1;
    opTypes[REDUCE_TYPE].num_md = nmsgs;
    opTypes[BCAST_TYPE].num_md = 1;
    opTypes[SEND_TYPE].num_md = 1;

    for (i=1; i<NUM_OP_TYPES; i++){

	if ((rc = PtlEQAlloc(__SrvrLibNI, 
                              opTypes[i].eqsize, 
                              &(opTypes[i].mddef.eventq))) != PTL_OK){
	    COLLP3ERROR;
	}

        opTypes[i].md = (ptl_handle_md_t *)malloc(sizeof(ptl_handle_md_t) *
                                                  opTypes[i].num_md);

        if (!opTypes[i].md){
	    server_coll_done();
	    CPerrno = ENOMEM;
	    return DSRVR_RESOURCE_ERROR;
        }
    }

    /*
    ** Barrier setup - barrier ME is live as soon as MD is attached here
    */


    mddef = &(opTypes[BARRIER_TYPE].mddef);
    me    = &(opTypes[BARRIER_TYPE].me);
    md    = opTypes[BARRIER_TYPE].md;

    mddef->start     = NULL;
    mddef->length       = 0;
    mddef->threshold = PTL_MD_THRESH_INF;
    mddef->max_offset = 0;
    mddef->options   = PTL_MD_OP_PUT;
    mddef->user_ptr  = NULL;

    eventListSize = 0;

    ibits = 0xFFFFFFFF00000000;

    rc = PtlMEAttach(__SrvrLibNI, COLLPORTALS, 
                     any_process, barrierMbits, ibits, PTL_RETAIN,
                     me);

    if (rc != PTL_OK){
        server_coll_done();
        COLLP3ERROR;
    }

    rc = PtlMDAttach(*me, *mddef, PTL_RETAIN, md);

    if (rc != PTL_OK){
        server_coll_done();
        COLLP3ERROR;
    }

    /*
    ** Broadcast setup - ME is a placeholder for now
    */
    mddef = &(opTypes[BCAST_TYPE].mddef);
    me    = &(opTypes[BCAST_TYPE].me);

    mddef->start     = NULL;
    mddef->length       = 0;
    mddef->threshold = 0;
    mddef->max_offset = 0;
    mddef->options   = PTL_MD_OP_PUT;
    mddef->user_ptr  = NULL;

    mbits = (ptl_match_bits_t)0;
    ibits = (ptl_match_bits_t)0;

    rc = PtlMEInsert(opTypes[BARRIER_TYPE].me, any_process, mbits, ibits, 
                     PTL_RETAIN, PTL_INS_AFTER, me);

    if (rc != PTL_OK){
        server_coll_done();
        COLLP3ERROR;
    }

    /*
    ** Gather setup - it's a placeholder too
    */
    mddef = &(opTypes[GATHER_TYPE].mddef);
    me    = &(opTypes[GATHER_TYPE].me);

    mddef->start      = NULL;
    mddef->length     = 0;
    mddef->threshold  = 0;
    mddef->max_offset = 0;
    mddef->options    = PTL_MD_OP_PUT | PTL_MD_MANAGE_REMOTE;
    mddef->user_ptr   = NULL;

    mbits = (ptl_match_bits_t)0;
    ibits = (ptl_match_bits_t)0;

    rc = PtlMEInsert(opTypes[BCAST_TYPE].me, any_process, mbits, ibits, 
                     PTL_RETAIN, PTL_INS_AFTER, me);

    if (rc != PTL_OK){
        server_coll_done();
        COLLP3ERROR;
    }

    /*
    ** Reduce setup - just a placeholder for now
    */
    mddef = &(opTypes[REDUCE_TYPE].mddef);
    me    = &(opTypes[REDUCE_TYPE].me);

    mddef->start      = NULL;
    mddef->length     = 0;
    mddef->threshold  = 0;
    mddef->max_offset = 0;
    mddef->options    = PTL_MD_OP_PUT;
    mddef->user_ptr   = NULL;

    mbits = (ptl_match_bits_t)0;
    ibits = (ptl_match_bits_t)0;

    rc = PtlMEInsert(opTypes[GATHER_TYPE].me, any_process, mbits, ibits, 
                     PTL_RETAIN, PTL_INS_AFTER, me);

    if (rc != PTL_OK){
        server_coll_done();
        COLLP3ERROR;
    }

    /*
    ** Send setup 
    */
    mddef = &(opTypes[SEND_TYPE].mddef);

    mddef->start      = NULL;
    mddef->length     = 0;
    mddef->threshold  = 0;
    mddef->max_offset = 0;
    mddef->options    = 0;
    mddef->user_ptr   = NULL;

    collInit = 1;

    return DSRVR_OK;
}
void
server_coll_done()
{
int i;

    for (i=1; i<NUM_OP_TYPES; i++){
        clear_eq(opTypes + i);
        clear_me(opTypes + i);
        clear_buf((char *)opTypes[i].md);
    }

    collInit = 0;
}
/*
** Use this for fault recovery.  Just dash the collective portal and
** rebuild it from scratch.
**
** Note this resets the barrier portal too, so it will be momentarily
** down.  Better not be synchronizing with it after the reset.
*/
int
srvr_reset_coll()
{
    server_coll_done();
    return server_coll_init();
}

static int
await_senders(int *nids, int *pids, int nsenders, int where, int optype, int tmout)
{
int status, i, rc;
int ndone;
char gotmsg[MAXFANIN];
ptl_event_t ev;

    t1 = time(NULL);
    status = DSRVR_OK;
    ndone = 0;

    if (__SrvrDebug){
        printf("await_senders: %d senders, for %s, timeout %d\n",
	             nsenders,dsrvr_routines[where],tmout);

        for (i=0; i<nsenders; i++){
	    printf("           %d  (%d/%d)\n",
		   memberRankByNidPid(nids[i],pids[i]),
		   nids[i], pids[i]);
	}
        
    }
    memset(gotmsg, 0, MAXFANIN);

    if ( (optype == BARRIER_TYPE) && eventListSize){

        /*
	** msgs may have come in during a previous barrier, if we
	** have different subgroups out there doing barriers.  Check
	** the "saved" list.
	*/
        for (i=0; i<nsenders; i++){

	    rc = searchEventList(nids[i], pids[i], &ev);

            if (rc){
		ndone++;

		gotmsg[i] = 1;

                if (__SrvrDebug){
                    printf("    got %d from saved list (%d bytes)\n",
		       memberRankByNidPid(nids[i], pids[i]),
		       ev.mlength);
                }

	        if (eventListSize == 0) break;
            }

	}
    }

    while (ndone < nsenders){

        if (tmout && ((time(NULL) - t1) > tmout)){

            for (i=0; i<nsenders; i++){
                if (!gotmsg[i]){
                    break;
                }
            }

            set_failure(nids[i], pids[i], where, RECEIVE_OP, TIMEDOUT_WAITING);

            CPerrno = ERECVTIMEOUT;
            status = DSRVR_EXTERNAL_ERROR;

            break;
        }

        rc = PtlEQGet(opTypes[optype].mddef.eventq, &ev);

        if (rc == PTL_EQ_EMPTY){
            continue;
        }
        if ( ((rc != PTL_OK) && (rc != PTL_EQ_DROPPED))  ||
	     (ev.type != PTL_EVENT_PUT)){

	    COLLP3ERROR;
        }
	if (rc == PTL_EQ_DROPPED){
	    log_msg("warning - event dropped in await senders");
        }

        for (i=0; i<nsenders; i++){

            if ( !gotmsg[i] &&
                 (ev.initiator.nid == nids[i]) &&
                 (ev.initiator.pid == pids[i])     ){

                gotmsg[i] = 1;

                if (__SrvrDebug){
                    printf("    got %d (%d bytes)\n",
		       memberRankByNidPid(nids[i],pids[i]),
		       ev.mlength);
                }

                ndone++;
		break;
            }
        }
        if (i == nsenders){

	    if (optype == BARRIER_TYPE){
                if (__SrvrDebug){
                    printf("    unexpected msg from %d/%d (%d bytes) - SAVE IT\n",
		       ev.initiator.nid, ev.initiator.pid, ev.mlength);
                }
	        addEventList(&ev);   /* save it */
	    }
	    else{
	        log_warning(
	        "await_senders: op %d, ignore message from unexpected nid/pid %d/%d",
	         optype, ev.initiator.nid, ev.initiator.pid);
	    }

	    continue;
        }
    }

    if (status != DSRVR_OK){
        reset_eq(optype);
    }

    return status;
}
#ifdef REDUCE_WORKS
static int
setup_reduce(int nsenders, int type, char *buf, int blklen)
{
ptl_match_bits_t mbits, ibits;
int rc, i;
char *c;
opInfo *reduce;

    reduce = opTypes + REDUCE_TYPE;

    if (nsenders > reduce->num_md){
        CPerrno = EINVAL;
        return DSRVR_ERROR;
    }

    rc = PtlMEUnlink(reduce->me);

    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    mbits = ((ptl_match_bits_t)(REDUCE_TYPE) << 32) | type;
    ibits = 0L;

    rc = PtlMEInsert(opTypes[BCAST_TYPE].me, any_process, mbits, ibits, 
                     PTL_RETAIN, PTL_INS_AFTER, &(reduce->me));

    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    reduce->mddef.threshold = 1;

    for (c=buf, i=0; i<nsenders; i++, c += blklen){

        reduce->mddef.start = c;
        reduce->mddef.length = blklen;
	reduce->mddef.max_offset = blklen;

        if (i==0){
             rc = PtlMDAttach(reduce->me, reduce->mddef, PTL_RETAIN, reduce->md+i);
        }
        else{
             rc = PtlMDInsert(reduce->md[i-1], reduce->mddef, 
                             PTL_RETAIN, PTL_INS_AFTER, reduce->md+i);
        }
    }

    return DSRVR_OK;
}
#endif
static int
setup_gather(int nsenders, int type, char *buf, int len)
{
int rc;
opInfo *gather;
ptl_match_bits_t mbits, ibits;

    gather = opTypes + GATHER_TYPE;

    gather->mddef.start      = buf;
    gather->mddef.length     = len;
    gather->mddef.threshold  = nsenders;
    gather->mddef.max_offset = len;

    mbits = ((ptl_match_bits_t)(GATHER_TYPE) << 32) | type;
    ibits = 0L;

    rc = PtlMEUnlink(gather->me);
    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    rc = PtlMEInsert(opTypes[BCAST_TYPE].me, any_process, mbits, ibits,
                      PTL_RETAIN, PTL_INS_AFTER, &(gather->me));
    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    rc  = PtlMDAttach(gather->me, gather->mddef, PTL_RETAIN, gather->md);

    if (rc != PTL_OK){
        COLLP3ERROR;
    }
    return DSRVR_OK;
}
static int
setup_bcast(int nid, int pid, int type, char *buf, int len)
{
ptl_match_bits_t mbits, ibits;
int rc;
opInfo *bcast;
ptl_process_id_t from;

    from.nid = nid;
    from.pid = pid;
    from.addr_kind = PTL_ADDR_NID;

    bcast = opTypes + BCAST_TYPE;

    bcast->mddef.start = buf;
    bcast->mddef.length   = len;
    bcast->mddef.threshold = 1;
    bcast->mddef.max_offset = len;

    mbits = ((ptl_match_bits_t)(BCAST_TYPE) << 32) | type;
    ibits = 0L;

    rc = PtlMEUnlink(bcast->me);
    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    rc = PtlMEInsert(opTypes[BARRIER_TYPE].me, from, mbits, ibits,
                      PTL_RETAIN, PTL_INS_AFTER, &(bcast->me));
    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    rc  = PtlMDAttach(bcast->me, bcast->mddef, PTL_RETAIN, bcast->md);

    if (rc != PTL_OK){
        COLLP3ERROR;
    }
    return DSRVR_OK;
}
static int
coll_send(ptl_md_t mddef, int nid, int pid,
          ptl_match_bits_t mbits, int offset, ptl_ack_req_t ack)
{
ptl_handle_md_t md;
ptl_event_t ev;
int status, rc, sent, acked;
ptl_process_id_t dst;

    status = DSRVR_OK;

    if (__SrvrDebug){
        printf("coll_send : to rank %d (%d/%d) %d bytes\n",
         memberRankByNidPid(nid,pid), nid, pid, mddef.length) ;
    }

    rc = PtlMDBind(__SrvrLibNI, mddef, &md);

    if (rc != PTL_OK){
        COLLP3ERROR;
    }

    dst.nid = nid;
    dst.pid = pid;
    dst.addr_kind = PTL_ADDR_NID;

    rc = PtlPut(md, ack, dst, COLLPORTALS, SRVR_ACL_ANY,
                mbits, offset, 0);

    if (rc != PTL_OK){
	PtlMDUnlink(md);
        COLLP3ERROR;
    }

    t1 = time(NULL);

    acked = sent = 0;

    while (1){

        if ((time(NULL) - t1) > SEND_TIMEOUT){

            CPerrno = ESENDTIMEOUT;
	    
            srvrHandleSendTimeout(mddef, md);  /* does a PtlMDUnlink */

	    return DSRVR_EXTERNAL_ERROR;  /* not *my* problem */
        }

	rc = PtlEQGet(mddef.eventq, &ev);

	if (rc == PTL_EQ_EMPTY){
	    continue;
	}
        if ((rc != PTL_OK) && (rc != PTL_EQ_DROPPED)){
            PtlMDUnlink(md);
            COLLP3ERROR;
        }
        if (rc == PTL_EQ_DROPPED){
	    log_msg("warning - event dropped in coll_send");
	}
        /*
        ** According to Rolf, the ACK can strangely enough precede
        ** the SENT in the case where the receiving node's confirmation
        ** that data has arrived gets lost, and then the ACK gets
        ** sent, and then confirmation is re-sent.
        */
	if (ev.type == PTL_EVENT_SENT){

	    sent = 1;

            if (__SrvrDebug){
	        printf("        got PTL_EVENT_SENT\n");
	    }

	    if ((ack == PTL_NOACK_REQ) || acked){
                break;
            }
        }
        else if (ev.type == PTL_EVENT_ACK){
	    acked = 1;

            if (__SrvrDebug){
	        printf("        got PTL_EVENT_ACK\n");
	    }

	    if (sent){
                break;
	    }
        }
    }
    PtlMDUnlink(md);

    return status;
}

/*
********************************************************************************
**
**   Collective operations
**
**   barrier
**   gather
**   reduce
**   broadcast
**
** Return codes indicate fault recovery required:
**
**    DSRVR_OK    - operation completed successfully
**    DSRVR_ERROR - invalid arguments to call, but no server recovery required
**    DSRVR_RESOURCE_ERROR - local failure in portal use, memory allocation etc,
**                           better leave distributed service and exit.
**    DSRVR_EXTERNAL_ERROR - failure of an external member of distributed
**                           service, go into abnormal completion protocol,
**                           then back to work.
**
**    CPerrno is set with more information, printf("%s",CPstrerror(CPerrno)) 
**    to display error string.
**
**    Also, in the case of DSRVR_EXTERNAL_ERROR the global dsrvr_failInfo 
**    is set with information identifying that bad remote node.
**    dsrvr_who_failed() will return a string describing the external
**    failure.
**
** In order to be fault tolerant, blocking calls must include a timeout.  
** The timeout is not strictly observed, we take it to be an order of
** magnitude.  Actual timeout may be greater, since your timeout value
** is passed unaltered to subroutines.  A timeout of 0 indicates call
** should block indefinitely.
**
** These collective ops require that a group has been formed.  See the
** membership routines in srvr_coll_membership to see how this is done.
********************************************************************************
*/


static void
nidpid(int rank, int *list, int *nid, int *pid)
{
    if (list){
	*nid = memberNidByRank(list[rank]);
	*pid = memberPidByRank(list[rank]);
    }
    else{
	*nid = memberNidByRank(rank);
	*pid = memberPidByRank(rank);
    }
}

/*************************************************************
**          BARRIER
**
** Group ranks in list go into a barrier.  If list==NULL, all
** members of group go into barrier.
**************************************************************
*/

int
dsrvr_barrier(int tmout, int *list, int listLen)
{
int i, myelt;
int nrecvs, nsends;
int status;
int inNidSpecial[MAXFANIN], inPidSpecial[MAXFANIN];
int outNidSpecial, outPidSpecial;
int *inNids, *inPids, *outNid, *outPid;

    if (!collInit) return DSRVR_ERROR;

    if (!dsrvrMembersInited){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
    }

    if (__SrvrDebug) printf("BARRIER START\n");

    if ((dsrvrMembers==1) || (list && (listLen == 1))) return DSRVR_OK;

    dsrvr_clear_fail_info();

    if (!list){
        myelt = dsrvrMyGroupRank;
        listLen = dsrvrMembers;

	nrecvs = fanInSources;
	nsends = fanInTargets;

	inNids = fanInFromNids;
	inPids = fanInFromPids;

	outNid = &fanInToNid;
	outPid = &fanInToPid;
    }
    else{ 
        myelt = dsrvr_getMyRelativeRank(list, listLen);
        
        if (myelt == -1){   
            if (CPerrno == EINVAL){
                return DSRVR_ERROR;  /* bad list */
            }
            else{
                if (__SrvrDebug) printf("BARRIER END\n");
                return DSRVR_OK;     /* I don't participate */
            }
        }
	dsrvr_calc_fan_in(list, listLen, myelt,
	            &nrecvs, inNidSpecial, inPidSpecial,
		    &nsends, &outNidSpecial, &outPidSpecial);

        inNids = inNidSpecial;
        inPids = inPidSpecial;
	outNid = &outNidSpecial;
	outPid = &outPidSpecial;
    }

    status = DSRVR_OK;

    /* fan in : do the receives and then the send */

    if (nrecvs){
        status = await_senders(inNids, inPids, nrecvs, DSRVR_BARRIER, BARRIER_TYPE, tmout);

        if (status != DSRVR_OK) return status;
    }
    if (nsends){

        opTypes[SEND_TYPE].mddef.start = NULL;
        opTypes[SEND_TYPE].mddef.length = 0;
        opTypes[SEND_TYPE].mddef.threshold = 1;
        opTypes[SEND_TYPE].mddef.max_offset = 0;

        status = coll_send(opTypes[SEND_TYPE].mddef, *outNid, *outPid,
                   barrierMbits, 0, PTL_NOACK_REQ);

        if (status != DSRVR_OK){ 

	    if (CPerrno == ESENDTIMEOUT){
		set_failure(*outNid, *outPid, 
		           DSRVR_BARRIER, SEND_OP, TIMEDOUT_WAITING);            

	    } 

	    return status;
	}

    }

    /* fan out : receive from the "send" and send to the "receives" */

    if (nsends){
        status = await_senders(outNid, outPid, 1, DSRVR_BARRIER, BARRIER_TYPE, tmout);

        if (status != DSRVR_OK) return status;
    }

    for (i=nrecvs-1; i >= 0 ; i--){

        opTypes[SEND_TYPE].mddef.start = NULL;
        opTypes[SEND_TYPE].mddef.length = 0;
        opTypes[SEND_TYPE].mddef.threshold = 1;
        opTypes[SEND_TYPE].mddef.max_offset = 0;

        status = coll_send(opTypes[SEND_TYPE].mddef,
                   inNids[i], inPids[i],
                   barrierMbits, 0, PTL_NOACK_REQ);

        if (status != DSRVR_OK){ 

	    if (CPerrno == ESENDTIMEOUT){
		set_failure(inNids[i], inPids[i],  
		            DSRVR_BARRIER, SEND_OP, TIMEDOUT_WAITING);            
	    } 

	    return status;
	}

    }
    if (__SrvrDebug) printf("BARRIER END\n");

    return status;
}

/*************************************************************
**                  GATHER
**
**  data -   The entire buffer which will contain all members blocks
**           on root node when operation completes, my contribution
**           is in my block.
**  blklen - size of one member's entry
**  nblks  - number of members/blocks
**  tmout  - timeout in seconds for collective op (0 if blocking)
**  type   - a message type that will serve to verify correctness
**           (all members must use same message type)
**
**  list   - list of ranks participating, list==NULL means all members
**  listLen - redundant, this should always be nblks
**************************************************************
*/

int
dsrvr_gather(char *data, int blklen, int nblks, int tmout, int type,
             int *list, int listLen)
{
ptl_match_bits_t mbits;
int status;
int myelt, i;
int left, right, mid;
int iroot, src;
int offset, len;
int clog;
int nsends, nrecvs;
int rnids[MAXFANIN], rpids[MAXFANIN];
int snid, spid;
blockInfo rblock[MAXFANIN], sblock;

    if (!collInit) return DSRVR_ERROR;

    if (!dsrvrMembersInited){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
    }
    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_GATHER;

    if (nblks == 1) return DSRVR_OK;

    if (!list){
        myelt = dsrvrMyGroupRank;
        listLen = dsrvrMembers;
    }
    else{ 
        myelt = dsrvr_getMyRelativeRank(list, listLen);

        if (myelt == DSRVR_ERROR){   
            if (CPerrno == EINVAL){
                return DSRVR_ERROR;  /* bad list */
            }
            else{
                return DSRVR_OK;     /* I don't participate */
            }
        }
    }


    /*
    ** Do a gather - using algorithm lifted from Tramm's old puma collcore
    */
    clog  = log2x(listLen);
    iroot = 0;
    left  = 0;
    right = listLen - 1;

    nsends = nrecvs = 0;

    for (i = 0; i < clog; i++) {

        mid = (left + right) / 2;
        if (iroot <= mid) {
            src = (iroot == left ? mid + 1 : right);
        } else {
            src = (iroot == right ? mid : left);
        }

        /*
        ** calculate the message segment
        */
        if (src <= mid) {                   /* left ... mid */
            offset = left * blklen;
            len =  (mid - left + 1) * blklen;
        } else {                            /* mid+1 ... right */
            offset = (mid + 1) * blklen;
            len    = (right - mid) * blklen;
        }

        /*
        ** sender/receiver information
        */
        if (myelt==iroot) {

            nidpid(src, list, rnids+nrecvs, rpids+nrecvs);

            rblock[nrecvs].offset = offset;
            rblock[nrecvs].len = len;

            nrecvs++;

        } else if (myelt==src) {

            nidpid(iroot, list, &snid, &spid);

            sblock.offset = offset;
            sblock.len = len;

            nsends++;
        }

        /*
        ** subdivide mesh partitions
        */
        if (myelt <= mid) {
            if (iroot > mid) {
                iroot = src;
            }
            right = mid;
        } else {
            if (iroot <= mid) {
                iroot = src;
            }
            left = mid + 1;
        }
        if (left == right) break;
    }

    /*
    ** Await the receives, if any
    */
    status = DSRVR_OK;

    if (nrecvs){

        status = setup_gather(nrecvs, type, data, blklen * nblks);

        if (status != DSRVR_OK){
            return status;
        }

        /*
        ** Portal is set up to receive, so we're OK to go
        */
	status = dsrvr_barrier(tmout, list, listLen);

        /*
        ** Await them
        */
        if (status == DSRVR_OK){

            status = await_senders(rnids, rpids, nrecvs, 
				     DSRVR_GATHER, GATHER_TYPE, tmout);

        }

    }
    else{
	status = dsrvr_barrier(tmout, list, listLen);
    }

    /*
    ** Do the send, if I'm not the root of the gather
    */

    if ((status == DSRVR_OK) && nsends){

        opTypes[SEND_TYPE].mddef.start = data + sblock.offset;
        opTypes[SEND_TYPE].mddef.length = sblock.len;
        opTypes[SEND_TYPE].mddef.threshold = 1;
        opTypes[SEND_TYPE].mddef.max_offset = sblock.len;

        mbits = ((ptl_match_bits_t)GATHER_TYPE << 32) | type;
        
        status = coll_send(opTypes[SEND_TYPE].mddef,
	               snid, spid, mbits,
                       sblock.offset, PTL_NOACK_REQ);

        if (status != DSRVR_OK){ 

	    if (CPerrno == ESENDTIMEOUT){
		set_failure(snid, spid,  DSRVR_GATHER, SEND_OP, TIMEDOUT_WAITING);            
	    } 
	    return status;
	}

    }
 
    return status;
}
/*************************************************************
**                BROADCAST
**************************************************************
*/
unsigned char dsrvr_bcast_cksum=0;
char dsrvr_do_cksum=0;

static int
check_incoming(unsigned char *buf, int len)
{
int i;
unsigned char mysum;

    mysum = 0;

    for (i=0; i<len; i++){
        mysum ^= buf[i];
    }

    if (mysum == dsrvr_bcast_cksum){
        return DSRVR_OK;
    }
    else{
        log_msg("check_incoming: error, mysum %x, yod's sum %x",mysum,dsrvr_bcast_cksum);
        CPerrno = ECORRUPT;
        /*
	** Could be sender, or problem could be on my node, I just don't know. 
	** For now we call it an external error and carry on.  Sys admins will
	** be told of message corruption and investigate further.
	*/
        return DSRVR_EXTERNAL_ERROR; /* Could be sender, but problem could be on my node though, I don't know. */
    }
}

int
dsrvr_bcast(char *buf, int len, int tmout, int type, int *list, int listLen)
{
int status, myelt;
int nsends, nrecv, i;
ptl_match_bits_t mbits;
int outNidSpecial[MAXFANIN], outPidSpecial[MAXFANIN];
int inNidSpecial, inPidSpecial;
int *inNid, *inPid, *outNids, *outPids;

    if (!collInit) return DSRVR_ERROR;

    status = DSRVR_OK;

    if (!dsrvrMembersInited){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
    }
    if (__SrvrDebug) printf("BROADCAST START\n");

    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_BCAST;

    if ((dsrvrMembers==1) || (list && (listLen==1))) return DSRVR_OK;

    if (!list){
        myelt = dsrvrMyGroupRank;
        listLen = dsrvrMembers;

        nsends = fanInSources;   /* we do a backwards fan in */
        nrecv  = fanInTargets;

        outNids = fanInFromNids;
        outPids = fanInFromPids;

        inNid = &fanInToNid;
        inPid = &fanInToPid;
    }
    else{ 
        myelt = dsrvr_getMyRelativeRank(list, listLen);

        if (myelt == -1){   
            if (CPerrno == EINVAL){
                return DSRVR_ERROR;  /* bad list */
            }
            else{
                if (__SrvrDebug) printf("BROADCAST END\n");
                return DSRVR_OK;     /* I don't participate */
            }
        }
	if (__SrvrDebug) printf("My list rank: %d\n",myelt);

        dsrvr_calc_fan_in(list, listLen, myelt,
                    &nsends, outNidSpecial, outPidSpecial,
                    &nrecv, &inNidSpecial, &inPidSpecial);

        inNid = &inNidSpecial;
        inPid = &inPidSpecial;
        outNids = outNidSpecial;
        outPids = outPidSpecial;

    }

    mbits = ((ptl_match_bits_t)(BCAST_TYPE) << 32) | type;

    /*
    ** receive buffer to broadcast (if I'm not the root)
    */

    if (nrecv){

         status = setup_bcast(*inNid, *inPid, type, buf, len);

         if (status != DSRVR_OK){
             return status;
         }
	 status = dsrvr_barrier(tmout, list, listLen);

         if (status != DSRVR_OK){
             return status;
         }


         status = await_senders(inNid, inPid, 1, DSRVR_BCAST, BCAST_TYPE, tmout);

         if ((status == DSRVR_OK) && dsrvr_do_cksum){
             status = check_incoming(buf, len);

             if (status != DSRVR_OK){
                 set_failure(*inNid, *inPid, DSRVR_BCAST, RECEIVE_OP, CORRUPT_MESSAGE);
             }
         }
    }
    else{
	status = dsrvr_barrier(tmout, list, listLen);
    }

    dsrvr_do_cksum = 0;

    if (status != DSRVR_OK){
	return status;
    }

    /*
    ** Send buffer along
    */
    for (i = nsends-1; i >= 0; i--){

        opTypes[SEND_TYPE].mddef.start = buf;
        opTypes[SEND_TYPE].mddef.length = len;
        opTypes[SEND_TYPE].mddef.threshold = 1;
        opTypes[SEND_TYPE].mddef.max_offset = len;

        status = coll_send(opTypes[SEND_TYPE].mddef,
	      outNids[i], outPids[i], mbits, 0, PTL_NOACK_REQ);

        if (status != DSRVR_OK){ 

	    if (CPerrno == ESENDTIMEOUT){
		set_failure(outNids[i], outPids[i], DSRVR_BCAST, SEND_OP, 
		            TIMEDOUT_WAITING);            

	    } 
	    return status;
	}
    }

    if (__SrvrDebug) printf("BROADCAST END\n");

    return status;
}
/*************************************************************
**                REDUCE
**
** Data is combined using op with data fanned in from other nodes, 
** then fanned in.
**
**  op    a function that performs the pairwise reduction on all
**        data fanned in.  Result is propagated along.
**  data  buffer containing the operand. 
**  len   length of buffer
**  tmout in seconds for reduction operation
**  type  message type used to verify all members are doing the same
**        thing
**************************************************************
*/

#ifdef REDUCE_WORKS

/*
** still need to do the PUMA_OP part
*/

#define PUMA_OP int

int
dsrvr_reduce(PUMA_OP op, char *data, int len, int tmout, int type, 
                int *list, int listLen)
{
int status, rc;
int myelt, i;
char *sourcedata;
ptl_match_bits_t mbits;
iny nrecvs, nsends;
int inNidSpecial[MAXFANIN], inPidSpecial[MAXFANIN];
int outNidSpecial, outPidSpecial;
int *inNids, *inPids, *outNid, *outPid;

    if (!collInit) return DSRVR_ERROR;

    status = DSRVR_OK;

    if (!dsrvrMembersInited){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
    }
    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_REDUCE;

    if ((dsrvrMembers==1) || (list && (listLen==1))) return DSRVR_OK;

    if (!list){
        myelt = dsrvrMyGroupRank;
        listLen = dsrvrMembers;

        nrecvs = fanInSources;
        nsends = fanInTargets;

        inNids = fanInFromNids;
        inPids = fanInFromPids;

        outNid = &fanInToNid;
        outPid = &fanInToPid;
    }
    else{ 
        myelt = dsrvr_getMyRelativeRank(list, listLen);

        if (myelt == -1){   
            if (CPerrno == EINVAL){
                return DSRVR_ERROR;  /* bad list */
            }
            else{
                return DSRVR_OK;     /* I don't participate */
            }
        }
        dsrvr_calc_fan_in(list, listLen,
                    &nrecvs, inNidSpecial, inPidSpecial,
                    &nsends, &outNidSpecial, &outPidSpecial);

        inNids = inNidSpecial;
        inPids = inPidSpecial;
        outNid = &outNidSpecial;
        outPid = &outPidSpecial;
    }

    mbits = ((ptl_match_bits_t)(REDUCE_TYPE) << 32) | type;

    if (nrecvs){
        sourcedata = (char *)malloc(nrecvs * len);

	if (!sourcedata){
	    CPerrno = ENOMEM;
	    return DSRVR_RESOURCE_ERROR;
	}

	status = setup_reduce(nrecvs, type, sourcedata, len);

	if (status != DSRVR_OK){
	     free(sourcedata);
	     return status;
	}
    }
    /*
    ** Wait until all participants are set up
    */

    status = dsrvr_barrier(tmout, list, listLen);

    if (status != DSRVR_OK){

	free(sourcedata);
        return status;

    }
    /*
    ** Collect reduction operands
    */
    if (nrecvs){

	status = await_senders(inNids, inPids, nrecvs, DSRVR_REDUCE, REDUCE_TYPE, tmout);

	if (status != DSRVR_OK){
	    free(sourcedata);
	    return rc;
	}

	for (i = 1; i < nsources; i++){

             /*
	     ** Combine the operands including mine
	     */

	}
    }
    free(sourcedata);

    /*
    ** send mine along
    */
    if (nsends){

        opTypes[SEND_TYPE].mddef.start = data;
        opTypes[SEND_TYPE].mddef.length = len;
        opTypes[SEND_TYPE].mddef.threshold = 1;
        opTypes[SEND_TYPE].mddef.max_offset = len;

        status = coll_send(opTypes[SEND_TYPE].mddef, *outNid, *outPid,
	                   mbits, 0, PTL_NOACK_REQ);

        if (status != DSRVR_OK){ 

	    if (CPerrno == ESENDTIMEOUT){
		set_failure(*outNid, *outPid, DSRVR_REDUCE, SEND_OP, TIMEDOUT_WAITING);            
	    } 
	    return status;
	}


    }

    return status;
}
#endif
/*
** We need to save events from barrier portal.  They may not arrive
** in the order in which we search for them.  (Can happen when subgroups
** go into barriers.)
**
** If we needed to do this for more ops, we could search on mbits
** too.  For now we assume all saved events are from the barrier portal.
*/
typedef struct _eList{
    ptl_event_t ev;
    struct _eList *next;
    struct _eList *prev;
} eList;

static eList *eListFirst;
static eList *eListLast;

static void
freeEventList()
{
eList *el, *enext;
int hopcount;

    hopcount = eventListSize;

    if (eventListSize > 0){

        el = eListFirst;

	while (el && hopcount--){

	    enext = el->next;
	    free(el);
	    el = enext;
	    
	}
    }
    eventListSize = 0;
    eListFirst = eListLast = NULL;
}
static int
addEventList(ptl_event_t *ev)
{
eList *el;

    if (eventListSize == 0){
        eListFirst = eListLast = NULL;
    }

    el = (eList *)malloc(sizeof(eList));

    if (!el){
	 CPerrno = ENOMEM;
	 return DSRVR_RESOURCE_ERROR;
    }
    memcpy(&(el->ev), ev, sizeof(ptl_event_t));

    if (eListLast){

	eListLast->next = el;
	el->prev = eListLast;
	el->next = NULL;

	eListLast = el;

	return DSRVR_OK;
    }
    else{
        eListFirst = eListLast = el;
	el->next = NULL;
	el->prev = NULL;
    }

    eventListSize++;

    return DSRVR_OK;
}
/*
** returns 0 if not found, 
**         1 if found, and copies event to evOut
*/
static int
searchEventList(int nid, int pid, ptl_event_t *evOut)
{
ptl_event_t *ev;
eList *el;
int hopcount;
int status;

    status = 0;

    if (eventListSize == 0) return 0;

    hopcount = eventListSize;

    el = eListFirst;
    ev = NULL;

    while (el && hopcount--){

	if ( (el->ev.initiator.nid == nid) &&
	     (el->ev.initiator.pid == pid)    ){

            ev = &(el->ev);
	    status = 1;
	    break;
        }

	el = el->next;
    }

    if (ev){      /* remove from the list */

        if (el == eListFirst){
	    eListFirst = el->next;
	}
	else{
	    el->prev->next = el->next;
	}
        if (el == eListLast){
	    eListLast = el->prev;
	}
	else{
	    el->next->prev = el->prev;
	}

        if (evOut){
	    memcpy(evOut, ev, sizeof(ptl_event_t));
	}

	free(el);
	eventListSize--;
    }


    return status; 
}

