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
** $Id: srvr_coll_comm.c,v 1.1 2000/11/04 03:56:31 lafisk Exp $
*/
#include "qkdefs.h"
#include "portal.h"
#include "sysptl.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "srvr_coll_fail.h"
#include <malloc.h>
#include <string.h>

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif
 
/*
#define DBGCOLLCOMM
*/

extern double dclock(void);

/*
********************************************************************************
** Use puma collective ops to perform these fault tolerant collective operations
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
**                           better leave distributed service.
**    DSRVR_EXTERNAL_ERROR - failure of an external member of distributed
**                           service, go into abnormal completion protocol
**
**    CPerrno is set with more information, printf("%s",CPstrerror(CPerrno)) 
**    to display error string.
**
** In order to be fault tolerant, blocking calls must include a timeout.  
** The timeout is not strictly observed, we take it to be an order of
** magnitude.  Actual timeout may be greater, since your timeout value
** is passed unaltered to subroutines.  A timeout of 0.0 indicates call
** should block indefinitely.
**
** These collective ops require that a group has been formed.  See the
** membership routines in srvr_coll_membership to see how this is done.
********************************************************************************
*/

int
dsrvr_barrier(double tmout, int *list, int listLen)
{
dsrvrInfo myInfo;
COLL_STRUCT c_st;
int rc, ngroup, rootrank, myrank, i;

    rootrank = 0;
    dsrvr_failInfo.dsrvrRoutine = DSRVR_BARRIER;

    if (listLen && list){        /* global op on sub group only */
	ngroup = listLen;
	for (i=0; i<listLen; i++){
	    if (list[i] == dsrvrMyGroupRank){
		myrank = i;
		break;
	    }
	}
    }
    else{                      /* empty list means whole group */
	ngroup = dsrvrMembers;
	myrank = dsrvrMyGroupRank;
	list = NULL;
    }

    c_st.cntr = 0;
    c_st.proto.p_init = dsrvr_puma_init;
    c_st.proto.p_vsend = dsrvr_puma_send;
    c_st.proto.p_vrecv = dsrvr_puma_recv;
    c_st.proto.p_cleanup = dsrvr_puma_no_cleanup;
    c_st.user_struct = (void *)&myInfo;

    myInfo.ptl = DSRVR_BARRIER_PORTAL;

#ifdef DBGCOLLCOMM
    log_msg("dsrvr barrier on %d",myInfo.ptl);
#endif

    myInfo.ping_ptl  = DSRVR_DONT_PING;
    myInfo.tmout     = tmout;

    dsrvr_failInfo.collRoutine = COLL_REDUCE_SHORT;

    rc = _reduce_short(PUMA_NULLOP, NULL, 0, 0, DSRVR_BARRIER_TYPE, 
		  list, 1, ngroup, myrank, rootrank, NULL, &c_st);
   
    rc = ((rc == -1) ? DSRVR_ERROR : rc);

    if (!rc){

	c_st.proto.p_cleanup = dsrvr_puma_no_cleanup;

        dsrvr_failInfo.collRoutine = COLL_BCAST_SHORT;

	rc = _bcast_short(NULL, 0, DSRVR_BARRIER_TYPE,
		  list, 1, ngroup, myrank, rootrank,
		  &c_st);

	rc = ((rc == -1) ? DSRVR_ERROR : rc);
    }
#ifdef DBGCOLLCOMM
    log_msg("DONE dsrvr barrier");
#endif
     
    return rc;
}

/*
** dsrvr_gather:
**  o  "data" buffer must be locked down
**  o  block received must have at least one non-zero bit, so we can
**         determine if data came in or not.  Perhaps with another
**         transport below this will not be necessary.
**
**  data -   The entire buffer which will contain all members blocks
**           on root node when operation completes, my contribution
**           is in my block.  Call dsrvr_new_coll_comm_buffer on this
**           buffer beforehand, and dsrvr_free_coll_comm_buffer when
**           done with it.
**  blklen - size of one member's entry
**  nblks  - number of members/blocks
**  tmout  - timeout in seconds for collective op
**  type   - a message type that will serve to verify correctness
**           (all members must use same message type)
**
**  list   - list of ranks participating, list==NULL means all members
*/
static char **dsrvr_gatherPtrs;

