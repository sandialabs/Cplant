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
$Id: vcoll.c,v 1.8 2001/02/16 05:39:20 lafisk Exp $
*/

#include <stdio.h>
#include <string.h>
#include "puma.h"
#include "puma_errno.h"
#include "malloc.h"

#include "collcore.h"

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

/*****************************************************************************/
/*****************************************************************************/
/* These collective communication support functions do not compile.             */
/* They use some structures from Portals 2.  But we'll keep them around because */
/* they show how the functions in collcore.c can be implemented.             */
/*****************************************************************************/
/*****************************************************************************/

#if 0
/*****************************************************************************/

#undef VDEBUG

#define PROTOCOL_THRESHOLD        (1024)

#define PTL( ptl )                (_my_pcb->portal_table2[(ptl)])
#define IND_MD( ptl )                (_my_pcb->portal_table2[(ptl)].mem_desc.ind)
#define SINGLE_MD( ptl )        (_my_pcb->portal_table2[(ptl)].mem_desc.single)
#define ML_MD( ptl )                (_my_pcb->portal_table2[(ptl)].mem_desc.match)
#define MLE( ptl,mle )                (_my_pcb->portal_table2[(ptl)].mem_desc.match.u_lst[(mle)])
#define MLE_DYN_MD(ml_ptr,mle)        ((ml_ptr)->u_lst[(mle)].mem_desc.dyn)
#define MLE_SINGLE_MD(ml_ptr,mle)        ((ml_ptr)->u_lst[(mle)].mem_desc.single)


/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  Collective Protocol Dependent Layer
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

/*---------------------------------------------------------------------------*/
/*
**==================================
** Portal Collective Initialization
**==================================
*/

