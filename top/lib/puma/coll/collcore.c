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
$Id: collcore.c,v 1.10 2001/02/16 05:39:20 lafisk Exp $
*/

#include <stdio.h>
#include <string.h>
#include "puma.h"
#include "puma_errno.h"
#include "malloc.h"

#define SAVE_PUMA_PROTO
/* #include  "libload.h" */
#include "collcore.h"

#ifdef LINUX_PORTALS
#define PORTAL_COLLECTIVE
#endif

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  REDUCTION OPERATION ROUTINES
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

#define PUMA_ROUTINE(name, type, op)        \
    PUMA_PROTO( (name) )                \
    {                                        \
        type *a;                        \
        type *b;                        \
        INT32 i = *len;                        \
	(void) user_struct;               \
        a = (type *)c_in;                \
        b = (type *)c_out;                \
        while( --i >= 0 ) {                \
            b[i] = (op);                \
        }                                \
        return (0);                        \
    }


/* Global sum */
PUMA_ROUTINE( PUMA_ISUM,  INT32,        (a[i] + b[i]) )
PUMA_ROUTINE( PUMA_SSUM,  FLOAT32,        (a[i] + b[i]) )
PUMA_ROUTINE( PUMA_DSUM,  FLOAT64,        (a[i] + b[i]) )
 
/* Products */
PUMA_ROUTINE( PUMA_IPROD, INT32,        (a[i] * b[i]) )
PUMA_ROUTINE( PUMA_SPROD, FLOAT32,        (a[i] * b[i]) )
PUMA_ROUTINE( PUMA_DPROD, FLOAT64,        (a[i] * b[i]) )

/* Logical and bitwise AND */
PUMA_ROUTINE( PUMA_LAND,  UINT32,        (a[i] && b[i]) )
PUMA_ROUTINE( PUMA_BAND,  UINT32,        (a[i] & b[i]) )

/* Logical and bitwise OR */
PUMA_ROUTINE( PUMA_LOR,   UINT32,        (a[i] || b[i]) )
PUMA_ROUTINE( PUMA_BOR,   UINT32,        (a[i] | b[i]) )

/* Min and Max routines */
PUMA_ROUTINE( PUMA_IMAX,  INT32,        (a[i] > b[i] ? a[i] : b[i]) )
PUMA_ROUTINE( PUMA_SMAX,  FLOAT32,        (a[i] > b[i] ? a[i] : b[i]) )
PUMA_ROUTINE( PUMA_DMAX,  FLOAT64,        (a[i] > b[i] ? a[i] : b[i]) )

PUMA_ROUTINE( PUMA_IMIN,  INT32,        (a[i] < b[i] ? a[i] : b[i]) )
PUMA_ROUTINE( PUMA_SMIN,  FLOAT32,        (a[i] < b[i] ? a[i] : b[i]) )
PUMA_ROUTINE( PUMA_DMIN,  FLOAT64,        (a[i] < b[i] ? a[i] : b[i]) )

/* Accept the args and do nothing.  A collective NOP! */
PUMA_PROTO( PUMA_NULLOP )
{
 (void) c_in;
 (void) c_out;
 (void) len;
 (void) user_struct;
 return (0);
}


/*****************************************************************************/
/*****************************************************************************/
/*    These collective communication routines do not compile.  They use      */
/*    some structures from Portals 2.  But we'll keep them around because    */
/*    someone may find them useful after doing a small amount of             */
/*    modification.                                                          */
/*****************************************************************************/
/*****************************************************************************/

#if 0

#ifdef __COUGAR__
    static INT32 BANDWIDTH = 390.0e6;   /* Mb/sec  */
    static INT32 ALPHA1way =  10.0e-6;  /* seconds */
    static INT32 ALPHAshft =  20.0e-6;  /* seconds */
    static INT32 ALPHAxchg =  20.0e-6;  /* seconds */
    static INT32 BETA      =  2.56e-9;  /* sec/byte  or  1/BANDWIDTH */
#else /*  __COUGAR__ */
    /* static INT32 BANDWIDTH = 160.0e6; */   /* Mb/sec  */
    static INT32 ALPHA1way =  25.0e-6;  /* seconds */
    static INT32 ALPHAshft =  50.0e-6;  /* seconds */
    /* static INT32 ALPHAxchg =  50.0e-6; */  /* seconds */
    static INT32 BETA      =  6.25e-9;  /* sec/byte  or  1/BANDWIDTH */
#endif /* __COUGAR__ */

#define _beta(byte_len)         (BETA);
#define PTL( ptl )                (_my_pcb->portal_table2[(ptl)])
#define IND_MD( ptl )                (_my_pcb->portal_table2[(ptl)].mem_desc.ind)
#define SINGLE_MD( ptl )        (_my_pcb->portal_table2[(ptl)].mem_desc.single)
#define ML_MD( ptl )                (_my_pcb->portal_table2[(ptl)].mem_desc.match)
#define MLE( ptl,mle )                (_my_pcb->portal_table2[(ptl)].mem_desc.match.u_lst[(mle)])
#define MLE_DYN_MD(ml_ptr,mle)        ((ml_ptr)->u_lst[(mle)].mem_desc.dyn)
#define MLE_SINGLE_MD(ml_ptr,mle)        ((ml_ptr)->u_lst[(mle)].mem_desc.single)

UINT32  single_mle = 1;
UINT32  reply_mle  = 2;
UINT32  heap_mle   = 3;

PORTAL_INDEX Coll_Heap_Ptl=INVALID_PTL;
PORTAL_INDEX Coll_Bucket_Ptl=INVALID_PTL;
PORTAL_INDEX Coll_Hdr_Ptl=INVALID_PTL;

DATA_REC                *g_recs;
IND_MD_BUF_DESC                *req_bucket_desc=NULL;
MATCH_DESC_TYPE                *tree_ml=NULL;
DYN_MALLOC_LINK_TYPE        *tree_heap_buf=NULL;
IND_LINK                *ind_tail_link=NULL;
IND_LINK                 ind_head_link;

/*****************************************************************************/
/* If the routine and proto macros are not needed, remove them */
#ifndef SAVE_PUMA_ROUTINE
#undef PUMA_ROUTINE
#undef PUMA_PROTO
#endif /* SAVE_PUMA_ROUTINE */

/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  BROADCAST
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/


/*---------------------------------------------------------------------------*/
/*
**=============================
** Broadcast timing routine
**=============================
*/

FLOAT64 
_bcast_time(INT32 p, INT32 n, INT32 interleave, BOOLEAN ishort) 
{

FLOAT64 dp,dn;
FLOAT64 time;
FLOAT64 alpha,beta;
FLOAT64 c_logp;

    if ((p<=1) || (n<=0)) {
        return(0.0);
    }

    dp = (FLOAT64)p;
    dn = (FLOAT64)n;

    c_logp = (FLOAT64)_ceillogp( p );
    beta   = (FLOAT64)interleave * _beta(n);
    if (ishort) {

        /* Fanout */
        alpha  = ALPHA1way;
        time = (c_logp) * alpha + (c_logp * dn) * beta;

    } else {
        
        /* Scatter */
        alpha  = ALPHA1way;
        time   = (c_logp) * alpha + ((dp-1.0)/dp * dn) * beta;

        /* Collect */
        alpha  = ALPHAshft;
        time  += ((dp-1.0)/2.0) * alpha + ((dp-1.0)/dp * dn) * beta;

    }

    return (time);
}