int
dsrvr_gather(char *data, int blklen, int nblks, double tmout, int type,
             int *list, int listLen)
{
dsrvrInfo myInfo;
COLL_STRUCT c_st;
int i, rc, ngroup, rootrank, myrank;

    if (nblks > dsrvrMaxMembers){
        CPerrno = EINVAL;
	return DSRVR_ERROR;
    }
    rootrank = 0;
    dsrvr_failInfo.dsrvrRoutine = DSRVR_GATHER;

    if (listLen && list){        /* global op on sub group only */
	ngroup = listLen;
	for (i=0; i<listLen; i++){
	    if (list[i] == dsrvrMyGroupRank){
		myrank = i;
		break;
	    }
	}
    }
    else{                      /* empty list means whole group */
        ngroup = dsrvrMembers;
        myrank = dsrvrMyGroupRank; 
        list = NULL;
    }

    myInfo.ptl       = DSRVR_GATHER_PORTAL;
    myInfo.ping_ptl  = DSRVR_HANDSHAKE_PORTAL;
    myInfo.tmout     = tmout;

    SING_MD(DSRVR_GATHER_PORTAL).buf = data;
    SING_MD(DSRVR_GATHER_PORTAL).buf_len = blklen * nblks;

    dsrvr_gatherPtrs[0] = data;
    for (i=1; i<=nblks; i++){
	dsrvr_gatherPtrs[i] = dsrvr_gatherPtrs[i-1] + blklen;
    }

    c_st.cntr = 0;
    c_st.proto.p_init = dsrvr_puma_init;
    c_st.proto.p_vsend = dsrvr_puma_send;
    c_st.proto.p_vrecv = dsrvr_puma_recv;
    c_st.proto.p_cleanup = dsrvr_puma_no_cleanup;
    c_st.user_struct = (void *)&myInfo;

    SPTL_ACTIVATE(DSRVR_GATHER_PORTAL);

    dsrvr_failInfo.collRoutine = COLL_GATHER;

    rc = _gather(dsrvr_gatherPtrs, 1, type,
             list, 1, ngroup, myrank, rootrank, &c_st);

    rc = ((rc == -1) ? DSRVR_ERROR : rc);

    SPTL_DEACTIVATE(DSRVR_GATHER_PORTAL);
    resetPtl(DSRVR_GATHER_PORTAL);

    if (rc == DSRVR_OK){   /* must synchronize before next global op */
	rc = dsrvr_barrier(tmout, list, listLen); 
    }

    return rc;
}
/*
** Broadcast.  Call dsrvr_new_coll_comm_buffer on the buffer 
** beforehand, and dsrvr_free_coll_comm_buffer when done with it.
*/
int
dsrvr_bcast(char *buf, int len, double tmout, int type, int *list, int listLen)
{
COLL_STRUCT c_st;
dsrvrInfo myInfo;
int rc, ngroup, myrank, rootrank, i;

    rootrank = 0;
    dsrvr_failInfo.dsrvrRoutine = DSRVR_BCAST;

    if (listLen && list){        /* global op on sub group only */
	ngroup = listLen;
	for (i=0; i<listLen; i++){
	    if (list[i] == dsrvrMyGroupRank){
		myrank = i;
		break;
	    }
	}
    }
    else{                      /* empty list means whole group */
        ngroup = dsrvrMembers;
        myrank = dsrvrMyGroupRank;
        list = NULL;
    }

    myInfo.ptl       = DSRVR_BROADCAST_PORTAL;
    myInfo.ping_ptl  = DSRVR_HANDSHAKE_PORTAL;
    myInfo.tmout     = tmout;

    IND_MD(DSRVR_BROADCAST_PORTAL).buf_desc_table->buf     = buf;
    IND_MD(DSRVR_BROADCAST_PORTAL).buf_desc_table->buf_len = len;

    c_st.cntr = 0;
    c_st.proto.p_init = dsrvr_puma_init;
    c_st.proto.p_vsend = dsrvr_puma_send;
    c_st.proto.p_vrecv = dsrvr_puma_recv;
    c_st.proto.p_cleanup = dsrvr_puma_no_cleanup;
    c_st.user_struct = (void *)&myInfo;

    SPTL_ACTIVATE(DSRVR_BROADCAST_PORTAL);

    dsrvr_failInfo.collRoutine = COLL_BCAST_SHORT;

    rc = _bcast_short(buf, len, type,
            list, 1, ngroup, myrank, rootrank, &c_st);
   
    rc = ((rc == -1) ? DSRVR_ERROR : rc);
     
    SPTL_DEACTIVATE(DSRVR_BROADCAST_PORTAL);
    resetPtl(DSRVR_BROADCAST_PORTAL);

    if (rc == DSRVR_OK){ /* must synchronize before next global op */
	rc = dsrvr_barrier(tmout, list, listLen);
    }

    return rc;
}
/*
** Data is combined using op with data fanned in from other nodes, 
** then fanned in.
**
**  op    a function that performs the pairwise reduction on all
**        data fanned in.  Result is propagated along.
**  data  buffer containing the operand. Call 
**        dsrvr_new_coll_comm_buffer on this buffer beforehand, 
**        and dsrvr_free_coll_comm_buffer when done with it.
**  len   length of buffer, which can't exceed the maximum reduction
**          buffer specified in dsrvr_comm_init
**  tmout in seconds for reduction operation
**  type  message type used to verify all members are doing the same
**        thing
*/