INT32
_puma_collective_init(VOID)
{
INT32   i,rc;
MATCH_LIST_TYPE *ml;
CHAMELEON mbits_ignore_all = IGNORE_ALL();
CHAMELEON mbits_match_all = MATCH_ALL();

    /*
    ** Initialize hdr link structure
    */
    HDR_LINK_INIT();

    /*
    ** Allocate the Collective Portals
    */
    rc = sptl_c_alloc( &Coll_Heap_Ptl,NULL,_my_nnodes,_my_rank );
    if (!rc) rc = sptl_c_alloc( &Coll_Bucket_Ptl,NULL,_my_nnodes,_my_rank );
    if (!rc) rc = sptl_c_alloc( &Coll_Hdr_Ptl,NULL,_my_nnodes,_my_rank );

    if (rc) {
        coll_assert(rc,"_puma_collective_init", 
            "_puma_collective_init() detected error");
    }
    if ( (Coll_Heap_Ptl>=NUM_PORTALS) ||
         (Coll_Bucket_Ptl>=NUM_PORTALS) ||
         (Coll_Hdr_Ptl>=NUM_PORTALS) ) {
        ERRNO = ENOCPTLS;
        coll_assert(-1,"_puma_collective_init()",
            "Failed to allocate portals for collective ops");
    }
    /*
    printf("%d: Coll: heap %d bucket %d hdr %d\n",
        _my_rank, Coll_Heap_Ptl, Coll_Bucket_Ptl, Coll_Hdr_Ptl);
    fflush(stdout);
    */


    /*
    ** Malloc memory
    */
    req_bucket_desc = (IND_MD_BUF_DESC *)
        malloc( 2 * _my_nnodes * sizeof(IND_MD_BUF_DESC) );
    tree_ml = (MATCH_DESC_TYPE *) 
        malloc( TREE_HEAP_ENTRIES * sizeof(MATCH_DESC_TYPE) );
    tree_heap_buf = (DYN_MALLOC_LINK_TYPE *) 
        malloc( TREE_HEAP_SIZE );
    g_recs = (DATA_REC *) malloc( NUM_DATA_RECS * sizeof(DATA_REC) );
    if ( (req_bucket_desc == NULL) ||
         (tree_ml == NULL) ||
         (tree_heap_buf == NULL) ||
         (g_recs == NULL) ) {
        ERRNO = ENOMEM;
        coll_assert(-1, "_puma_collective_init()",
            "Malloc failed in _puma_collective_init()");
    }

    /*
    ** Initialize the portals
    ** - This will not activate them yet.
    */
    rc = sptl_init( Coll_Heap_Ptl );
    if (!rc) { rc  = sptl_init( Coll_Hdr_Ptl ); }
    if (!rc) { rc = sptl_init( Coll_Bucket_Ptl ); }
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init portals in _puma_collective_init()");

    /*
    ** Initialize match list
    ** Used by  _bcast_short(), _gather(), _scatter(), _reduce()
    */
    ml = &ML_MD(Coll_Heap_Ptl);
    rc = sptl_ml_init( ml, (VOID *)tree_ml, TREE_HEAP_ENTRIES );
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init match list in _puma_collective_init()");

    /*
    ** Initialize match list entries
    ** Used by  _bcast_short(), _gather(), _scatter(), _reduce()
    */
    rc = sptl_mle_init( ml, GROUP, -1, mbits_ignore_all, 
        mbits_match_all, heap_mle );
    if (!rc) { rc = sptl_mle_init( ml, GROUP, -1, mbits_match_all, 
        mbits_match_all, single_mle ); }
    if (!rc) { rc = sptl_mle_init( ml, GROUP, -1, mbits_match_all, 
        mbits_match_all, reply_mle ); }
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init dyn mle in _puma_collective_init()");
    SPTL_MCH_ACTIVATE( Coll_Heap_Ptl,heap_mle );
    SPTL_MCH_DEACTIVATE( Coll_Heap_Ptl,single_mle );
    SPTL_MCH_DEACTIVATE( Coll_Heap_Ptl,reply_mle );

    /*
    ** Initialize the single block mle's
    ** Used by  _bcast_short(), _gather(), _scatter(), _reduce()
    */
    MLE( Coll_Heap_Ptl,single_mle).mem_op = SINGLE_SNDR_OFF_SV_BDY;
    MLE( Coll_Heap_Ptl,reply_mle).mem_op = SINGLE_SNDR_OFF_RPLY;
    rc = sptl_single_init( &MLE_SINGLE_MD( ml,single_mle ), NULL, 0 ); 
    if (!rc) { 
        rc = sptl_single_init( &MLE_SINGLE_MD( ml,reply_mle ), NULL, 0 ); 
    }
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init single mle's in _puma_collective_init()");

    /*
    ** Initialize the dynamic heap mle
    ** Used by  _bcast_short(), _gather(), _scatter(), _reduce()
    */
    MLE( Coll_Heap_Ptl,heap_mle).mem_op = DYN_SV_HDR_BDY;
    rc = sptl_dyn_init( &MLE_DYN_MD( ml,heap_mle ),
        tree_heap_buf, TREE_HEAP_SIZE );
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init dyn heap in _puma_collective_init()");


    /*
    ** Intialize match list pointers and activate heap portal
    ** Used by  _bcast_short(), _gather(), _scatter(), _reduce()
    */
    rc = sptl_mle_set_ptrs( ml, 0, single_mle, single_mle, single_mle );
    if (!rc) { 
        rc = sptl_mle_set_ptrs( ml, single_mle, reply_mle, reply_mle, 
            reply_mle ); 
    }
    if (!rc) { 
        rc = sptl_mle_set_ptrs( ml, reply_mle, heap_mle, heap_mle, 
            heap_mle );
    }
    if (!rc) { 
        rc = sptl_mle_set_ptrs( ml, heap_mle, 0, 0, 0 );
    }
    coll_assert(rc,"_puma_collective_init()",
        "Failed to init match list links in _puma_collective_init()");
    PTL( Coll_Heap_Ptl ).mem_op = MATCHING;
    SPTL_ACTIVATE( Coll_Heap_Ptl );

    /*
    ** Initialize header block for handshakes.
    ** Used by _collect_long() and _dist_reduce_long()
    */
    for (i=0; i<(int) (_my_nnodes*2); i++) {
        req_bucket_desc[i].buf=NULL;
        req_bucket_desc[i].buf_len=0;
    }


    /*
    ** Now that the buffer descriptor is setup,
    ** attach it to the independent block portal and activate it.
    ** Used by _collect_long() and _dist_reduce_long()
    */
    rc = sptl_ind_init( &IND_MD( Coll_Hdr_Ptl), 
                        req_bucket_desc, 2 * _my_nnodes ); 
    coll_assert(rc,"_puma_collective_init()",
        "Failed to setup header portal in _puma_collective_init()");
    PTL( Coll_Hdr_Ptl ).mem_op = IND_CIRC_SV_HDR;
    SPTL_ACTIVATE( Coll_Hdr_Ptl );


    /*
    ** Initialize single data block portal.
    ** Used by _collect_long() and _dist_reduce_long()
    */
    rc = sptl_single_init( &SINGLE_MD( Coll_Bucket_Ptl), 
                            NULL, 0 ); 
    coll_assert(rc,"_puma_collective_init()",
        "Failed to setup header portal in _puma_collective_init()");
    PTL( Coll_Bucket_Ptl ).mem_op = SINGLE_SNDR_OFF_SV_BDY;
    SPTL_DEACTIVATE( Coll_Bucket_Ptl );

    return 0;
}

