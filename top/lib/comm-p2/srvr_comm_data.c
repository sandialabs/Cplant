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
**  $Id: srvr_comm_data.c,v 1.1 2000/11/04 03:56:31 lafisk Exp $
*/

#include "puma.h"
//#include "portal.h"
#include "sysptl.h"
#include "srvr_comm.h"
//#include "errorUtil.h"

//#include "srvr_comm_data.h"
#include "srvr_coll_util.h"

#include <malloc.h>
#if !defined(__GLIBC__) || (__GLIBC__ < 2) \
  || (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0)
typedef long int intptr_t;
#else
#include <stdint.h>
#endif

INT32 SrvrDbg = 0;
 
void display_match_list(int ptl);
void display_sing_mem_desc(SINGLE_MD_TYPE *smd);


/***********************************************************
** A match list portal is required to set up read memory and
** write memory operations specified in the control messages.
** A match list entry points to the buffer to be read by the
** remote process, or to the buffer to be written to by the
** remote process.
************************************************************
*/
int
srvr_init_data_ptl(INT32 max_num_bufs)
{
PORTAL_INDEX ptl;
INT32 rc, tmsgs, i;
VOID *ml_buf;

    tmsgs = max_num_bufs + 1;

    /*
    ** allocate a new portal
    */

    rc = sptl_l_alloc(&ptl);

    if (rc != ESUCCESS){
        return SRVR_INVAL_PTL;
    } 

    rc = sptl_init(ptl);

    if (rc != ESUCCESS){
        sptl_dealloc(ptl);
        return SRVR_INVAL_PTL;
    } 

    PTL_DESC(ptl).mem_op = MATCHING;

    ml_buf = (VOID *)malloc(tmsgs * sizeof(MATCH_DESC_TYPE));

    if (!ml_buf){          /* buffer gets locked in sptl_ml_init */
        CPerrno = ENOMEM;
        sptl_dealloc(ptl);
        return SRVR_INVAL_PTL;
    }

    rc = sptl_ml_init(&(MATCH_MD(ptl)), ml_buf, tmsgs);

    if (rc != ESUCCESS){
        sptl_dealloc(ptl);
        free(ml_buf);
        return SRVR_INVAL_PTL;
    }
    /*
    ** careful: sptl_ml_init clears ctl_bits
    */
    for (i=1; i<tmsgs; i++) {
        MATCH_MD(ptl).u_lst[i].ctl_bits |= MCH_NOT_ACTIVE;
    }

    SPTL_ACTIVATE(ptl);

    return (int)ptl;
}
INT32
srvr_release_data_ptl(int __ptl)
{
PORTAL_INDEX ptl=(PORTAL_INDEX)__ptl;
int rc;

    if ((ptl > MAX_PORTAL) ||
        (PTL_DESC(ptl).mem_op != MATCHING)){

        CPerrno = EINVAL;
        return -1;
    }

    /*
    ** we may be exiting because of a bad portal, don't
    ** call quiescence or we may never get out of here.

    sptl_quiescence(ptl);
    */

    sptl_dealloc(ptl);

    rc = portal_unlock_buffer(MATCH_MD(ptl).u_lst, 
                         MATCH_MD(ptl).list_len * sizeof(MATCH_DESC_TYPE));

    if (rc){
        CPerrno = EUNLOCK;
        return -1;
    }

    free(MATCH_MD(ptl).u_lst);

    return 0;
}
/*
** return match list index of new entry, -1 on error
*/
INT32
srvr_add_data_buf(VOID *buf, INT32 len, INT32 type, int __ptl,
             unsigned long __ign_mbits, unsigned long __must_mbits)
{
PORTAL_INDEX ptl=(PORTAL_INDEX)__ptl;
INT32 mle, first_mle, offset, rc;
MATCH_LIST_TYPE *ml_hdr;
MATCH_DESC_TYPE *top_desc;
SINGLE_MD_TYPE  *s_desc;
CHAMELEON ign_mbits, must_mbits;

    MKCHAMELEON (ign_mbits, __ign_mbits);
    MKCHAMELEON (must_mbits, __must_mbits);
 
    if (SrvrDbg){
        printf("srvr_add_data_buf: buf %p, len %d, type %d, ptl %d\n",
                      buf, len, type, ptl);
        printf("\tignore 0x%08x 0x%08x match 0x%08x 0x%08x\n",
            ign_mbits.ints.i0, ign_mbits.ints.i1,
            must_mbits.ints.i0, must_mbits.ints.i1);
    }

    if ( (ptl > MAX_PORTAL) || (PTL_DESC(ptl).mem_op != MATCHING)){
        CPerrno = EINVAL;
        return -1;
    }

    ml_hdr = &(MATCH_MD(ptl));

    /*
    ** find a free mle
    */
    mle = ml_hdr->list_len;

    while (--mle > 0){

        if (MLE_INACTIVE(ptl, mle)){
            break;
        }
    }
        
    if (mle == 0){   /* we're out of match list entries */
        CPerrno = EMLSTFULL;
        return -1;
    } 

    MLE(ptl, mle).rank          = ANY_RANK;    /* matching info */
    MLE(ptl, mle).gid           = ANY_GID;
    MLE(ptl, mle).ign_mbits.ll  = ign_mbits.ll;
    MLE(ptl, mle).must_mbits.ll = must_mbits.ll;
    
    if (type == READ_BUFFER){                  /* memory descriptor */
        MLE(ptl, mle).mem_op = SINGLE_SNDR_OFF_RPLY;
    }
    else if (type == WRITE_BUFFER){
        MLE(ptl, mle).mem_op = SINGLE_SNDR_OFF_SV_BDY;
    }
    
    s_desc = &(MLE(ptl, mle).mem_desc.single);

    s_desc->msg_cnt  = 0;
    s_desc->buf      = buf;
    s_desc->buf_len  = len;
    s_desc->rw_bytes = -1;

    offset = (intptr_t) buf & 0x07;   /* lock an aligned buffer */

    if (len > 0){
	rc = portal_lock_buffer(((char *)buf - offset), len + offset);

	if (rc == -1){
	    CPerrno = ELOCK;
	    return -1;
	}
    }

    top_desc = &(MLE(ptl, 0));               /* match list links */
    first_mle = top_desc->next;

    if (first_mle == 0){   /* this is the only entry */

        NEXT_MLE(ptl, mle) = 0;
        PREV_MLE(ptl, mle) = 0;

    }
    else{                  /* place new entry in front of first entry */

        NEXT_MLE(ptl, mle) = first_mle;
        PREV_MLE(ptl, mle) = 0;

        PREV_MLE(ptl, first_mle) = mle;
    }
    ml_hdr->hop_cnt++;

    top_desc->next = mle;   /* kernel will skip it until we activate */

    ACTIVATE_MLE(ptl, mle);
 
    if (SrvrDbg)
       display_match_list(ptl);

    return mle;
}
/*
** Check if data has come into the write memory buffer
**
**    1  if len bytes have arrived
**    0  if len bytes have NOT arrived 
**   -1  some error
*/
INT32
srvr_test_write_buf(int __ptl, INT32 mle)
{
PORTAL_INDEX ptl=(PORTAL_INDEX)__ptl;
SINGLE_MD_TYPE *smd;

    if ( (ptl > MAX_PORTAL) || 
         (PTL_DESC(ptl).mem_op != MATCHING) ||
         (mle <= 0) ||
         (mle >= (INT32) MATCH_MD(ptl).list_len) ||
         (MLE_INACTIVE(ptl, mle))        ||
         (MLE(ptl, mle).mem_op != SINGLE_SNDR_OFF_SV_BDY) ){

         CPerrno = EINVAL;
         return -1;
    }

    smd = &(MLE(ptl, mle).mem_desc.single);

    if (SrvrDbg){
        printf("\nsrvr_test_write_buf ptl %d mle %d\n",ptl,mle);
        display_sing_mem_desc(smd);
    }

    if (((INT32)smd->buf_len == 0) || (smd->rw_bytes >= (INT32)smd->buf_len)){
        return 1;
    }

    return 0;
}
/*
** test if "count" accesses have been made to the read data buffer
** by remote processes
**
**    1  at least "count" accesses have been made, and the data has been
**         sent out of the buffer and the buffer can safely be reused.
**    0  fewer than "count" accesses have been made, or "count" accesses
**         have been made but we can't tell if the data is yet done 
**         being sent out (because there is activity on the portal).
**   -1  some error
*/
INT32
srvr_test_read_buf(int __ptl, INT32 mle, INT32 count)
{
PORTAL_INDEX ptl=(PORTAL_INDEX)__ptl;
SINGLE_MD_TYPE *smd;

    if ( (ptl > MAX_PORTAL) || 
         (PTL_DESC(ptl).mem_op != MATCHING) ||
         (mle <= 0) ||
         (mle >= (INT32) MATCH_MD(ptl).list_len) ||
         (MLE_INACTIVE(ptl, mle))        ||
         (MLE(ptl, mle).mem_op != SINGLE_SNDR_OFF_RPLY) ){

         CPerrno = EINVAL;
         return -1;
    }

    smd = &(MLE(ptl, mle).mem_desc.single);

    if (smd->buf_len == 0){
        return 1;
    }
    if ((smd->msg_cnt >= count) && (PTL_DESC(ptl).active_cnt == 0)){
        return 1;
    }

    return 0;
}
INT32
srvr_delete_buf(int __ptl, INT32 mle)
{
PORTAL_INDEX ptl=(PORTAL_INDEX)__ptl;
SINGLE_MD_TYPE *s_desc;

    if (SrvrDbg){
        printf("srvr_delete_buf, ptl %d, mle %d\n",ptl,mle);
    }
 
    if ((ptl > MAX_PORTAL) ||
        (mle <= 0)  || (mle >= (INT32) MATCH_MD(ptl).list_len)){

       CPerrno = EINVAL;
       return -1;
    }

    if (MLE_INACTIVE(ptl, mle)){   /* it's already gone */
        return 0;
    }

    MLE(ptl, mle).ctl_bits = MCH_NOT_ACTIVE;   /* deactivate it */

    NEXT_MLE(ptl, (PREV_MLE(ptl,mle)))   /* fix up linked list of entries */
           = NEXT_MLE(ptl, mle);

    if (NEXT_MLE(ptl, mle) != 0){

        PREV_MLE(ptl, (NEXT_MLE(ptl, mle))) = PREV_MLE(ptl, mle);
    }

    MATCH_MD(ptl).hop_cnt--;

    s_desc = &(MLE(ptl, mle).mem_desc.single);

    portal_unlock_buffer(s_desc->buf, s_desc->buf_len);

    if (SrvrDbg)
       display_match_list(ptl);
 
    return 0;
}
static const char *mem_op_strings[30] = {
"SINGLE_SNDR_OFF_SV_BDY = 0",
"SINGLE_SNDR_OFF_SV_BDY_ACK",
"SINGLE_SNDR_OFF_RPLY",
"SINGLE_RCVR_OFF_SV_BDY",
"SINGLE_RCVR_OFF_SV_BDY_ACK",
"SINGLE_RCVR_OFF_RPLY",
"IND_CIRC_SV_HDR",
"IND_CIRC_SV_HDR_ACK",
"IND_CIRC_SV_HDR_BDY",
"IND_CIRC_SV_HDR_BDY_ACK",
"IND_CIRC_RPLY",
"IND_LIN_SV_HDR",
"IND_LIN_SV_HDR_ACK",
"IND_LIN_SV_HDR_BDY",
"IND_LIN_SV_HDR_BDY_ACK",
"IND_LIN_RPLY",
"COMB_SNDR_OFF_SV_BDY",
"COMB_SNDR_OFF_SV_BDY_ACK",
"COMB_SNDR_OFF_RPLY",
"COMB_RCVR_OFF_SV_BDY",
"COMB_RCVR_OFF_SV_BDY_ACK",
"COMB_RCVR_OFF_RPLY",
"DYN_SV_HDR",
"DYN_SV_HDR_ACK",
"DYN_SV_HDR_BDY",
"DYN_SV_HDR_BDY_ACK",
"MATCHING",
"UNASSIGNED_MD"
};
void
display_match_list(int ind)
{
MATCH_LIST_TYPE *mmd_hdr;
MATCH_DESC_TYPE *mmd;
PORTAL_DESCRIPTOR *ptl;
SINGLE_MD_TYPE *smd;
int i;
 
    ptl = &(PTL_DESC(ind));

    if (ptl->mem_op != MATCHING){
        printf("display_match_list: not a match list portal\n");
    }
 
    mmd_hdr = &(ptl->mem_desc.match);
 
    printf("match list top: list %p, list_len %d, hop_cnt %d\n",
        mmd_hdr->u_lst, mmd_hdr->list_len, mmd_hdr->hop_cnt);
 
    for (i = 0, mmd = mmd_hdr->u_lst; i < (int) mmd_hdr->list_len; i++, mmd++){
 
        printf("\t%03d) rank %d  gid %d\n",
                      i, (INT32)mmd->rank, (INT32)mmd->gid);
 
        printf("\t\tignore 0x%08x 0x%08x  match 0x%08x 0x%08x\n",
            mmd->ign_mbits.ints.i0, mmd->ign_mbits.ints.i1,
            mmd->must_mbits.ints.i0, mmd->must_mbits.ints.i1);
 
        printf("\t\tnext %d  next_on_nobuf %d  next_on_nofit %d  pad %d\n",
            mmd->next, mmd->next_on_nobuf, mmd->next_on_nofit, mmd->pad16[0]);
 
        printf("\t\tmem_op %d  control bits 0x%02x\n",
             mmd->mem_op, mmd->ctl_bits);

        if (i){
            if (IS_SINGLE_MD_OP(mmd->mem_op)){
                printf("\t\t%s\n",mem_op_strings[mmd->mem_op]);
                smd = &(mmd->mem_desc.single);
                display_sing_mem_desc(smd);
            }
            else{
                printf("\t\tmemory descriptor not single block\n");
            }
        }
    }

    printf("\n");
}
void
display_sing_mem_desc(SINGLE_MD_TYPE *smd)
{
    printf("\t\tmsg_cnt %d, buf %p, len %d, rw_bytes %d\n",
       smd->msg_cnt, smd->buf, smd->buf_len, smd->rw_bytes);
}