int
dsrvr_reduce(PUMA_OP op, char *data, int len, double tmout, int type, 
                int *list, int listLen)
{
dsrvrInfo myInfo;
COLL_STRUCT c_st;
int rc, ngroup, rootrank, myrank, i;

    if (len < (int) IND_MD(DSRVR_REDUCE_PORTAL).buf_desc_table[0].buf_len){
	CPerrno = EINVAL;
	return -1;
    }
    rootrank = 0;
    dsrvr_failInfo.dsrvrRoutine = DSRVR_REDUCE;

    if (listLen && list){        /* global op on sub group only */
	ngroup = listLen;
	for (i=0; i<listLen; i++){
	    if (list[i] == dsrvrMyGroupRank){
		myrank = i;
		break;
	    }
	}
    }
    else{                      /* empty list means whole group */
        ngroup = dsrvrMembers;
	myrank = dsrvrMyGroupRank;
        list = NULL;
    }

    myInfo.ptl       = DSRVR_REDUCE_PORTAL;
    myInfo.ping_ptl  = DSRVR_HANDSHAKE_PORTAL;
    myInfo.tmout     = tmout;

    SPTL_ACTIVATE(DSRVR_REDUCE_PORTAL);

    c_st.cntr = 0;
    c_st.proto.p_init = dsrvr_puma_init;
    c_st.proto.p_vsend = dsrvr_puma_send;
    c_st.proto.p_vrecv = dsrvr_puma_recv;
    c_st.proto.p_cleanup = dsrvr_puma_no_cleanup;
    c_st.user_struct = (void *)&myInfo;

    dsrvr_failInfo.collRoutine = COLL_REDUCE_SHORT;

    rc = _reduce_short(op, data, 1, len, type, 
              list, 1, ngroup, myrank, rootrank,
		  NULL, &c_st);
   
    rc = ((rc == -1) ? DSRVR_ERROR : rc);
     
    SPTL_DEACTIVATE(DSRVR_REDUCE_PORTAL);
    resetPtl(DSRVR_REDUCE_PORTAL);

    if (rc == DSRVR_OK){/* must synchronize before next global op */
	rc = dsrvr_barrier(tmout, list, listLen); 
    }

    return rc;
}
/*
********************************************************************************
** Our init, send and recv functions, invoked by the Puma collective ops on
** our behalf.
**
** These functions assume the portals are set up to go, and will be reset
** by caller afterward EXCEPT if handshaking first, the receive function 
** will reset the receive portal to the state of having received no messages
** before engaging in the handshake, relieving caller of the responsibility.
********************************************************************************
*/

int
dsrvr_puma_init(int gid ATTR_UNUSED, CHAMELEON mbits, char *inbuf, char *outbuf,
		 int data_size, int buf_len, void *ustruct,
		 VSENDRECV_HANDLE *handle)
{
    handle->mbits.ints.i0 = mbits.ints.i0;
    handle->mbits.ints.i1 = mbits.ints.i1;

    handle->in_buf = inbuf;
    handle->out_buf = outbuf;
    handle->data_size = data_size;
    handle->buf_len = buf_len;

    handle->user_struct = ustruct;

    return 0;
}