/*---------------------------------------------------------------------------*/

INT32
_puma_collective_cleanup(VOID)
{
INT32 rc=0;

    if ( (Coll_Heap_Ptl<NUM_PORTALS) && 
          SPTL_ALLOC_TEST( Coll_Heap_Ptl ) ) {
        if ( IS_MEM_OP( PTL( Coll_Heap_Ptl ).mem_op ) ) {
            SPTL_DEACTIVATE( Coll_Heap_Ptl );
            sptl_quiescence( Coll_Heap_Ptl );
        }
        rc  = sptl_dealloc( Coll_Heap_Ptl );
        coll_assert(rc,"_puma_collective_cleanup()",
            "Failed to deallocate heap portal in _puma_collective_cleanup()");
    }
    if ( (Coll_Bucket_Ptl<NUM_PORTALS) && 
          SPTL_ALLOC_TEST( Coll_Bucket_Ptl ) ) {
        if (IS_MEM_OP( PTL( Coll_Bucket_Ptl ).mem_op ) ) {
            SPTL_DEACTIVATE( Coll_Bucket_Ptl );
            sptl_quiescence( Coll_Bucket_Ptl );
        }
        rc = sptl_dealloc( Coll_Bucket_Ptl );
        coll_assert(rc,"_puma_collective_init()",
            "Failed to deallocate bucket portal in _puma_collective_cleanup()");
    }
    if ( (Coll_Hdr_Ptl<NUM_PORTALS) && 
          SPTL_ALLOC_TEST( Coll_Hdr_Ptl ) ) {
        if (IS_MEM_OP( PTL( Coll_Hdr_Ptl ).mem_op ) ) {
            SPTL_DEACTIVATE( Coll_Hdr_Ptl );
            sptl_quiescence( Coll_Hdr_Ptl );
        }
        rc = sptl_dealloc( Coll_Hdr_Ptl );
        coll_assert(rc,"_puma_collective_init()",
            "Failed to deallocate hdr portal in _puma_collective_cleanup()");
    }

    HDR_LINK_CLEANUP();

    free( req_bucket_desc );
    free( tree_ml );
    free( tree_heap_buf );

    return 0;
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Portal Initialize Protocol Functions
**==================================
*/

INT32
_vcoll_init_func(
                COLL_VINIT_F    vinit,
                COLL_VSEND_F    vsend,
                COLL_VRECV_F    vrecv,
                COLL_VTEST_F    vtest,
                COLL_VCLEANUP_F vcleanup,
                COLL_PROTO_TYPE *proto )
{
INT32 rc;

    if (proto) {
        if (vinit) { proto->p_init=vinit; }
        if (vsend) { proto->p_vsend=vsend; }
        if (vrecv) { proto->p_vrecv=vrecv; }
        if (vtest) { proto->p_test=vtest; }
        if (vcleanup) { proto->p_cleanup=vcleanup; }
    } else {
        ERRNO = EINVAL;
        rc = -1;
        coll_assert(rc,"_vcoll_init_func()",
            "Failed to init protocol functions in _vcoll_init_func()");
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/*
**==================================
** Protocol Init
**==================================
*/


INT32
def_vinit(
    INT32 gid,
    CHAMELEON mbits,
    CHAR *in_buf,
    CHAR *out_buf,
    INT32 data_size,
    INT32 buf_len,
    VOID *user_struct,
    VSENDRECV_HANDLE *handle )
{

MATCH_LIST_TYPE *ml;
MATCH_DESC_TYPE *smle;
MATCH_DESC_TYPE *rmle;

    /*
    ** For the portal implementation, setup a single block that points to the
    ** data buffer of length the specified length.  The user_struct needs to be
    ** filled in with the match list entry.
    ** 
    ** If this is the NX implementation, nothing needs to be done.
    */

    /*
    ** Fill in the handle structure
    */
    handle->portal                 = Coll_Heap_Ptl;
    handle->gid                 = gid;
    handle->in_buf                 = in_buf;
    handle->out_buf                 = out_buf;
    handle->data_size                 = data_size;
    handle->buf_len                 = buf_len;
    handle->heap_mle                 = heap_mle;
    handle->single_mle                 = single_mle;
    handle->reply_mle                 = reply_mle;
    handle->user_struct         = user_struct;
#ifdef __LONG_LONG__
        handle->mbits.ll         = mbits.ll;
#else /* __LONG_LONG__ */
        handle->mbits.ints.i0         = mbits.ints.i0;
        handle->mbits.ints.i1         = mbits.ints.i1;
#endif /* __LONG_LONG__ */

    /*
    ** Setup the match list entry and create the single block
    */
    ml = &ML_MD( handle->portal );
    MLE_SINGLE_MD( ml,handle->single_mle ).buf = in_buf;
    MLE_SINGLE_MD( ml,handle->single_mle ).buf_len = buf_len;
    MLE_SINGLE_MD( ml,handle->reply_mle ).buf = out_buf;
    MLE_SINGLE_MD( ml,handle->reply_mle ).buf_len = buf_len;

    smle = &MLE( handle->portal, handle->single_mle );
    rmle = &MLE( handle->portal, handle->reply_mle );
    smle->rank = (UINT32)-1;
    rmle->rank = (UINT32)-1;
    smle->gid  = (UINT32)gid;
    rmle->gid  = (UINT32)gid;
#ifdef __LONG_LONG__
        smle->ign_mbits.ll = 0x0000000000000000;
        rmle->ign_mbits.ll = 0x0000000000000000;
        smle->must_mbits.ll = mbits.ll;
#else /* __LONG_LONG__ */
        smle->ign_mbits.ints.i0 = 0x00000000;
        smle->ign_mbits.ints.i1 = 0x00000000;
        rmle->ign_mbits.ints.i0 = 0x00000000;
        rmle->ign_mbits.ints.i1 = 0x00000000;
        smle->must_mbits.ints.i0 = mbits.ints.i0;
        smle->must_mbits.ints.i1 = mbits.ints.i1;
#endif /* __LONG_LONG__ */
    rmle->must_mbits.ints.i0 = mbits.ints.i0 | COLLCORE_PING_BIT;
    rmle->must_mbits.ints.i1 = mbits.ints.i1;

#ifdef VDEBUG
    {
        char s[10*80];
        char stmp[80];
        int slen;
        s[0]='\0';
        sprintf(stmp,"/******************************/\n");
        slen=strlen(stmp); strncat(s,stmp,slen);
        sprintf(stmp,"%d:_init: single\n", _my_rank);
        slen=strlen(stmp); strncat(s,stmp,slen);
        sprintf(stmp,"\tbuf 0x%08x len %d mbits 0x%08x 0x%08x\n", 
                in_buf, buf_len, 
                smle->must_mbits.ints.i0, smle->must_mbits.ints.i1);
        slen=strlen(stmp); strncat(s,stmp,slen);
        sprintf(stmp,"%d:_init: reply\n", _my_rank);
        slen=strlen(stmp); strncat(s,stmp,slen);
        sprintf(stmp,"\tbuf 0x%08x len %d mbits 0x%08x 0x%08x\n", 
                out_buf, buf_len, 
                rmle->must_mbits.ints.i0, rmle->must_mbits.ints.i1);
        slen=strlen(stmp); strncat(s,stmp,slen);
        sprintf(stmp,"/******************************/\n");
        slen=strlen(stmp); strncat(s,stmp,slen);
        printf("%s",s); fflush(stdout);
    }
#endif /* VDEBUG */


    return 0;
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Protocol Cleanup
**==================================
*/

INT32
def_vcleanup( VSENDRECV_HANDLE *handle)
{
MATCH_LIST_TYPE *ml;

    /*
    ** Release the match list entry for the single block and de-initialize
    ** the structure.  It is assumed that these mle's have been
    ** deactivated before entry into this routine.
    */
    ml = &ML_MD( handle->portal );
    MLE_SINGLE_MD( ml,handle->single_mle ).buf = NULL;
    MLE_SINGLE_MD( ml,handle->single_mle ).buf_len = 0;
    MLE_SINGLE_MD( ml,handle->reply_mle ).buf = NULL;
    MLE_SINGLE_MD( ml,handle->reply_mle ).buf_len = 0;

    handle->portal                 = (PORTAL_INDEX)-1;
    handle->gid                 = (GID_TYPE)-1;
    handle->in_buf                 = NULL;
    handle->out_buf                 = NULL;
    handle->buf_len                 = 0;
    handle->heap_mle                 = 0;
    handle->single_mle                 = 0;
    handle->reply_mle                 = 0;
    handle->user_struct         = NULL;
    handle->mbits.ints.i0         = 0;
    handle->mbits.ints.i1         = 0;

    return 0;
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Protocol Vector Send
**==================================
*/

INT32
def_vsend(
    BOOLEAN flow_control,
    INT32 nrecs,
    DATA_REC *recs,
    VSENDRECV_HANDLE *handle)
{
INT32   rc;
INT32   i;
MATCH_LIST_TYPE *ml;


    /*
    ** For long messages, use flow_control for bandwidth
    */
    if (handle->buf_len > PROTOCOL_THRESHOLD) {
        flow_control = TRUE;
    }

#ifdef VDEBUG
        {
            char s[10*80];
            char stmp[80];
            int slen;
            s[0]='\0';
            printf("%d:_vsend: nrecs=%d recs=0x%08x\n", _my_rank, nrecs, recs);
            fflush(stdout);
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"%d:_vsend: nrecs=%d recs=0x%08x\n", 
                _my_rank, nrecs, recs);
            slen=strlen(stmp); strncat(s,stmp,slen);
            for (i=0; i<nrecs; i++) {
                sprintf(stmp,"\ts_rnk %d r_rnk %d oset %d len %d\n", 
                    recs[i].sndr_rank, recs[i].rcvr_rank,
                    recs[i].offset, recs[i].len);
                    slen=strlen(stmp); strncat(s,stmp,slen);
            }
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            printf("%s",s); fflush(stdout);
        }
#endif /* VDEBUG */
    if (flow_control) {
        /*
        ** Flow control
        */
        BOOLEAN  found;
        INT32 cnt;
        DYN_MSG_LINK *mlink;
        CHAMELEON matchall_bits = MATCH_ALL();


        /*
        ** Activate reply portal.
        ** It is assumed that the user appropriately uses the
        ** tags to make sure other requests do not access 
        ** this data.
        */

        SPTL_MCH_ACTIVATE( handle->portal, handle->reply_mle );


        /* 
        ** Look for readmem requests in heap and reply if found 
        */
        cnt = 0;
        ml = &ML_MD( handle->portal );
        handle->mbits.ints.i0 |= COLLCORE_PING_BIT;
        for (i=0; i<nrecs; i++) {
            found = FALSE;

            mlink = sptl_dyn_srch(
                &MLE_DYN_MD( ml,handle->heap_mle ),
                recs[i].rcvr_rank, handle->gid, matchall_bits,
                handle->mbits, handle->portal, FALSE, &found );
            if (found) {

                /*
                ** Found the message, fulfill the data request
                */
#ifdef VDEBUG
                {
                    char s[10*80];
                    char stmp[80];
                    int slen;
                    s[0]='\0';
                    sprintf(stmp,"/******************************/\n");
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    sprintf(stmp,"%d:_send:_portal_send:flow control\n", 
                        _my_rank);
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    sprintf(stmp,"\tbuf 0x%08x offset 0x%08x len 0x%08x\n", 
                            handle->out_buf, 
                            recs[i].offset, recs[i].len);
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    sprintf(stmp,"\tgid %d rcvr_rnk %d portal %d\n", 
                            handle->gid, 
                            recs[i].rcvr_rank,
                            handle->portal); 
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    sprintf(stmp,"\tmbits 0x%08x 0x%08x\n", 
                            handle->mbits.ints.i0,
                            handle->mbits.ints.i1); 
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    sprintf(stmp,"/******************************/\n");
                    slen=strlen(stmp); strncat(s,stmp,slen);
                    printf("%s",s); fflush(stdout);
                }
#endif /* VDEBUG */
                cnt++;
                handle->mbits.ints.i0 &= ~COLLCORE_PING_BIT;

                rc = portal_send(
                    (CHAR *)(handle->out_buf + recs[i].offset),
                    recs[i].len, 
                    recs[i].offset,
                    handle->gid, 
                    recs[i].rcvr_rank,
                    handle->portal, 
                    &(handle->mbits),
                    NULL, 0, 0, 0, NULL, NULL);


                handle->mbits.ints.i0 |= COLLCORE_PING_BIT;
                rc = sptl_dyn_hp_free (
                        &MLE_DYN_MD( ml,handle->heap_mle ), mlink );
                coll_assert(rc,"def_vsend()",
                    "Failed on portal_send() in send in def_vsend()");
            }
        }
        handle->mbits.ints.i0 &= ~COLLCORE_PING_BIT;

        /* Wait for all readmem's to be satisfied */
#ifdef VDEBUG
        {
            char s[10*80];
            char stmp[80];
            int slen;
            s[0]='\0';
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"%d:_send:flow control wait\n", _my_rank);
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"\tcnt= %d  reply_cnt %d nrecs %d\n", 
                    cnt, 
                    MLE_SINGLE_MD( ml, handle->reply_mle ).msg_cnt, 
                    nrecs);
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            printf("%s",s); fflush(stdout);
        }
#endif /* VDEBUG */
 
        while ( MLE_SINGLE_MD( ml, handle->reply_mle ).msg_cnt < (nrecs-cnt) );

        MLE_SINGLE_MD( ml, handle->reply_mle ).msg_cnt -= (nrecs-cnt);
        SPTL_MCH_DEACTIVATE( handle->portal, handle->reply_mle );

    } else {
        /*
        ** No flow control
        */
        for (i=0; i<nrecs; i++) {
#ifdef VDEBUG
            {
                char s[10*80];
                char stmp[80];
                int slen;
                s[0]='\0';
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"%d:_send:_portal_send:no flow\n", _my_rank);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tbuf 0x%08x offset 0x%08x len 0x%08x\n", 
                        handle->out_buf, 
                        recs[i].offset, recs[i].len);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tgid %d rcvr_rnk %d portal %d\n", 
                        handle->gid, 
                        recs[i].rcvr_rank,
                        handle->portal); 
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tmbits 0x%08x 0x%08x\n", 
                        handle->mbits.ints.i0,
                        handle->mbits.ints.i1); 
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                printf("%s",s); fflush(stdout);
            }
#endif /* VDEBUG */

            rc = portal_send(
                    (CHAR *)(handle->out_buf + recs[i].offset),
                    recs[i].len, 
                    recs[i].offset,
                    handle->gid, 
                    recs[i].rcvr_rank,
                    handle->portal, 
                    &(handle->mbits),
                    NULL, 0, 0, 0, NULL, NULL);

            coll_assert(rc,"def_vsend()",
                "Failed on portal_send() in send in def_vsend()");
        }
    }

    return ESUCCESS;
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Protocol Vector Receive
**==================================
*/