/*---------------------------------------------------------------------------*/
/*
**=============================
** Broadcast short routine
**=============================
*/

#include "srvr_err.h"

INT32
_bcast_short(
    CHAR * data, INT32 byte_len,
    INT32 type,
    INT32 *list, INT32 lstride, INT32 ngroup, INT32 myrank, INT32 rootrank,
    COLL_STRUCT *c_st)
{
INT32   rc;
INT32   left, right, mid;
INT32   iroot, dest;
INT32   n_srecs;
INT32   n_rrecs;
DATA_REC *s_recs;
DATA_REC  r_rec;
BOOLEAN   flow_control;
CHAMELEON itype;
VSENDRECV_HANDLE handle;

    rc = 0;

    /* Quick return */
    if (ngroup <= 1) {
        return (ngroup == 1 ? 0 : -1);
    }

    itype.ints.i0 = COLLCORE_BCAST;
    itype.ints.i1 = (type == -1 ? COLLCORE_BCAST : type);

    s_recs = g_recs;
    n_srecs = 0;
    n_rrecs = 0;
    iroot = rootrank;
    left = 0;
    right = ngroup - 1;
    while (left < right) {
        mid = (left + right) / 2;
        if (iroot <= mid) {
            dest = (iroot == left ? mid + 1 : right);
        } else {
            dest = (iroot == right ? mid : left);
        }

        if (myrank == iroot) {
            s_recs[n_srecs].sndr_rank = _my_rank;
            s_recs[n_srecs].rcvr_rank = 
                (list ? list[dest * lstride] : dest * lstride);
            s_recs[n_srecs].offset = 0;
            s_recs[n_srecs].len = byte_len;
            n_srecs++;
        } else if (myrank == dest) {
            /* There is at most one receive */
            r_rec.sndr_rank = 
                (list ? list[iroot * lstride] : iroot * lstride);
            r_rec.rcvr_rank = _my_rank;
            r_rec.offset = 0;
            r_rec.len = byte_len;
            n_rrecs++;
        }

        if (myrank <= mid) {
            if (iroot > mid) {
                iroot = dest;
            }
            right = mid;
        } else {
            if (iroot <= mid) {
                iroot = dest;
            }
            left = mid + 1;
        }

        if (rc) {
            return (rc);
        }
    }

    /*
    ** Perform communication
    */
    rc = c_st->proto.p_init( GROUP, itype, data, data, 
                    1, byte_len, c_st->user_struct, &handle );
    flow_control = (byte_len > PROTOCOL_THRESHOLD ? TRUE : FALSE);
    if (!rc && n_rrecs) { 
        rc = c_st->proto.p_vrecv( NULL, flow_control, n_rrecs, 
            &r_rec, &handle); 
    }
    if (!rc && n_srecs) { 

        rc = c_st->proto.p_vsend( flow_control, n_srecs, s_recs, &handle); 

    }
    if (!rc) { 
        rc = c_st->proto.p_cleanup(&handle); 
    }
    coll_assert(rc,"_bcast_short()","Failed proto comms in _bcast_long()");

    return (rc);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Broadcast long
**=============================
*/

INT32
_bcast_long(CHAR **data_ptrs, INT32 dstride, INT32 type,
            INT32 *list, INT32 lstride, INT32 ngroup, 
            INT32 myrank, INT32 rootrank, 
            COLL_STRUCT *c_st, BOOLEAN send_uplist) 
{

INT32 rc;


    rc = 0;
    rc = _scatter( data_ptrs, dstride, type, list, lstride, ngroup, myrank, 
        rootrank, c_st);
    coll_assert(rc,"_bcast_long()","Failed to scatter in _bcast_long()");

    rc = _collect_long( data_ptrs, dstride, type, list, lstride, ngroup,
        myrank, c_st, send_uplist);
    coll_assert(rc,"_bcast_long()","Failed to _collect_long in _bcast_long()");

    return (rc);
}



/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  COLLECT
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/


/*---------------------------------------------------------------------------*/
/*
**=============================
** Collect timing routine
**=============================
*/

FLOAT64 
_collect_time(INT32 p, INT32 n, INT32 interleave, BOOLEAN ishort) 

{

FLOAT64 dp,dn;
FLOAT64 time;
FLOAT64 beta;
FLOAT64 c_logp;

    if ((p<=1) || (n<=0)) {
        return(0.0);
    }

    dp = (FLOAT64)p;
    dn = (FLOAT64)n;

    c_logp = (FLOAT64)_ceillogp( p );
    beta   = (FLOAT64)interleave * _beta(n);
    if (ishort) {

#ifdef NEWCOL
            time = (c_logp+1) * ALPHA1way + ((dp*dp-1)/dp/3.0) * dn * beta;
#else /* NEWCOL */
            /* Gather */
            time  = (c_logp) * ALPHA1way + ((dp-1)/dp) * dn * beta;
            /* Bcast  */
            time += (c_logp) * ALPHA1way + (c_logp) * dn * beta;
#endif /* NEWCOL */

    } else {
        
        /* Bucket collect */
        time  = ((dp-1.0)/2.0) * ALPHAshft + ((dp-1.0)/dp * dn) * beta;

    }

    return (time);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Collect short
**=============================
*/

INT32
_collect_short(CHAR **data_ptrs ATTR_UNUSED, INT32 dstride ATTR_UNUSED,
	    INT32 type ATTR_UNUSED, INT32 *list ATTR_UNUSED,
	    INT32 lstride ATTR_UNUSED, INT32 ngroup ATTR_UNUSED,
            INT32 myrank ATTR_UNUSED, COLL_STRUCT *c_st ATTR_UNUSED) 

{

INT32 rc;

    rc = 0;
    return (rc);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Collect long
**=============================
*/


INT32
_collect_long(
    CHAR * data_ptrs[], INT32 dstride,
    INT32 type,
    INT32 list[], INT32 lstride, INT32 ngroup, INT32 myrank,
    COLL_STRUCT *c_st ATTR_UNUSED, BOOLEAN send_uplist)
{
INT32   i, rc;
INT32   dest_node;                /* destination node's id        */
INT32   src_node;                /* source node's id                */

#ifndef PORTAL_COLLECTIVE
INT32   rsrc;                        /* Temporary source id                */
#endif

INT32   send_bkt;                /* bucket of data to be sent        */
INT32   recv_bkt;                /* bucket of data to receive        */
INT32   m_length;                /* length of bucket                */
INT32   recv_len;
UINT32  offset;
BOOLEAN found;
IND_MD_BUF_DESC *msg_buf_desc;
CHAMELEON mbits;


    rc = 0;
    if (ngroup <= 1) {
        return (ngroup==1 ? 0 : -1);
    }


    /*
    ** calculate which node to send to and which node to receive from.
    */
    if (send_uplist) {
        /*
        ** pass buckets to the right
        */
        dest_node = ((myrank + 1) % ngroup) * lstride;
        src_node = ((myrank + ngroup - 1) % ngroup) * lstride;
    } else {
        /*
        ** pass buckets to the left
        */
        dest_node = ((myrank + ngroup - 1) % ngroup) * lstride;
        src_node = ((myrank + 1) % ngroup) * lstride;
    }


    /*
    ** If there is a list, translate the logical node ID into an actual ID
    */
    if (list) {
        dest_node = list[dest_node];
        src_node = list[src_node];
    }


#ifdef PORTAL_COLLECTIVE
        /*
        ** Portal specific initialization: Setup single block for data, ping
        ** source and wait for handshake from receiver.  Once received, send
        ** until all the data is taken care of.
        */

        /* 
        ** Initialize data 
        */
        recv_len = 0;


        /* 
        ** Setup the single block entry
        */
        m_length = data_ptrs[ngroup * dstride] - data_ptrs[0];
        SINGLE_MD( Coll_Bucket_Ptl ).buf = data_ptrs[0];
        SINGLE_MD( Coll_Bucket_Ptl ).buf_len = m_length;
        SPTL_ACTIVATE( Coll_Bucket_Ptl );


        /* 
        ** Ping the sender 
        */
        mbits.ints.i0=COLLCORE_COLLECT;
        mbits.ints.i1=(type==-1 ? COLLCORE_COLLECT : type);
        rc = portal_send(
            NULL, 0, 0, GROUP, src_node, Coll_Hdr_Ptl,
            &mbits, NULL, 0, 0, 0, NULL, NULL);
        coll_assert(rc,"_collect_long()",
            "Failed to ping source in _collect_long()");


        /* 
        ** Wait for ping from receiver.
        **
        ** Note that for MPI, there may be overlapped communicators
        ** each trying to perform this operation.  We must be prepared
        ** to have multiple out-of-order requests coming in from
        ** other nodes.  Thus the do-while loop.
        */
        msg_buf_desc = NULL;
        found = FALSE;
        while (1) {
            sptl_quiescence(Coll_Hdr_Ptl);
            rc = sptl_ind_msg_probe( &IND_MD( Coll_Hdr_Ptl ),
                                     IND_CIRC_SV_HDR,
                                     &msg_buf_desc,
                                     FALSE );
            if ((!rc) && (msg_buf_desc == NULL)) {
                LINK_FIND_N_FREE( mbits,found );
                if (!found) {
                    sptl_quiescence(Coll_Hdr_Ptl);
                    rc = sptl_ind_msg_probe( &IND_MD( Coll_Hdr_Ptl ),
                                             IND_CIRC_SV_HDR,
                                             &msg_buf_desc,
                                             TRUE );
                } else {
                    /*
                    ** Found it in my unexpected list.
                    */
                    break;
                }
            }
            if ((!rc) && 
#ifdef __LONG_LONG__
                    (msg_buf_desc->hdr.dst_mbits.ll != mbits.ll) ){
#else /* __LONG_LONG__ */
                    ((msg_buf_desc->hdr.dst_mbits.ints.i1 != mbits.ints.i1) || 
                    (msg_buf_desc->hdr.dst_mbits.ints.i0 != mbits.ints.i0)) ) {
#endif /* __LONG_LONG__ */
                HDR_LINK_ADD( msg_buf_desc );
                sptl_quiescence(Coll_Hdr_Ptl);
                rc = sptl_ind_msg_free( &IND_MD(Coll_Hdr_Ptl), 
                                        IND_CIRC_SV_HDR, 
                                        msg_buf_desc );
                if (rc) { break; }
            } else {
                if (!rc) {
                    sptl_quiescence(Coll_Hdr_Ptl);
                    rc = sptl_ind_msg_free( &IND_MD(Coll_Hdr_Ptl), 
                                            IND_CIRC_SV_HDR, 
                                            msg_buf_desc );
                }
                break;
            }
        } 
        coll_assert(rc,
            "_collect_long()", "Failed probing for ping in _collect_long()");


        /*
        ** Free the buffer for reuse and reset the probe.
        */
        rc = sptl_ind_rst_probe( &IND_MD(Coll_Hdr_Ptl) ); 
        coll_assert(rc,
            "_collect_long()","Failed to free ind buffers in _collect_long()");


#endif /* PORTAL_COLLECTIVE */


    type = (type == -1 ? COLLCORE_COLLECT : type);
    for (i = 1; i < ngroup; i++) {
        /*
        ** calculate which bucket to send and which to receive
        */
        if (send_uplist){
            recv_bkt   = (myrank + ngroup - i) % ngroup;
            send_bkt  = (recv_bkt + 1) % ngroup;
            send_bkt *= dstride;
            recv_bkt  *= dstride;
        } else {
            recv_bkt   = (myrank + i) % ngroup;
            send_bkt  = (recv_bkt + ngroup - 1) % ngroup;
            send_bkt *= dstride;
            recv_bkt  *= dstride;
        }


        /*
        ** calculate the length of the bucket being passed
        */
        m_length = (data_ptrs[send_bkt + dstride] - data_ptrs[send_bkt]);


        /*
        ** Send a portion
        */
#ifdef PORTAL_COLLECTIVE
            offset = data_ptrs[send_bkt] - data_ptrs[0];
            rc = portal_send(
                data_ptrs[send_bkt], m_length, offset, 
                GROUP, dest_node, Coll_Bucket_Ptl,
                &mbits, NULL, 0, 0, 0, NULL, NULL);
            coll_assert(rc,"_collect_long()",
                "Failed to send data in _collect_long()");
#else /* PORTAL_COLLECTIVE */
            rc = _nsend(data_ptrs[send_bkt], m_length, dest_node, type,
                NULL, NULL);
            coll_assert(rc,"_collect_long()",
                "Failed on nsend in _collect_long()");
#endif /* PORTAL_COLLECTIVE */


        /*
        ** calculate the length of the bucket being received
        */
        m_length = (data_ptrs[recv_bkt + dstride] - data_ptrs[recv_bkt]);


#ifdef PORTAL_COLLECTIVE
            /*
            ** The next batch of data must have arrived before we
            ** may procede.  recv_len keeps track of the total number
            ** of bytes that we should expect to have received
            */
            recv_len += m_length;
            while (SINGLE_MD(Coll_Bucket_Ptl).msg_cnt < i);

#else /* PORTAL_COLLECTIVE */
            /*
            ** Receive a portion
            */
            rsrc = src_node;
            rc = _nrecv(data_ptrs[recv_bkt], &m_length, &rsrc, &type, NULL,
                NULL);
            coll_assert( rc,"_collect_long()",
                "Failed on _nrecv in collect_long()");
#endif /* PORTAL_COLLECTIVE */
    }


#ifdef PORTAL_COLLECTIVE
        /*
        ** Cleanup the portal structures for the next time.
        ** Make sure all the data has arrived.
        */


        /*
        ** Release the portal
        */
        SPTL_DEACTIVATE( Coll_Bucket_Ptl );
        sptl_quiescence( Coll_Bucket_Ptl );
        SINGLE_MD( Coll_Bucket_Ptl ).buf = NULL;
        SINGLE_MD( Coll_Bucket_Ptl ).buf_len = 0;
        SINGLE_MD(Coll_Bucket_Ptl).rw_bytes = -1;
        SINGLE_MD(Coll_Bucket_Ptl).msg_cnt -= ngroup-1;

#endif /* PORTAL_COLLECTIVE */

    return (rc);

}


/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  GATHER
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

/*---------------------------------------------------------------------------*/
/*
**=============================
** Gather timing routine
**=============================
*/

FLOAT64 
_gather_time(INT32 p ATTR_UNUSED, INT32 n ATTR_UNUSED,
	     INT32 interleave ATTR_UNUSED, BOOLEAN ishort ATTR_UNUSED)

{
    return 0.0;
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Gather routine
**=============================
*/

INT32
_gather(CHAR * data_ptrs[], INT32 dstride, INT32 type,
    INT32 list[], INT32 lstride, INT32 ngroup,
    INT32 myrank, INT32 rootrank,
    COLL_STRUCT *c_st)
{
INT32   i,rc;
INT32   left, right, mid;
INT32   iroot, src;
INT32   dstart, dend, len, offset;
INT32   data_len;
INT32   clog;
INT32     n_rrecs;
INT32     n_srecs;
DATA_REC  s_rec;
DATA_REC *r_recs;
BOOLEAN   flow_control;
CHAMELEON itype;
VSENDRECV_HANDLE handle;

    rc = 0;

    /* Quick return */
    if (ngroup <= 1) {
        return (ngroup == 1 ? 0 : -1);
    }

    /*
    ** calculate the size of the single block portal buffer
    */
    data_len = data_ptrs[dstride * ngroup] - data_ptrs[0];

    itype.ints.i0 = COLLCORE_GATHER;
    itype.ints.i1 = (type == -1 ? COLLCORE_GATHER : type);

    r_recs = g_recs;
    n_rrecs = 0;
    n_srecs = 0;
    clog  = _ceillogp(ngroup);
    iroot = rootrank;
    left  = 0;
    right = ngroup - 1;

    /*
    ** using the scatter routine's algorithm
    */
    for (i = 0; i < clog; i++) {
        mid = (left + right) / 2;
        if (iroot <= mid) {
            src = (iroot == left ? mid + 1 : right);
        } else {
            src = (iroot == right ? mid : left);
        }

        /*
        ** calculate the start and end of message segment
        */
        if (src <= mid) {
            dstart = left * dstride;
            dend = (mid + 1 ) * dstride;
        } else {
            dstart = (mid + 1) * dstride;
            dend = (right + 1) * dstride;
        }
        len = data_ptrs[dend] - data_ptrs[dstart];
        offset = data_ptrs[dstart] - data_ptrs[0];

        /*
        ** capture sender/receiver information
        */
        if (myrank==iroot) {
            r_recs[n_rrecs].rcvr_rank = _my_rank;
            r_recs[n_rrecs].sndr_rank = 
                (list ? list[src * lstride] : src * lstride);
            r_recs[n_rrecs].offset = offset;
            r_recs[n_rrecs].len = len;
            n_rrecs++;
        } else if (myrank==src) {
            /* There is at most one send */
            s_rec.sndr_rank = _my_rank;
            s_rec.rcvr_rank = 
                (list ? list[iroot * lstride] : iroot * lstride);
            s_rec.offset = offset;
            s_rec.len = len;
            n_srecs++;
        }


        /*
        ** subdivide mesh partitions
        */
        if (myrank <= mid) {
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
    ** Perform communication
    */
    rc = c_st->proto.p_init( GROUP, itype, data_ptrs[0], 
                    data_ptrs[0], 1, data_len, c_st->user_struct, &handle );
    flow_control = (data_len/2 > PROTOCOL_THRESHOLD ? TRUE : FALSE);
    if (!rc && n_rrecs) { 
        rc = c_st->proto.p_vrecv( NULL, flow_control, 
                    n_rrecs, r_recs, &handle); 
    }
    if (!rc && n_srecs) { 
        rc = c_st->proto.p_vsend( flow_control, n_srecs, &s_rec, &handle); 
    }
    if (!rc) { 
        rc = c_st->proto.p_cleanup(&handle); 
    }
    coll_assert(rc,"_gather()","Failed proto comms in _gather()");


    return (rc);
}


/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  SCATTER
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/


/*---------------------------------------------------------------------------*/
/*
**=============================
** Scatter timing routine
**=============================
*/

FLOAT64 
_scatter_time(INT32 p ATTR_UNUSED, INT32 n ATTR_UNUSED,
	      INT32 interleave ATTR_UNUSED, BOOLEAN ishort ATTR_UNUSED)
{
    return (0.0);
}


/*---------------------------------------------------------------------------*/
/*
**=============================
** Scatter routine
**=============================
*/

INT32
_scatter(
    CHAR * data_ptrs[], INT32 dstride,
    INT32 type,
    INT32 list[], INT32 lstride, INT32 ngroup,
    INT32 myrank, INT32 rootrank,
    COLL_STRUCT *c_st)
{
INT32   rc;
INT32   left, right, mid;
INT32   dstart, dend, len;
INT32   iroot, dest;
INT32   data_len, offset;
INT32     n_srecs;
INT32     n_rrecs;
DATA_REC  r_rec;
DATA_REC *s_recs;
BOOLEAN   flow_control;
CHAMELEON itype;
VSENDRECV_HANDLE handle;

    rc = 0; 

    /* Quick return */
    if (ngroup <= 1) {
        return (ngroup == 1 ? 0 : -1);
    }

    /*
    ** Initialization
    */
    data_len = data_ptrs[ngroup * dstride] - data_ptrs[0];
    itype.ints.i0 = COLLCORE_SCATTER;
    if (type <= 0) {
        itype.ints.i1 = COLLCORE_SCATTER;
    } else {
        itype.ints.i1 = type;
    }
    s_recs = g_recs;
    n_srecs = 0;
    n_rrecs = 0;
    iroot = rootrank;
    left = 0;
    right = ngroup - 1;


    /*
    ** Communication Loop
    */
    while (left < right) {
        mid = (left + right) / 2;

        /*
        ** If the root is the leftmost node in the partition, send to the
        ** left node in the other partition (mid + 1).  If the root is the
        ** right node in the partition, send to the right node in the other
        ** (mid).  Otherwise, send to the extreme right or left node
        */
        if (iroot <= mid) {
            dest = (iroot == left ? mid + 1 : right);
            dstart = dstride * (mid + 1);
            dend   = dstride * (right + 1);
        } else {
            /* iroot > mid */
            dest = (iroot == right ? mid : left);
            dstart = dstride * left;
            dend   = dstride * (mid + 1);
        }

        /*
        ** Calculate the length of the message to be transfered. Data must be
        ** contiguous in core.  Calculate the offset for the data.
        */
        len = data_ptrs[dend] - data_ptrs[dstart];
        offset = data_ptrs[dstart] - data_ptrs[0];

        /*
        ** Capture sender/receiver information
        */
        if (myrank == iroot) {
            s_recs[n_srecs].sndr_rank = _my_rank;
            s_recs[n_srecs].rcvr_rank = 
                (list ? list[dest * lstride] : dest * lstride);
            s_recs[n_srecs].offset = offset;
            s_recs[n_srecs].len = len;
            n_srecs++;
        } else if (myrank == dest) {
            /* There is at most one recv */
            r_rec.rcvr_rank = _my_rank;
            r_rec.sndr_rank = 
                (list ? list[iroot * lstride] : iroot * lstride);
            r_rec.offset = offset;
            r_rec.len = len;
            n_rrecs++;
        }

        /*
        ** Split each partition into two subpartitions of roughly equal size.
        ** All nodes in the partition with the destination node use it as
        ** their root for the next transfer.  The other partition maintains
        ** its current root.
        ** 
        ** Left partitions will be slightly larger by rounding up
        */
        if (myrank <= mid) {
            iroot = (iroot > mid ? dest : iroot);
            right = mid;
        } else {
            /* myrank > mid */
            iroot = (iroot <= mid ? dest : iroot);
            left = mid + 1;
        }
    }


    /*
    ** Perform communication
    */
    rc = c_st->proto.p_init( GROUP, itype, data_ptrs[0], 
                    data_ptrs[0], 1, data_len, c_st->user_struct, &handle );
    flow_control = (data_len/2 > PROTOCOL_THRESHOLD ? TRUE : FALSE);
    if (!rc && n_rrecs) { 
        rc = c_st->proto.p_vrecv( NULL, flow_control, n_rrecs, 
                &r_rec, &handle); 
    }
    if (!rc && n_srecs) { 
        rc = c_st->proto.p_vsend( flow_control, n_srecs, s_recs, &handle); 
    }
    if (!rc) { 
        rc = c_st->proto.p_cleanup(&handle); 
    }
    coll_assert(rc,"_scatter()","Failed proto comms in _scatter()");


    return (rc);
}


/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  REDUCE
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

/*---------------------------------------------------------------------------*/
/*
**=============================
** Reduce timing routine
**=============================
*/

FLOAT64 
_reduce_time(INT32 p, INT32 n, INT32 interleave, BOOLEAN ishort) 

{
    FLOAT64 alpha, beta, gamma;
    FLOAT64 c_logp, dn, dp;
    FLOAT64 latency, transfer, compute;

    c_logp = (FLOAT64) _ceillogp( p );
    dn = (FLOAT64) n;
    dp = (FLOAT64) p;

    if (ishort) {
        alpha = ALPHA1way;
        beta = interleave * _beta(n);
        gamma = 0.0;                /* Computation is free */

        /* Fan-in time */
        latency  = (c_logp) * alpha;
        transfer = (c_logp * dn) * beta;
        compute  = (c_logp * dn) * gamma;

        return (latency + transfer + compute);
    } else {

        return (0.0);
    }

}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Reduce short routine
**=============================
*/

INT32
_reduce_short(
              PUMA_OP op,
              CHAR * data, INT32 data_elements, INT32 data_size,
              INT32 type,
              INT32 list[], INT32 lstride, INT32 ngroup,
              INT32 myrank, INT32 rootrank,
              CHAR work[], COLL_STRUCT *c_st)
{
INT32           mymaskrank;        /* My logical rank */
INT32                partner_m;        /* Partner's masked rank */
INT32                partner_a;        /* Partner's actual rank */
INT32           byte_len;        /* Length of message to be sent */
INT32           rc, i;
INT32             n_rrecs;
INT32             n_srecs;
DATA_REC          s_rec;
DATA_REC         *r_recs;
BOOLEAN           flow_control;
CHAMELEON        itype;
VSENDRECV_HANDLE handle;

    rc = 0;

    /* Quick return */
    if (ngroup <= 1) {
        return (ngroup == 1 ? 0 : -1);
    }

    /*
    ** Initialization
    */
    r_recs = g_recs;
    n_rrecs = 0;
    n_srecs = 0;
    itype.ints.i0 = COLLCORE_REDUCE;
    itype.ints.i1 = (type == -1 ? COLLCORE_REDUCE : type);
    byte_len = data_size * data_elements;

    /*
    ** Remap my actual rank to the new list where root is zero
    */
    mymaskrank = (myrank - rootrank + ngroup) % ngroup;

    /*
    ** Use standard bit-toggled binary fan-in
    */
    for (i = 1; i < ngroup; i <<= 1) {
        /*
        ** Determine my partner's masked rank and then
        ** find the actual rank
        */
        partner_m = mymaskrank ^ i;
        partner_a = (partner_m + rootrank) % ngroup;


        /*
        ** If my masked rank is less than my partner's, s/he is 
        ** sending to me.  Otherwise, I am sending to her/him and 
        ** returning afterwards.
        */
        if (partner_m > mymaskrank) {
            /*
            ** Receive from my partner 
            */
            if (partner_m < ngroup) {
                r_recs[n_rrecs].rcvr_rank = _my_rank;
                r_recs[n_rrecs].sndr_rank = 
                    (list ? list[partner_a * lstride] : partner_a * lstride);
                r_recs[n_rrecs].offset = 0;
                r_recs[n_rrecs].len = byte_len;
                n_rrecs++;
            }
        } else {
            /*
            ** Send to my partner
            ** There is at most one send 
            */
            s_rec.sndr_rank = _my_rank;
            s_rec.rcvr_rank = 
                (list ? list[partner_a * lstride] : partner_a * lstride);
            s_rec.offset = 0;
            s_rec.len = byte_len;
            n_srecs++;
            break;
        }
    }

    /*
    ** Perform communication
    */
    flow_control = TRUE;
    rc = c_st->proto.p_init( GROUP, itype, work, data, data_size, 
                    byte_len, c_st->user_struct, &handle );
    if (!rc && n_rrecs) { 

        rc = c_st->proto.p_vrecv( op, flow_control, 
                    n_rrecs, r_recs, &handle); 
    }
    if (!rc && n_srecs) { 

        rc = c_st->proto.p_vsend( flow_control, n_srecs, &s_rec, &handle); 
    }
    if (!rc) { 
        rc = c_st->proto.p_cleanup(&handle); 
    }
    coll_assert(rc,"_reduce_short()","Failed proto comms in _reduce_short()");

    return (rc);
}


/*---------------------------------------------------------------------------*/
/*
**=============================
** Reduce long 
**=============================
*/

INT32
_reduce_long(PUMA_OP op ATTR_UNUSED, CHAR *data ATTR_UNUSED,
	    INT32 len ATTR_UNUSED, INT32 data_size ATTR_UNUSED,
	    INT32 type ATTR_UNUSED, INT32 *list ATTR_UNUSED,
	    INT32 lstride ATTR_UNUSED, INT32 ngroup ATTR_UNUSED,
            INT32 myrank ATTR_UNUSED, INT32 rootrank ATTR_UNUSED,
	    COLL_STRUCT *c_st ATTR_UNUSED)

{

INT32 rc;

    rc = 0;
    return (rc);
}



/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  DISTRIBUTED REDUCE
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/


/*---------------------------------------------------------------------------*/
/*
**======================================
** Distributed reduce timing routine
**======================================
*/

FLOAT64 
_dist_reduce_time(INT32 p, INT32 n, INT32 interleave, BOOLEAN ishort) 

{
FLOAT64 dn,dp;
FLOAT64 latency, transmit, computation, time;
FLOAT64 alpha, beta, gamma;

    dn                 = n;
    dp                 = p;
    time         = 0.0;
    alpha        = ALPHA1way;
    beta        = (FLOAT64)interleave * _beta(n);
    gamma        = 0.0;        /* Assumes computation is free */

    if (ishort) {
        time = 0.0;
    } else {        
        /* Long */

        /* Long: (p-1) a + (p-1)/ p * n b + (p-1) / p * n g */
        latency = (dp-1.0) * alpha;
        transmit = (dp-1.0) * dn * beta / dp;
        computation = (dp-1.0) * dn * gamma / dp;

        time = latency + transmit + computation;
    }

    return (time);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Distributed reduce short 
**=============================
*/

INT32
_dist_reduce_short(PUMA_OP op ATTR_UNUSED, CHAR **data_ptrs ATTR_UNUSED,
	    INT32 dstride ATTR_UNUSED, INT32 data_size ATTR_UNUSED,
	    INT32 type ATTR_UNUSED, INT32 *list ATTR_UNUSED,
	    INT32 lstride ATTR_UNUSED, INT32 ngroup ATTR_UNUSED,
            INT32 myrank ATTR_UNUSED, COLL_STRUCT *c_st ATTR_UNUSED)

{

INT32 rc;

    rc = 0;
    return (rc);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** Distributed reduce long 
**=============================
*/


INT32
_dist_reduce_long(
        PUMA_OP op, 
        CHAR *data_ptrs[], INT32 dstride, INT32 data_size,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup, INT32 myrank,
        CHAR work[], COLL_STRUCT *c_st, BOOLEAN send_uplist) 

{
INT32   i,rc;
INT32   send_bkt;
INT32   recv_bkt;
INT32   dest;
INT32   src;

#ifndef PORTAL_COLLECTIVE
INT32   rsrc, rlen, rtype;
#endif

INT32   elements;
INT32   m_length;
INT32   recv_len;
UINT32  offset;
BOOLEAN found;
CHAR   *tmp_addr;
IND_MD_BUF_DESC *msg_buf_desc;
CHAMELEON mbits;
    
    /* 
    ** Quick return
    */
    rc = 0;
    if (ngroup <= 1) { 
        return (ngroup==1 ? 0 : -1);
    }


    /*
    ** calculate which node to send to and which node to receive from.
    */
    if ( send_uplist ) {
        src = ((myrank + ngroup - 1) % ngroup) * lstride;
        dest  = ((myrank + 1) % ngroup) * lstride;;
        src   = ( list ? list[ src ]  : src );
        dest  = ( list ? list[ dest ] : dest );
    } else {
        src = ((myrank + 1) % ngroup) * lstride;
        dest  = ((myrank + ngroup - 1) % ngroup) * lstride;
        src   = ( list ? list[ src ]  : src );
        dest  = ( list ? list[ dest ] : dest );
    }


    /* 
    ** Initialization 
    */
#ifdef PORTAL_COLLECTIVE
        /*
        ** Portal specific initialization: Setup single block 
        ** for workspace, ping source and wait for handshake 
        ** from receiver.  Once received, send until all the 
        ** data is taken care of.
        */

        /* 
        ** Initialize data 
        */
        recv_len = 0;


        /* 
        ** Setup the single block entry.  Work space is modeled
        ** after the buffer pointed to by data_ptrs, though it
        ** IS different memory.
        */
        m_length = data_ptrs[ngroup * dstride + 1] - data_ptrs[0];
        SINGLE_MD( Coll_Bucket_Ptl ).buf = work;
        SINGLE_MD( Coll_Bucket_Ptl ).buf_len = m_length;
        SPTL_ACTIVATE( Coll_Bucket_Ptl );


        /* 
        ** Ping the sender 
        */
        mbits.ints.i0=COLLCORE_DISTREDUCE;
        mbits.ints.i1=(type==-1 ? COLLCORE_DISTREDUCE : type);
        rc = portal_send(
            NULL, 0, 0, GROUP, src, Coll_Hdr_Ptl,
            &mbits, NULL, 0, 0, 0, NULL, NULL);
        coll_assert(rc,
            "_dist_reduce_long()",
            "Failed to ping source in _dist_reduce_long()");


        /* 
        ** Wait for ping from receiver.
        **
        ** Note that for MPI, there may be overlapped communicators
        ** each trying to perform this operation.  We must be prepared
        ** to have multiple out-of-order requests coming in from
        ** other nodes.  Thus the do-while loop.
        */
        msg_buf_desc = NULL;
        found = FALSE;
        while (1) {
            sptl_quiescence(Coll_Hdr_Ptl);
            rc = sptl_ind_msg_probe( &IND_MD( Coll_Hdr_Ptl ),
                                     IND_CIRC_SV_HDR,
                                     &msg_buf_desc,
                                     FALSE );
            if ((!rc) && (msg_buf_desc == NULL)) {
                LINK_FIND_N_FREE( mbits,found );
                if (!found) {
                    sptl_quiescence(Coll_Hdr_Ptl);
                    rc = sptl_ind_msg_probe( &IND_MD( Coll_Hdr_Ptl ),
                                             IND_CIRC_SV_HDR,
                                             &msg_buf_desc,
                                             TRUE );
                } else {
                    /*
                    ** Found it in my unexpected list.
                    */
                    break;
                }
            }
            if ((!rc) && 
#ifdef __LONG_LONG__
                    (msg_buf_desc->hdr.dst_mbits.ll != mbits.ll) ){
#else /* __LONG_LONG__ */
                    ((msg_buf_desc->hdr.dst_mbits.ints.i1 != mbits.ints.i1) || 
                    (msg_buf_desc->hdr.dst_mbits.ints.i0 != mbits.ints.i0)) ) {
#endif /* __LONG_LONG__ */
                HDR_LINK_ADD( msg_buf_desc );
                sptl_quiescence(Coll_Hdr_Ptl);
                rc = sptl_ind_msg_free( &IND_MD(Coll_Hdr_Ptl), 
                                        IND_CIRC_SV_HDR, 
                                        msg_buf_desc );
                if (rc) { break; }
            } else {
                if (!rc) {
                    sptl_quiescence(Coll_Hdr_Ptl);
                    rc = sptl_ind_msg_free( &IND_MD(Coll_Hdr_Ptl), 
                                            IND_CIRC_SV_HDR, 
                                            msg_buf_desc );
                }
                break;
            }
        } 
        coll_assert(rc, "_dist_reduce_long()", 
            "Failed probing for ping in _dist_reduce_long()");


        /*
        ** Free the buffer for reuse and reset the probe.
        */
        rc = sptl_ind_rst_probe( &IND_MD(Coll_Hdr_Ptl) ); 
        coll_assert(rc, "_dist_reduce_long()",
            "Failed to free ind buffers in _dist_reduce_long()");


#endif /* PORTAL_COLLECTIVE */


    
    /* 
    ** Communication Loop 
    */ 
    type = (type == -1 ? COLLCORE_DISTREDUCE : type);
    for( i=ngroup-1; i>0; i--) {
        /* 
        ** Calculate the buffers to be sent/received
        */
        if (send_uplist) {
            send_bkt  = ((myrank + i) % ngroup) * dstride;
            recv_bkt = ((send_bkt + ngroup - 1) % ngroup) * dstride;
        } else {
            send_bkt  = ((myrank + ngroup - i) % ngroup) * dstride;
            recv_bkt = ((send_bkt + 1) % ngroup) * dstride;
        }


        /*
        ** calculate the length of the bucket being passed
        */
        m_length = (data_ptrs[send_bkt + dstride] - data_ptrs[send_bkt]);


        /*
        ** Send a portion
        */
#ifdef PORTAL_COLLECTIVE
            offset = data_ptrs[send_bkt] - data_ptrs[0];
            rc = portal_send(
                data_ptrs[send_bkt], m_length, offset, 
                GROUP, dest, Coll_Bucket_Ptl,
                &mbits, NULL, 0, 0, 0, NULL, NULL);
            coll_assert(rc,"_dist_reduce_long()",
                "Error sending message to heap for _dist_reduce_long()");
#else /* PORTAL_COLLECTIVE */
            /* 
            ** Send to my dest and return on an error 
            */
            if ((rc = _nsend(data_ptrs[send_bkt], m_length, dest, type, 
                NULL, NULL)) < 0) {
                return (rc);
            }
#endif /* PORTAL_COLLECTIVE */


        /* 
        ** Calculate the length of the work data 
        */
        m_length = data_ptrs[recv_bkt + dstride] - data_ptrs[recv_bkt];


#ifdef PORTAL_COLLECTIVE
            /*
            ** The next batch of data must have arrived before we
            ** may procede.  recv_len keeps track of the total number
            ** of bytes that we should expect to have received
            */
            recv_len += m_length;
            while (SINGLE_MD(Coll_Bucket_Ptl).msg_cnt < ngroup-i);

#else /* PORTAL_COLLECTIVE */
            /*
            ** Wait for the message, store it in the temp space and
            ** return on an error
            */
            rtype = type;
            rsrc = src;
            rlen = m_length;
            if ((rc = _nrecv(work, &rlen, &rsrc, &rtype, NULL, NULL))<0) {
                return (rc);
            }
#endif /* PORTAL_COLLECTIVE */


        /* 
        ** Perform the operation and store the result in the vector 
        ** If the operation is NULL, do nothing on it.  The caller 
        ** must ensure that m_length is always a multiple of data_size.
        */
        elements = m_length / data_size;


#ifdef PORTAL_COLLECTIVE
            tmp_addr = work + (data_ptrs[recv_bkt] - data_ptrs[0]);
            op( tmp_addr, data_ptrs[recv_bkt], &elements, c_st->user_struct);

#else /* PORTAL_COLLECTIVE */
            op(work, data_ptrs[recv_bkt], &elements, c_st->user_struct);
#endif /* PORTAL_COLLECTIVE */
    }

#ifdef PORTAL_COLLECTIVE
        /*
        ** Cleanup the portal structures for the next time.
        ** Make sure all the data has arrived.
        */


#ifdef NOT_NEEDED
        /* 
        ** Wait for enough data 
        */
        m_length = (data_ptrs[dstride * ngroup] - data_ptrs[0]) -
            (data_ptrs[dstride * (myrank + 1)] - data_ptrs[dstride * myrank]);
        while (SINGLE_MD(Coll_Bucket_Ptl).rw_bytes < m_length);
#endif /* NOT_NEEDED */


        /*
        ** Release the portal
        */
        SPTL_DEACTIVATE( Coll_Bucket_Ptl );
        sptl_quiescence( Coll_Bucket_Ptl );
        SINGLE_MD( Coll_Bucket_Ptl ).buf = NULL;
        SINGLE_MD( Coll_Bucket_Ptl ).buf_len = 0;
        SINGLE_MD(Coll_Bucket_Ptl).rw_bytes = -1;
        SINGLE_MD(Coll_Bucket_Ptl).msg_cnt -= ngroup-1;

#endif /* PORTAL_COLLECTIVE */

    return (0);
}



/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  ALL-REDUCE
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

/*---------------------------------------------------------------------------*/
/*
**=============================
** All-reduce timing routine
**=============================
*/

FLOAT64 
_all_reduce_time(INT32 p ATTR_UNUSED, INT32 n ATTR_UNUSED,
		 INT32 interleave ATTR_UNUSED, BOOLEAN ishort ATTR_UNUSED)
{
    return (0.0);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** All-reduce short 
**=============================
*/

INT32
_all_reduce_short(PUMA_OP op, 
            CHAR *data, INT32 data_elements, INT32 data_size, INT32 type,
            INT32 *list, INT32 lstride, INT32 ngroup, 
            INT32 myrank, CHAR *work, COLL_STRUCT *c_st) 

{
INT32 rc;
INT32 iroot;

    rc = ESUCCESS;
    iroot = 0;


    rc = _reduce_short( op, data, data_elements, data_size, type,
        list, lstride, ngroup, myrank, iroot, work, c_st);

    coll_assert(rc,"_all_reduce_short()", "Error detected from _reduce_short()");

    rc = _bcast_short( data, data_elements*data_size, type,
        list, lstride, ngroup, myrank, iroot, c_st);


    coll_assert(rc,"_all_reduce_short()", "Error detected from _bcast_short()");

    return (rc);
}

/*---------------------------------------------------------------------------*/
/*
**=============================
** All-reduce long 
**=============================
*/

INT32
_all_reduce_long(PUMA_OP op, 
    CHAR *data_ptrs[], INT32 dstride, INT32 data_size, INT32 type,
    INT32 *list, INT32 lstride, INT32 ngroup, 
    INT32 myrank, CHAR *work, 
    COLL_STRUCT *c_st, BOOLEAN send_uplist) 
{
INT32 rc;

    rc = 0;

    rc = _dist_reduce_long( op, data_ptrs, dstride, data_size, type,
        list, lstride, ngroup, myrank, work, c_st, send_uplist);
    coll_assert(rc,"_all_reduce_long()", 
        "Error detected from _dist_reduce_long()");

    rc = _collect_long( data_ptrs, dstride, type, list, lstride, ngroup,
        myrank, c_st, send_uplist);
    coll_assert(rc,"_all_reduce_long()", 
        "Error detected from _collect_long()");

    return (rc);
}


/*****************************************************************************/

/*
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
**==========================================================================
**
**  MISCELLANEOUS SUPPORT ROUTINES
**
**==========================================================================
**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

/*---------------------------------------------------------------------------*/
/*
**=========================================
** Collective Core Structure Initialization
**=========================================
*/

INT32
_coll_struct_init(COLL_STRUCT *st) 
{
INT32 rc;
INT32 my_rank, size, clog2size;
INT32 *list, *divlist;
#ifdef DIM_ARRAYS
    INT32 *dim_lda;
    INT32 lidx;
    INT32 i,j,k,l,tmp_mod;
    INT32 **dim_list, *dim_nlist, *dim_rank;
    INT32 idx[NDIMS];
#endif /* DIM_ARRAYS */
COLL_GRP *cgrp;

    /*
    ** Initialize variables
    */
    size          = _my_nnodes;
    my_rank       = _my_rank;
    clog2size     = _ceillogp(size);
    cgrp          = &(st->cgrp);

    st->cntr        = 0;
    st->user_struct = NULL;
    cgrp->ref_count = 1;


    /*
    ** Malloc space
    */
    list     = cgrp->list    = (INT32 *)malloc(size * sizeof(INT32));
    divlist  = cgrp->divlist = (INT32 *)malloc((size/2 + 1) * sizeof(INT32));
    if ( (list == NULL) || (divlist == NULL) ) {
        free(list);
        free(divlist);
        ERRNO = ENOMEM;
        coll_assert(-1,"_coll_struct_init()","Failed to malloc list, divlist");
        return (-1);
    }

    /*
    ** Malloc stack space if necessary 
    */
    if (size > 1) {
        cgrp->stack    = (INT32 *)
            malloc(HYB_NSTACK_ENTREES*clog2size*sizeof(INT32));
        cgrp->strategy = (INT32 *)malloc((clog2size+1)*sizeof(INT32));
        if ( (cgrp->stack == NULL) || (cgrp->strategy == NULL) ) {
            free(list);
            free(divlist);
            free(cgrp->strategy);
            free(cgrp->stack);
            ERRNO = ENOMEM;
            coll_assert(-1,"_coll_struct_init()",
                "Failed to malloc stack, strategy");
            return (-1);
        }
    } else {
        cgrp->stack = NULL;
        cgrp->strategy = NULL;
    }

#ifdef DIM_ARRAYS
    /*
    ** Get dimensions and calculate lda's.
    ** Ordering is assumed to be counted from
    ** Dimension 0 through Dimension NDIMS-1.
    */
    dim_lda = cgrp->dim_lda;
    dim_nlist = cgrp->dims.dim_nlist;
    for (i=0; i<NDIMS; i++) {
        dim_lda[i]=1;
    }
    for (i=0; i<NDIMS; i++) {
        dim_nlist[i] = DEVICE_DIMS( i );
        for (j=NDIMS-1; j>=i; j--) {
            dim_lda[j] *= dim_nlist[i];
        }
    }

    /*
    ** Calculate my dimension rank
    */
    dim_rank = cgrp->dims.dim_rank;
    dim_rank[0] = my_rank%dim_lda[0];
    for (i=1; i<NDIMS; i++) {
        tmp_mod = my_rank%dim_lda[i];
        dim_rank[i]    = tmp_mod/dim_lda[i-1];
    }

        /*
        ** Malloc dimension lists
        */
        dim_list  = cgrp->dims.dim_list;
        for (i=0;i<NDIMS;i++) {
            dim_list[i] = (INT32 *)malloc(dim_nlist[i] * sizeof(INT32));
            if (dim_list[i] == NULL) {
                free(list);
                free(divlist);
                free(cgrp->strategy);
                free(cgrp->stack);
                for (k=0;k<NDIMS;k++) {
                    free(dim_list[k]);
                }
                ERRNO = ENOMEM;
                coll_assert(-1,"_coll_struct_init()",
                    "Failed to malloc dimension lists");
                return (-1);
            }
        }

        /*
        ** Initialize the divisors list
        */
        for (i=1, lidx=0; i<=size/2; i++) {
            if ((size%i) == 0) {
                divlist[lidx++]= i;
            }
        }
        divlist[lidx++] = size;
        cgrp->ndivlist = lidx;

        /*
        ** Setup dimension lists
        */
        for (i=0; i<size; list[i]=i, i++);
        for (j=0; j<NDIMS; idx[j]=0, j++);
        for (i=0; i<size; i++) {
            for (j=0; j<NDIMS; j++) {
                l=0;
                if ((j!=0) && ((i%dim_lda[0]) == dim_rank[0])) {
                    l++;
                }
                for (k=1; k<NDIMS; k++) {
                    tmp_mod  = i%dim_lda[k];
                    if ((j!=k) && 
                        ((tmp_mod/dim_lda[k-1]) == dim_rank[k])) {
                        l++;
                    }
#ifdef DEBUG_HYBRID_INIT_D1
                        if (my_rank==0) {
                            printf("i=%1d j=%1d k-1=%1d l=%1d lda[k-1]=%3d\
                                dim_rank[k]=%1d\n",
                            i,j,k-1,l,dim_lda[k-1],dim_rank[k]);
                        }
#endif /* DEBUG_HYBRID_INIT_D1 */
                }
                if (l==NDIMS-1) {
                    dim_list[j][idx[j]++] = i;
#ifdef DEBUG_HYBRID_INIT_D1
                        if (my_rank==0) {
                            printf("j=%1d idx=%1d i=%1d dim_entry=%1d\n",
                                j,idx[j]-1,i,dim_list[j][idx[j]-1]);
                        }
#endif /* DEBUG_HYBRID_INIT_D1 */
                }
            }
        }
#else /* DIM_ARRAYS */
#endif /* DIM_ARRAYS */

    cgrp->nlist   = size;
    cgrp->my_rank = my_rank;


    /*
    ** Initialize default portal protocol for collective ops
    */
    rc = _vcoll_init_func( def_vinit, def_vsend, def_vrecv, def_vtest,
                            def_vcleanup, &(st->proto) );
    coll_assert(rc,"_coll_struct_init()","Failed to init default protocol");


    return 0;

}


/*---------------------------------------------------------------------------*/


VOID
dump_coll_struct(FILE *fp, COLL_STRUCT *st) 

{
#ifdef LINUX_PORTALS
    (void) fp;
    (void) st;
#else
INT32 i,j;
INT32 str_len;
static INT32 str_max=8192;
static char s[8192];
static char stmp[160];
COLL_GRP *cgrp;

    cgrp = &(st->cgrp);

    sprintf(s,"*------------------------------------------------------*\n");
    sprintf(stmp,"coll_struct = 0x%08x\n",st);
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    sprintf(stmp,"ref_count= %d\n",cgrp->ref_count);
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    sprintf(stmp,"\tlist =  (nlist = %d) (myrank = %d)\n",
                cgrp->nlist, cgrp->my_rank);
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    sprintf(stmp,"\t");
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    for (i=0; i<(cgrp->nlist); i++) {
        sprintf(stmp,"%3d ",cgrp->list[i]);
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
    }
    sprintf(stmp,"\n");
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    sprintf(stmp,"\tdivlist =  (ndivlist = %d)\n", cgrp->ndivlist);
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    sprintf(stmp,"\t");
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    for (i=0; i<cgrp->ndivlist; i++) {
        sprintf(stmp,"%3d ",cgrp->divlist[i]);
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
    }
    sprintf(stmp,"\n");
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
    else { fprintf(fp,"%s",s); return; }
    for (j=0; j<3; j++) {
        sprintf(stmp,
            "\tdim_list[%1d]= (dim_nlist= %d) (dim_rank= %d) (dim_lda= %d)\n",
            j, cgrp->dims.dim_nlist[j], cgrp->dims.dim_rank[j], 
            cgrp->dim_lda[j]);
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
        sprintf(stmp,"\t");
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
        for (i=0; i<cgrp->dims.dim_nlist[j]; i++) {
            sprintf(stmp,"%3d ",cgrp->dims.dim_list[j][i]);
            str_len = strlen(stmp);
            if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
            else { fprintf(fp,"%s",s); return; }
        }
        sprintf(stmp,"\n");
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
    }
    if (cgrp->strategy) {
        sprintf(stmp,"\tstrategy = \n");
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
        sprintf(stmp,"\t");
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
        i=0;
        while (cgrp->strategy[i]!=0) {
            sprintf(stmp,"%3d ",cgrp->strategy[i++]);
            str_len = strlen(stmp);
            if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
            else { fprintf(fp,"%s",s); return; }
        }
        sprintf(stmp,"\n");
        str_len = strlen(stmp);
        if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }
        else { fprintf(fp,"%s",s); return; }
    }
    sprintf(stmp,"*------------------------------------------------------*\n");
    str_len = strlen(stmp);
    if (str_len + strlen(s) < str_max-1) { strncat(s,stmp,str_len); }

    fprintf(fp,"%s",s);
    return;
#endif
}


/*---------------------------------------------------------------------------*/
/*
**==================================
** Ceiling log-base-two function
**==================================
*/

INT32
_ceillogp(INT32 p)
{
INT32 c_logp;
INT32 cnt;

    for (cnt=0, c_logp=1; c_logp < p; cnt++, c_logp <<= 1);

    return (cnt);
}

/*---------------------------------------------------------------------------*/
/*
**==================================
** Make chameleon support function
**==================================
*/

CHAMELEON
mkchameleon(UINT32 high, UINT32 low)
{
    CHAMELEON ret;
    ret.ints.i0 = high;
    ret.ints.i1 = low;
    return ret;
}

/*****************************************************************************/
#endif