static int
send_data(int nid, int pid, int ptl, 
          DATA_REC *rec, VSENDRECV_HANDLE *handle, double tmout ATTR_UNUSED)
{
SEND_INFO_TYPE s_info;
int rc;
double t1;

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));
    s_info.dst_matchbits.ints.i0 = handle->mbits.ints.i0;
    s_info.dst_matchbits.ints.i1 = handle->mbits.ints.i1;
    s_info.dst_offset = rec->offset;

    t1 = dclock();

    rc = send_user_msg_phys( handle->out_buf + rec->offset, rec->len, 
                             nid, pid, ptl, &s_info);

    if (rc){
	CPerrno = EPORTAL;
	return DSRVR_RESOURCE_ERROR;
    }
    rc = srvr_await_send_completion(&(s_info.send_flag));

    if (rc){
	CPerrno = ESENDTIMEOUT;
	return DSRVR_RESOURCE_ERROR;
    }
    return 0;
}
int
dsrvr_puma_send(BOOLEAN flow_control ATTR_UNUSED, int nrecs, DATA_REC *recs,
		VSENDRECV_HANDLE *handle)
{
int handshake;
int i, rc, nsent, nid, pid, token;
int nids[NUM_DATA_RECS], pids[NUM_DATA_RECS], sent[NUM_DATA_RECS];
double t1;
dsrvrInfo *info;
IND_MD_BUF_DESC *buf_hdr;

    CPerrno = 0;

#ifdef DBGCOLLCOMM
    log_msg("dsrvr_puma_send to");
#endif

    /*
    for (i=0; i<nrecs;i++) printf("(%d) send %d -> %d offset %d, len %d\n",
	     _my_rank, recs[i].sndr_rank, recs[i].rcvr_rank,
	      recs[i].offset, recs[i].len );
	      */

    dsrvr_failInfo.what = WHAT_NOT_SET;
    dsrvr_failInfo.where = PUMA_SEND;
    dsrvr_failInfo.last_nid = -1;
    dsrvr_failInfo.last_pid = -1;
    dsrvr_failInfo.ptl = -1;

    info = (dsrvrInfo *)handle->user_struct;

    handshake = (info->ping_ptl != DSRVR_DONT_PING);

    for (i=0; i<nrecs; i++){

	nids[i] = memberNidByRank(recs[i].rcvr_rank);
	pids[i] = memberPidByRank(recs[i].rcvr_rank);
	sent[i] = 0;

#ifdef DBGCOLLCOMM
	log_msg("   rank %d nid %d pid %d",i,nids[i],pids[i]);
#endif

	if ( (nids[i] < 0) || (pids[i] < 0)){
	    CPerrno = EINVAL;
	    return DSRVR_ERROR;
	}
    }

    t1 = dclock();

    if (!handshake){

	dsrvr_failInfo.where = PUMA_SEND_DATA_BLIND;

	for (i=0; i<nrecs; i++){

	    dsrvr_failInfo.last_nid = nids[i];
	    dsrvr_failInfo.last_pid = pids[i];

#ifdef DBGCOLLCOMM
   	    log_msg("   --->>> nid %d pid %d",nids[i],pids[i]);
#endif

	    rc = send_data(nids[i], pids[i], info->ptl, recs+i, 
			   handle, info->tmout);

	    if (rc){
		return rc;
	    }
	}
	return 0;
    }

    nsent = 0;

    while (nsent < nrecs){

	if ((info->tmout > 0.0) && ((dclock() - t1) > info->tmout)){

	    for (i=0; i<nrecs; i++){
		if (!sent[i]){

		     dsrvr_failInfo.last_nid = nids[i];
		     dsrvr_failInfo.last_pid = pids[i];
		     dsrvr_failInfo.where = PUMA_SEND_AWAIT_HANDSHAKE;

		     memberTimedOutOnCollectiveOp(recs[i].rcvr_rank);
		}
	    }
	    CPerrno = ERECVTIMEOUT;
	    resetPtl(info->ping_ptl);
	    return DSRVR_EXTERNAL_ERROR;
	}
	dsrvr_failInfo.where = PUMA_SEND_AWAIT_HANDSHAKE;
	dsrvr_failInfo.ptl   = info->ping_ptl;

	rc = sptl_ind_msg_probe(&(IND_MD(info->ping_ptl)), 
				IND_CIRC_SV_HDR, 
				&buf_hdr, NONBLOCKING);
	if (rc){
	    CPerrno = EPORTAL;
	    return DSRVR_RESOURCE_ERROR;
	}

	if (buf_hdr){                 /* this receiver is ready */


	    dsrvr_failInfo.where = PUMA_SEND_DATA;
	    dsrvr_failInfo.ptl   = -1;

	    nid = buf_hdr->hdr.src_nid;
	    pid = buf_hdr->hdr.src_pid;
	    token = buf_hdr->hdr.dst_mbits.ints.i0;

#ifdef DBGCOLLCOMM
	    log_msg("    handshake from %d/%d",nid,pid);
#endif

	    for (i=0; i<nrecs; i++){

		if ( (nids[i] != nid) || (pids[i] != pid) || sent[i]){
		    continue;
		}
#ifdef DBGCOLLCOMM
	        log_msg("    rank %d",i);
#endif
		  
		if (token != DSRVR_OKTOGO){
		    dsrvr_failInfo.last_nid = nids[i];
		    dsrvr_failInfo.last_pid = pids[i];
		    dsrvr_failInfo.what = BAD_TOKEN;

		    memberBizarreOnCollectiveOp(recs[i].rcvr_rank);
		    resetPtl(info->ping_ptl);

		    return DSRVR_EXTERNAL_ERROR;
		}

		dsrvr_failInfo.last_nid = nids[i];
		dsrvr_failInfo.last_pid = pids[i];
		
#ifdef DBGCOLLCOMM
		log_msg("   ---> nid %d pid %d",nids[i],pids[i]);
#endif

		rc = send_data(nid, pid, info->ptl, recs+i, handle, info->tmout);

		if (rc){
		    return rc;
		}
		sent[i] = 1;
		nsent++;
		rc = sptl_ind_msg_free(&(IND_MD(info->ping_ptl)),
				    IND_CIRC_SV_HDR, buf_hdr);
		if (rc){
		    CPerrno = EPORTAL;
		    return DSRVR_RESOURCE_ERROR;
		}
		break;
	    }
	}
    }  /* end while */

    sptl_ind_rst_probe(&(IND_MD(info->ping_ptl)));

    return 0;
}
/*
** Receive from other members into either a single block portal or
** into independent block buffers.  
**
** In order to detect when a member fails to send to us within timeout, 
** the values sent to an entry within a single block must contain as least 
** one non-zero bit.  This is the price for fault detection and recovery.
** For an example of this, see the voting function in srvr_coll_util.c.
** (For independent block we can look at the header to tell if the message 
** came in.)
*/
static int
zero_data(char *buf, int offset, int len)
{
char *c, *last;

    c = buf + offset;
    last = c + len;

    while (c < last){
	if (*c++) return 0;
    }
    return 1;
}