INT32
def_vtest( 
    VSENDRECV_HANDLE *handle ATTR_UNUSED, BOOLEAN wait ATTR_UNUSED)
{
    return ESUCCESS;
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Protocol Vector Receive
**==================================
*/

INT32
def_vrecv(
    PUMA_OP op,
    BOOLEAN flow_control,
    INT32 nrecs,
    DATA_REC *recs,
    VSENDRECV_HANDLE *handle)
{
INT32   i,rc;
INT32   elements;
MATCH_LIST_TYPE *ml;
CHAMELEON readmem_mbits;

    /*
    ** For long messages, use flow control for bandwidth
    */
    if (handle->buf_len > PROTOCOL_THRESHOLD) {
        flow_control = TRUE;
    }
#ifdef VDEBUG
        {
            char s[10*80];
            char stmp[80];
            int slen;
            s[0]='\0';
            printf("%d:_vrecv: nrecs=%d recs=0x%08x\n", _my_rank, nrecs, recs);
            fflush(stdout);
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"%d:_vrecv: nrecs=%d recs=0x%08x\n", 
                _my_rank, nrecs, recs);
            slen=strlen(stmp); strncat(s,stmp,slen);
            for (i=0; i<nrecs; i++) {
                sprintf(stmp,"\ts_rnk %d r_rnk %d oset %d len %d\n", 
                    recs[i].sndr_rank, recs[i].rcvr_rank,
                    recs[i].offset, recs[i].len);
                    slen=strlen(stmp); strncat(s,stmp,slen);
            }
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            printf("%s",s); fflush(stdout);
        }
#endif /* VDEBUG */
    if (flow_control) {
        /*
        ** Flow control
        */


        /*
        ** Activate single portal for reception.
        ** It is assumed that the user appropriately uses the
        ** tags to make sure this data is not corrupted by
        ** future or out-of-group writes.
        */

        SPTL_MCH_ACTIVATE( handle->portal, handle->single_mle );


        /* 
        ** Loop over senders, performing readmem requests
        ** one at a time at my pace.
        */
        ml = &ML_MD( handle->portal );
#ifdef __LONG_LONG__
            readmem_mbits.ll = handle->mbits.ll;
            readmem_mbits.ints.i0 |= COLLCORE_PING_BIT;
#else /* __LONG_LONG__ */
            readmem_mbits.ints.i0 = handle->mbits.ints.i0 | COLLCORE_PING_BIT;
            readmem_mbits.ints.i1 = handle->mbits.ints.i1;
#endif /* __LONG_LONG__ */
        for (i=0; i<nrecs; i++) {
            /* Send readmem request */
#ifdef VDEBUG
            {
                char s[10*80];
                char stmp[80];
                int slen;
                s[0]='\0';
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"%d:_recv:_readmem:flow control\n", _my_rank);
                slen=strlen(stmp); strncat(s,stmp,slen);
                /*
                sprintf(stmp,"\tbuf 0x%08x offset 0x%08x len 0x%08x\n", 
                        handle->out_buf, 
                        recs[i].offset, recs[i].len);
                slen=strlen(stmp); strncat(s,stmp,slen);
                */
                sprintf(stmp,"\tgid %d sndr_rnk %d portal %d\n", 
                        handle->gid, 
                        recs[i].sndr_rank,
                        handle->portal); 
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tmbits 0x%08x 0x%08x\n", 
                        handle->mbits.ints.i0,
                        handle->mbits.ints.i1); 
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                printf("%s",s); fflush(stdout);
            }
#endif /* VDEBUG */

            rc = portal_send(
                NULL, 0, recs[i].offset,
                handle->gid, recs[i].sndr_rank,
                handle->portal, &readmem_mbits,
                &(handle->mbits), handle->portal,
                recs[i].len, recs[i].offset, NULL, NULL);
                coll_assert(rc,"def_vrecv()",
                    "Failed on portal_send() in send in def_vrecv()");

            /* Wait for readmem data */
#ifdef VDEBUG
            {
                char s[10*80];
                char stmp[80];
                int slen;
                s[0]='\0';
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"%d:_recv:flow control wait\n", _my_rank);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tsingle_cnt %d nrecs %d\n", 
                        MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt, 
                        nrecs);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                printf("%s",s); fflush(stdout);
            }