/*
** reset memory descriptor to state where no messages have been rec'd
*/
void
resetPtl(int ptl)
{
int i, activePtl;

    if (PTL_DESC(ptl).stat_bits & PTL_ACTIVE){
        activePtl = 1;
        SPTL_DEACTIVATE(ptl);
    } else{
        activePtl = 0;
    }

    switch (PTL_DESC(ptl).mem_op)
    {
	case SINGLE_SNDR_OFF_SV_BDY:
	case SINGLE_SNDR_OFF_SV_BDY_ACK:
	case SINGLE_SNDR_OFF_RPLY:
	case SINGLE_RCVR_OFF_SV_BDY:
	case SINGLE_RCVR_OFF_SV_BDY_ACK:
	case SINGLE_RCVR_OFF_RPLY:

	    SING_MD(ptl).msg_cnt = 0;
	    SING_MD(ptl).rw_bytes = -1;

	    break;

        case IND_CIRC_SV_HDR:
	case IND_CIRC_SV_HDR_ACK:
	case IND_CIRC_SV_HDR_BDY:
	case IND_CIRC_SV_HDR_BDY_ACK :
	case IND_CIRC_RPLY:
        case IND_LIN_SV_HDR:
	case IND_LIN_SV_HDR_ACK:
	case IND_LIN_SV_HDR_BDY:
	case IND_LIN_SV_HDR_BDY_ACK :
	case IND_LIN_RPLY:

	    IND_MD(ptl).buf_desc_table[0].next_free = 0;
	    IND_MD(ptl).buf_desc_table[0].last_probe = -1;
	    IND_MD(ptl).buf_desc_table[0].first_read = 0;
	    for (i=0; i< IND_MD(ptl).num_buf_desc; i++){
		IND_MD(ptl).buf_desc_table[i].hdr.msg_len = -1;
		IND_MD(ptl).buf_desc_table[i].stage = 0;
	    }


	    break;
    }

    if (activePtl){
        SPTL_ACTIVATE(ptl);
    }
}
/*
** Use of portals on linux requires that receive buffers be locked down.
** If this library is reimplemented over some other transport, then if
** there's some buffer registry/de-registry required do it here.
*/
void
dsrvr_new_coll_comm_buffer(char *buf, int len)
{
    portal_lock_buffer(buf, len);
}
void
dsrvr_free_coll_comm_buffer(char *buf, int len)
{
    portal_unlock_buffer(buf, len);
}