#endif /* VDEBUG */

            while ( !MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt );

            MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt--;
            if (op && handle->buf_len) {
                elements = handle->buf_len / handle->data_size; 
                op( handle->in_buf, 
                    handle->out_buf, 
                    &elements,
                    handle->user_struct);
            }
        }
        SPTL_MCH_DEACTIVATE( handle->portal, handle->single_mle );


    } else if ( op ) {
        /*
        ** Operation, but no flow control 
        */
        BOOLEAN  found;
        DYN_MSG_LINK *mlink;
        CHAMELEON matchall_bits = MATCH_ALL();


        /* 
        ** All sender contributions will arrive in heap.
        ** Look for each one and accumulate
        */
        ml = &ML_MD( handle->portal );
        for (i=0; i<nrecs; i++) {
            found = FALSE;
#ifdef VDEBUG
            {
                char s[10*80];
                char stmp[80];
                int slen;
                s[0]='\0';
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"%d:_recv:op and no flow \n", _my_rank);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"\tSearching s_rnk= %d  mbits 0x%08x 0x%08x\n", 
                        recs[i].sndr_rank,
                        handle->mbits.ints.i0,
                        handle->mbits.ints.i1);
                slen=strlen(stmp); strncat(s,stmp,slen);
                sprintf(stmp,"/******************************/\n");
                slen=strlen(stmp); strncat(s,stmp,slen);
                printf("%s",s); fflush(stdout);
            }