int
dsrvr_puma_recv(PUMA_OP op, BOOLEAN flow_control ATTR_UNUSED, int nrecs,
                DATA_REC *recs, VSENDRECV_HANDLE *handle)
{
int i, rc, tbytes;
int handshake, recd, len;
int single_blk, ind_lin, ind_circ;
int nids[NUM_DATA_RECS], pids[NUM_DATA_RECS];
IND_MD_BUF_DESC *gotIt[NUM_DATA_RECS];
int nid, pid, colltype, usetype;
dsrvrInfo *info;
MEM_OP_TYPE ptlType;
SEND_INFO_TYPE s_info;
IND_MD_BUF_DESC *buf_hdr;
double t1;
char *inBuf;

    CPerrno = 0;

#ifdef DBGCOLLCOMM
    log_msg("dsrvr_puma_recv from");
#endif

    dsrvr_failInfo.what = WHAT_NOT_SET;
    dsrvr_failInfo.where = PUMA_RECV;
    dsrvr_failInfo.last_nid = -1;
    dsrvr_failInfo.last_pid = -1;
    dsrvr_failInfo.ptl = -1;

    /*
    for (i=0; i<nrecs;i++) printf("(%d) recv %d -> %d  offset %d len %d\n",
	     _my_rank, recs[i].sndr_rank, recs[i].rcvr_rank,
	      recs[i].offset, recs[i].len );
	      */

    if ((nrecs < 0) || (nrecs > dsrvrMembers)){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }
    if (nrecs == 0){
	return DSRVR_OK;
    }
    info = (dsrvrInfo *)handle->user_struct;

    handshake = (info->ping_ptl != DSRVR_DONT_PING);

    ptlType = PTL_DESC(info->ptl).mem_op;

    single_blk = ind_lin = ind_circ = 0;

    if (IS_SINGLE_MD_OP(ptlType)){
	single_blk = 1;
    } else if (IS_IND_BLK_LIN_OP(ptlType)){
	ind_lin = 1;
    } else if (IS_IND_BLK_CIRC_OP(ptlType)){
	ind_circ = 1;
    }
    else{
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }
    tbytes = 0;

    for (i=0; i<nrecs; i++){
	tbytes += recs[i].len;

	nids[i] = memberNidByRank(recs[i].sndr_rank);
	pids[i] = memberPidByRank(recs[i].sndr_rank);
	gotIt[i] = NULL;

#ifdef DBGCOLLCOMM
        log_msg("    rank %d nid %d pid %d",i,nids[i],pids[i]);
#endif

	if ( (nids[i] < 0) || (pids[i] < 0)){
	    CPerrno = EINVAL;
	    return DSRVR_ERROR;
	}
    }
    t1 = dclock();
    
    if (handshake){
	/*
	** reset portal to receive data
	*/
	resetPtl(info->ptl);

	if (single_blk){
            /*
	    ** zero the portion of the single block where we expect data
	    */
	    for (i=0; i<nrecs; i++){
                memset(SING_MD(info->ptl).buf + recs[i].offset, 0, recs[i].len);
	    }
	}

	/*
	** send okToGo to senders
	*/
	dsrvr_failInfo.where = PUMA_RECV_SEND_OK;

        for (i=0; i<nrecs; i++){

            memset(&s_info, 0, sizeof(SEND_INFO_TYPE));
	    s_info.dst_matchbits.ints.i0 = DSRVR_OKTOGO;

            dsrvr_failInfo.last_nid = nids[i];
            dsrvr_failInfo.last_pid = pids[i];

#ifdef DBGCOLLCOMM
	    log_msg("    send DSRVR_OKTOGO to %d/%d",nids[i],pids[i]);
#endif

	    rc = send_user_msg_phys(NULL, 0,
		   nids[i], pids[i], info->ping_ptl, &s_info);

            if (rc){
		return DSRVR_RESOURCE_ERROR;
	    }
	    rc = srvr_await_send_completion(&(s_info.send_flag));

	    if (rc){
		CPerrno = ESENDTIMEOUT;
		return DSRVR_RESOURCE_ERROR;
	    }
	}
    }

    /*
    ** await data - watch timeout
    */
    if (single_blk){

	dsrvr_failInfo.where = PUMA_RECV_SINGLE_BLK;
	dsrvr_failInfo.ptl = info->ptl;

#ifdef DBGCOLLCOMM
	log_msg("    await %d bytes in single block",tbytes);
#endif

	while(SING_MD(info->ptl).rw_bytes < tbytes){
	    if ((info->tmout > 0.0) && ((dclock() - t1) > info->tmout)){

		for (i=0; i<nrecs; i++){
		    if (zero_data(SING_MD(info->ptl).buf, 
				  recs[i].offset, recs[i].len)){

			 dsrvr_failInfo.last_nid = nids[i];
			 dsrvr_failInfo.last_pid = pids[i];

			 memberTimedOutOnCollectiveOp(recs[i].sndr_rank);
			 
		     }
		}
		CPerrno = ERECVTIMEOUT;
		return DSRVR_EXTERNAL_ERROR;
	    }
	}
    }
    else if (ind_lin || ind_circ){

	recd = 0;

	dsrvr_failInfo.where = PUMA_RECV_IND_BLK;
	dsrvr_failInfo.ptl = info->ptl;

	while (recd < nrecs){
	    if ((info->tmout > 0.0) && (dclock() - t1  >  info->tmout)){
		 for (i=0; i<nrecs; i++){
		     if (!gotIt[i]){

			 dsrvr_failInfo.last_nid = nids[i];
			 dsrvr_failInfo.last_pid = pids[i];

			 memberTimedOutOnCollectiveOp(recs[i].sndr_rank);
		     }
		 }
		 CPerrno = ERECVTIMEOUT;
		 resetPtl(info->ptl);
	         return DSRVR_EXTERNAL_ERROR;	
	    }

	    rc = sptl_ind_msg_probe(&(IND_MD(info->ptl)), ptlType, 
				    &buf_hdr, NONBLOCKING);
            if (rc){
		return DSRVR_RESOURCE_ERROR;
	    }
	    if (buf_hdr){


		nid = buf_hdr->hdr.src_nid;
		pid = buf_hdr->hdr.src_pid;
		colltype = buf_hdr->hdr.dst_mbits.ints.i0;
		usetype = buf_hdr->hdr.dst_mbits.ints.i1;

#ifdef DBGCOLLCOMM
 	        log_msg("    got data from %d/%d",nid,pid);
#endif

		for (i=0; i<nrecs; i++){
		    if ((nids[i] == nid) && (pids[i] == pid)){

                        if (((int) handle->mbits.ints.i0 != colltype) ||
			    ((int) handle->mbits.ints.i1 != usetype)  ||
			    (gotIt[i])                             ){

			    break;
                        }

#ifdef DBGCOLLCOMM
			log_msg("             rank %d",i);
#endif

			gotIt[i] = buf_hdr;

			recd++;
			break;
		    }
		}
	    }
	}
    }

    /*
    ** if op, then perform op on received data and my data
    */
    if (op){

	for (i=0; i<nrecs; i++){
	    if (single_blk){
		inBuf = SING_MD(info->ptl).buf + recs[i].offset;
		len = recs[i].len;
	    }
	    else{
		inBuf = gotIt[i]->buf;
		len   = gotIt[i]->hdr.msg_len;
	    }
	    op(inBuf, handle->out_buf, &len, NULL);
	}
    }
    /*
    ** For independent block portals, free the messages we consumed,
    ** and reset the probe back to the beginning.
    */
    if (IS_IND_MD_OP(ptlType)){
	for (i=0; i<nrecs; i++){

	    sptl_ind_msg_free(&(IND_MD(info->ptl)), ptlType, gotIt[i]);
	}
	sptl_ind_rst_probe(&(IND_MD(info->ptl)));
    }

    return 0;
}
int
dsrvr_puma_no_cleanup(VSENDRECV_HANDLE *handle ATTR_UNUSED)
{
    return 0;
}
int
dsrvr_puma_cleanup(VSENDRECV_HANDLE *handle)
{
dsrvrInfo *info;
 
    info = (dsrvrInfo *)handle->user_struct;
 
    switch (info->ptl)
    {
        case DSRVR_BARRIER_PORTAL:
        case DSRVR_HANDSHAKE_PORTAL:
        case DSRVR_GATHER_PORTAL:
        case DSRVR_REDUCE_PORTAL:
        case DSRVR_BROADCAST_PORTAL:
            break;
    }

    return 0;
}
 