#endif /* VDEBUG */
            while (!found) {

                mlink = sptl_dyn_srch(
                    &MLE_DYN_MD( ml,handle->heap_mle ),
                    recs[i].sndr_rank, handle->gid, matchall_bits,
                    handle->mbits, handle->portal, FALSE, &found );
            }
            /*
            ** Found the message, accumulate
            */
            if (op && handle->buf_len) {
                elements = handle->buf_len / handle->data_size;
                op( DYN_OSET2PTR( &MLE_DYN_MD(ml,handle->heap_mle),mlink->data),
                    handle->out_buf, 
                    &elements,
                    handle->user_struct);
            }
            rc = sptl_dyn_hp_free (
                    &MLE_DYN_MD( ml,handle->heap_mle ), mlink );
            coll_assert(rc,"def_vrecv()",
                "Failed on portal_send() in send in def_vrecv()");
        }

    } else {
        /*
        ** No flow control and no operation!
        */
        BOOLEAN  found;
        INT32 cnt;
        DYN_MSG_LINK *mlink;
        CHAMELEON matchall_bits = MATCH_ALL();


        /*
        ** Activate single portal to recv data.
        ** It is assumed that the user appropriately uses the
        ** tags to make sure this data is not corrupted by
        ** future or out-of-group writes.
        */
        SPTL_MCH_ACTIVATE( handle->portal, handle->single_mle );


        /* 
        ** Look for data that already arrived in the heap
        */
        cnt = 0;
        ml = &ML_MD( handle->portal );
        for (i=0; i<nrecs; i++) {
            found = FALSE;

            mlink = sptl_dyn_srch(
                &MLE_DYN_MD( ml,handle->heap_mle ),
                recs[i].sndr_rank, handle->gid, matchall_bits,
                handle->mbits, handle->portal, FALSE, &found );
            if (found) {
                /*
                ** Found the message, copy it into place
                */
                cnt++;
                memcpy( (CHAR *)(handle->in_buf + recs[i].offset),
                DYN_OSET2PTR(&MLE_DYN_MD( ml,handle->heap_mle ),mlink->data), 
                        mlink->hdr.msg_len );
                rc = sptl_dyn_hp_free (
                        &MLE_DYN_MD( ml,handle->heap_mle ), mlink );
                coll_assert(rc,"def_vrecv()",
                    "Failed on portal_send() in send in def_vrecv()");
            }
        }

        /* Wait for all data to arrive */
#ifdef VDEBUG
        {
            char s[10*80];
            char stmp[80];
            int slen;
            s[0]='\0';
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"%d:_recv:no flow wait\n", _my_rank);
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"\tcnt= %d  single_cnt %d nrecs %d\n", 
                    cnt, 
                    MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt, 
                    nrecs);
            slen=strlen(stmp); strncat(s,stmp,slen);
            sprintf(stmp,"/******************************/\n");
            slen=strlen(stmp); strncat(s,stmp,slen);
            printf("%s",s); fflush(stdout);
        }
#endif /* VDEBUG */


        while ( MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt<(nrecs-cnt) );

        MLE_SINGLE_MD( ml, handle->single_mle ).msg_cnt -= (nrecs-cnt);
        SPTL_MCH_DEACTIVATE( handle->portal, handle->single_mle );

    }

    return ESUCCESS;
}


/*****************************************************************************/
#endif