/***********************************************************************
** initialize comm structures
************************************************************************/
static int init_synchronization_portal(int ptl, int nbufs);

/*
** These two portals are always active, so it's possible messages
** arrived after we last left a group.  Clean them up here before
** forming a new group.
*/
void
dsrvr_comm_reset(void)
{
    resetPtl(DSRVR_HANDSHAKE_PORTAL);
    resetPtl(DSRVR_BARRIER_PORTAL);
}
/*
** nmembers:  maximum number of members in the group
** maxRedBuf: max size of buffer from each sender which will be OP'd 
**            in a reduce
*/
int
dsrvr_comm_init(int nmembers, int maxRedBuf)
{
char *vbuf;
IND_MD_BUF_DESC *table;
int i, rc, len, maxSenders;

    if ((nmembers <= 0) || (maxRedBuf < 0)) {
        CPerrno = EINVAL;
        return -1;
    }
    maxSenders = _ceillogp(nmembers);

    if (maxSenders == 0) maxSenders = 1;

    dsrvr_gatherPtrs = (char **)malloc(sizeof(char *) * (nmembers+1));
    if (dsrvr_gatherPtrs == NULL){
	CPerrno = ENOMEM;
	return -1;
    }
 
    /**********************************************************************
    ** Portals for BROADCAST, REDUCE and GATHER operations.  These are
    ** activated as needed and deactivated when done.
    ***********************************************************************
    */
    rc = sptl_init(DSRVR_GATHER_PORTAL);

    if (rc){
	CPerrno = EPORTAL;
	return -1;
    }

    PTL_DESC(DSRVR_GATHER_PORTAL).mem_op = SINGLE_SNDR_OFF_SV_BDY;

    SING_MD(DSRVR_GATHER_PORTAL).msg_cnt  = 0;
    SING_MD(DSRVR_GATHER_PORTAL).rw_bytes = -1;
    SING_MD(DSRVR_GATHER_PORTAL).buf      = NULL;
    SING_MD(DSRVR_GATHER_PORTAL).buf_len  = 0;
    
    rc = sptl_init(DSRVR_REDUCE_PORTAL);
 
    if (rc){
	CPerrno = EPORTAL;
        return -1;
    }
    PTL_DESC(DSRVR_REDUCE_PORTAL).mem_op = IND_LIN_SV_HDR_BDY;

    len = maxSenders * sizeof(IND_MD_BUF_DESC);

    table = (IND_MD_BUF_DESC *)malloc(len);
 
    if (!table){
        CPerrno = ENOMEM;
        return -1;
    }
 
    if (portal_lock_buffer(table, len) == -1) {
        CPerrno = ELOCK;
        return( -1 );
    }

    len = maxSenders * maxRedBuf;

    vbuf = (char *)malloc(len);
 
    if (!vbuf){
        CPerrno = ENOMEM;
        return -1;
    }
 
    if (portal_lock_buffer(vbuf, len) == -1) {
        CPerrno = ELOCK;
        return( -1 );
    }

    for (i=0; i<maxSenders; i++){
         table[i].stage = 0;
         table[i].hdr.msg_len = -1;
         table[i].buf = vbuf + (i * maxRedBuf);
         table[i].buf_len = maxRedBuf;
    }

    table[0].first_read = 0;
    table[0].last_probe = -1;
    table[0].next_free  = 0;
    table[0].ref_count  = 1;

    IND_MD(DSRVR_REDUCE_PORTAL).num_buf_desc = maxSenders;
    IND_MD(DSRVR_REDUCE_PORTAL).buf_desc_table = table;

    rc = sptl_init(DSRVR_BROADCAST_PORTAL);
 
    if (rc){
        CPerrno = EPORTAL; 
        return -1;
    }

    table = (IND_MD_BUF_DESC *)malloc(sizeof(IND_MD_BUF_DESC));
 
    if (!table){
        CPerrno = ENOMEM;
        return -1;
    }
 
    if (portal_lock_buffer(table, sizeof(IND_MD_BUF_DESC)) == -1) {
        free(table);
        CPerrno = ELOCK;
        return( -1 );
    }

    table->stage = 0;
    table->hdr.msg_len = -1;
    table->buf = NULL;
    table->buf_len = 0;
 
    table->first_read = 0;
    table->last_probe = -1;
    table->next_free  = 0;
    table->ref_count  = 1;
 
    PTL_DESC(DSRVR_BROADCAST_PORTAL).mem_op = IND_LIN_SV_HDR_BDY;

    IND_MD(DSRVR_BROADCAST_PORTAL).num_buf_desc = 1;
    IND_MD(DSRVR_BROADCAST_PORTAL).buf_desc_table = table;

    /*************************************************************
    ** Synchronization portals.  These are circular independent
    ** block, save headers only, activated now and not deactivated.
    **************************************************************/
    
    rc = init_synchronization_portal(DSRVR_BARRIER_PORTAL, 2*maxSenders);
    if (rc){
	return -1;
    }

    rc = init_synchronization_portal(DSRVR_HANDSHAKE_PORTAL, 2*maxSenders);
    if (rc){
	return -1;
    }

    /*
    ** Initialization required by puma library functions:
    **     _bcast_short()
    **     _gather()
    **     _reduce_short()
    */

    g_recs = (DATA_REC *)malloc(NUM_DATA_RECS * sizeof(DATA_REC));
    
    return 0;
}
static int
init_synchronization_portal(int ptl, int nbufs)
{
int len, rc, i;
IND_MD_BUF_DESC *table;

    len = nbufs * sizeof(IND_MD_BUF_DESC);

    table = (IND_MD_BUF_DESC *)malloc(len);
 
    if (!table){
        CPerrno = ENOMEM;
        return -1;
    }
 
    if (portal_lock_buffer(table, len) == -1) {
        free(table);
        CPerrno = ELOCK;
        return( -1 );
    }

    for (i=0; i<nbufs; i++){
         table[i].stage = 0;
         table[i].hdr.msg_len = -1;
         table[i].buf = NULL;
         table[i].buf_len = 0;
    }
 
    table[0].first_read = 0;
    table[0].last_probe = -1;
    table[0].next_free  = 0;
    table[0].ref_count  = 1;
 
    rc = sptl_init(ptl);
 
    if (rc){
	portal_unlock_buffer(table, len);
        free(table);
        return -1;
    }
    PTL_DESC(ptl).mem_op = IND_CIRC_SV_HDR;

    IND_MD(ptl).num_buf_desc = nbufs;
    IND_MD(ptl).buf_desc_table = table;

    SPTL_ACTIVATE(ptl);

    return 0;
}

