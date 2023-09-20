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
** $Id: collcore.h,v 1.5 2001/02/16 05:36:16 lafisk Exp $
*/

#ifndef CLCOLLCORE_H
#define CLCOLLCORE_H
#include "defines.h"

TITLE(collcore_h, "@(#) $Id: collcore.h,v 1.5 2001/02/16 05:36:16 lafisk Exp $");

/******************************************************************************/

#define PUMA_PROTO(name)                        \
        INT32 *name(                                \
        CHAR c_in[], CHAR c_out[],        \
        INT32 *len,                        \
        VOID *user_struct                \
        )

typedef PUMA_PROTO( (*PUMA_OP) );

/* Sumation */
PUMA_PROTO( PUMA_DSUM );
PUMA_PROTO( PUMA_ISUM );
PUMA_PROTO( PUMA_SSUM );

/* Products */
PUMA_PROTO( PUMA_DPROD );
PUMA_PROTO( PUMA_IPROD );
PUMA_PROTO( PUMA_SPROD );

/* Maximum */
PUMA_PROTO( PUMA_DMAX );
PUMA_PROTO( PUMA_IMAX );
PUMA_PROTO( PUMA_SMAX );

/* Minimum */
PUMA_PROTO( PUMA_DMIN );
PUMA_PROTO( PUMA_IMIN );
PUMA_PROTO( PUMA_SMIN );

/* Logical and Binary AND */
PUMA_PROTO( PUMA_LAND );
PUMA_PROTO( PUMA_BAND );

/* Logical and Binary OR */
PUMA_PROTO( PUMA_LOR );
PUMA_PROTO( PUMA_BOR );

/* No operation for sync calls */
PUMA_PROTO( PUMA_NULLOP );

/* Remove PUMA_PROTO if not needed */
#ifndef SAVE_PUMA_PROTO
# undef PUMA_PROTO
#endif

/*****************************************************************************/
/*****************************************************************************/
/*    The collective communication routines do not compile.  They use      */
/*    some structures from Portals 2.  But we'll keep them around because    */
/*    someone may find them useful after doing a small amount of             */
/*    modification.                                                          */
/*****************************************************************************/
/*****************************************************************************/


#if 0
/******************************************************************************/

#define COLLCORE_PING_BIT        (0x40000000)
#define COLLCORE_BCAST                (0xBFFFFFF0)   /* (-1000000500) */
#define COLLCORE_COLLECT        (0xBFFFFFF1)   /* (-1000000501) */
#define COLLCORE_SCATTER        (0xBFFFFFF2)   /* (-1000000502) */
#define COLLCORE_GATHER                (0xBFFFFFF3)   /* (-1000000503) */
#define COLLCORE_REDUCE                (0xBFFFFFF4)   /* (-1000000504) */
#define COLLCORE_DISTREDUCE        (0xBFFFFFF5)   /* (-1000000505) */

#define GROUP                        (-1)
#define TREE_HEAP_SIZE                (1024*256)
#define TREE_HEAP_ENTRIES        (4)
#define PROTOCOL_THRESHOLD        (1024)
#define NUM_DATA_RECS           (12)
#define IGNORE_ALL()                mkchameleon(0xffffffff,0xffffffff)
#define MATCH_ALL()                mkchameleon(0x00000000,0x00000000)

extern CHAMELEON mkchameleon(UINT32 high, UINT32 low);

extern UINT32                                single_mle;
extern UINT32                                reply_mle;
extern UINT32                                heap_mle;
extern PORTAL_INDEX                        Coll_Heap_Ptl;
extern PORTAL_INDEX                        Coll_Bucket_Ptl;
extern PORTAL_INDEX                        Coll_Hdr_Ptl;
extern IND_MD_BUF_DESC                        *req_bucket_desc;
extern MATCH_DESC_TYPE                        *tree_ml;
extern DYN_MALLOC_LINK_TYPE                *tree_heap_buf;


/******************************************************************************/


/* 
** Linked list of unexpected requests 
** Used by Coll_Hdr_Ptl collective ops.
*/

typedef struct ind_link {
    IND_MD_BUF_DESC ind_md_bd;
    struct ind_link *next;
    struct ind_link *prev;
} IND_LINK;

extern IND_LINK *ind_tail_link;
extern IND_LINK  ind_head_link;

#define HDR_LINK_INIT()                                                  \
    {                                                                        \
        ind_head_link.next = NULL;                                        \
        ind_head_link.prev = NULL;                                        \
        ind_tail_link = &ind_head_link;                                        \
    }

#define HDR_LINK_CLEANUP()                                                  \
    {                                                                        \
        IND_LINK *link;                                                        \
        while(ind_tail_link != &ind_head_link) {                        \
            link = ind_tail_link;                                        \
            ind_tail_link = ind_tail_link->prev;                        \
            free (link);                                                \
        }                                                                \
    }

#define HDR_LINK_ADD( msg_buf_desc )                                          \
    {                                                                        \
        IND_LINK *link;                                                        \
        link = (IND_LINK *)malloc( sizeof(IND_LINK) );                        \
        if (link==NULL) {                                                \
            errno = ENOMEM;                                                \
            coll_assert( -1, "HDR_LINK_ADD:",                                \
            "Failed to malloc link for unexpected Coll_Hdr_Ptl messages");  \
        }                                                                \
        memcpy( &(link->ind_md_bd),(msg_buf_desc),sizeof(IND_MD_BUF_DESC) ); \
        link->next=NULL;                                                \
        link->prev=ind_tail_link;                                        \
        ind_tail_link->next=link;                                        \
        ind_tail_link = link;                                                \
    }

#define LINK_FIND_N_FREE( mbits,found )                                        \
    {                                                                        \
        IND_LINK *link;                                                        \
        link = ind_head_link.next;                                        \
        (found) = FALSE;                                                \
        while( link != NULL) {                                                \
            if ((link->ind_md_bd.hdr.dst_mbits.ints.i1 == (mbits).ints.i1) && \
                    (link->ind_md_bd.hdr.dst_mbits.ints.i0 == (mbits).ints.i0) ) { \
                (found) = TRUE;                                                \
                if (link->next) {                                        \
                    link->next->prev = link->prev;                        \
                } else {                                                \
                    /* I am the last link, update last ptr */                 \
                    ind_tail_link = link->prev;                                \
                }                                                        \
                link->prev->next = link->next;                                \
                free (link);                                                \
                break;                                                        \
            } else {                                                        \
                link = link->next;                                        \
            }                                                                \
        }                                                                \
    }


/*****************************************************************************/

 
/*
** This macro is used by 2 stage collective operations such as 
** _allreduce_long() and _bcast_long() which must arbitrarily setup 
** their own data pointers.
** 
** This macro ensures that the data_ptrs pointers are formed, such that 
** each pointer into the buffer 'data' is aligned on a 'unit' boundary
** of length 'byte_len/ngroup' truncated to a multiple of 'unit'.
** It also takes care of special cases, such as cases where
** 'ngroup*unit < byte_len' and where 'remainder(byte_len/ngroup) != 0'.
*/
#define FORM_DATA_PTRS( data_ptrs, data, byte_len, unit, ngroup ) \
    { \
        INT32 ii; \
        INT32 subgr = (byte_len) / (unit); \
        if ((subgr / (ngroup)) == 0) { \
            for (ii=0; ii<=subgr; ii++) { \
                ((char **)data_ptrs)[ii] = &((char *)data)[ii*(unit)]; \
            } \
            for (ii=subgr+1; ii<=(ngroup); ii++) { \
                ((char **)data_ptrs)[ii] = &((char *)data)[(byte_len)]; \
            } \
        } else { \
            INT32 elements = (byte_len) / (unit) / (ngroup); \
            for (ii=0; ii<(ngroup); ii++) { \
                ((char **)data_ptrs)[ii] = &((char *)data)[ii*elements*(unit)]; \
            } \
            ((char **)data_ptrs)[(ngroup)] = &((char *)data)[(byte_len)]; \
        } \
    }
 


/******************************************************************************/

#define HYB_NSTACK_ENTREES            (6)

/***************************************************************************/


/*
** Defines, data structures, and function prototypes
** to support the collective ops mesh topology.
*/


/*
**  For portability reasons, the Puma collective communications support
**  NDIMS dimensional meshes.  The length of each dimension is to be
**  specified by the DEVICE_DIMS() macro.
*/
#define NDIMS           (3)
#define DEVICE_DIMS( i ) \
        ( (i)==0 ? _my_mesh_width : ((i)==1 ? _my_mesh_height : 1) )

#ifndef LINUX_PORTALS
#define DIM_ARRAYS
#endif /* LINUX_PORTALS */

#ifndef DIM_ARRAYS
    typedef struct {
        INT32  dim_base;        /* Base member (rank in master list)    */
        INT32  dim_stride;        /* Stride from base through master list */
        INT32  dim_len;                /* Number of members in this dimension  */
        INT32  dim_rank;        /* My rank in this dimension */
    } COLL_DIMDESC;
#else /* DIM_ARRAYS */
    typedef struct {
        INT32 *dim_list[NDIMS];        /* List members in each dim. */
        INT32  dim_nlist[NDIMS];/* Number of members in each dim. */
        INT32  dim_rank[NDIMS];        /* My rank in each dim. */
    } COLL_DIMDESC;
#endif /* DIM_ARRAYS */

 
typedef struct {
    INT32  ref_count;                /* reference counter */
    INT32 *list;                /* list of nodes in collective group */
    INT32  nlist;                /* length of list */
    INT32  my_rank;                /* my position in the list */
    INT32 *divlist;                /* list of divisors of nlist */
    INT32  ndivlist;                /* number of divisors in divlist */

#ifdef DIM_ARRAYS
        COLL_DIMDESC dims;      /* Dimension information */
#else /* DIM_ARRAYS */
        COLL_DIMDESC dims[NDIMS];        /* Dimension information */
#endif /* DIM_ARRAYS */

    INT32  dim_lda[NDIMS];        /* array of leading dimensions */
    INT32 *stack;                /* stack used for recursion */
    INT32 *strategy;                /* strategy array */
} COLL_GRP;
 

/***************************************************************************/


/*
** Defines, data structures, and function prototypes
** to support the collective ops message transfer protocol.  
*/
typedef struct {
    INT32 sndr_rank;
    INT32 rcvr_rank;
    INT32 offset;
    INT32 len;
} DATA_REC;

#define COLL_VINIT_PROTO( name )                 \
    INT32 name(                                        \
                    INT32 gid,                \
                    CHAMELEON mbits,                \
                    CHAR *in_buf,                \
                    CHAR *out_buf,                \
                    INT32 data_size,                \
                    INT32 buf_len,                \
                    VOID *user_struct,                \
                    VSENDRECV_HANDLE *handle );
typedef COLL_VINIT_PROTO( (*COLL_VINIT_F) );


#define COLL_VSEND_PROTO( name )                 \
    INT32 name(                                        \
                    BOOLEAN flow_control,        \
                    INT32 nrecs,                \
                    DATA_REC *recs,                \
                    VSENDRECV_HANDLE *handle);
typedef COLL_VSEND_PROTO( (*COLL_VSEND_F) )

#define COLL_VRECV_PROTO( name )                 \
    INT32 name(                                        \
                    PUMA_OP op,                        \
                    BOOLEAN flow_control,        \
                    INT32 nrecs,                \
                    DATA_REC *recs,                \
                    VSENDRECV_HANDLE *handle);
typedef COLL_VRECV_PROTO( (*COLL_VRECV_F) );

#define COLL_VTEST_PROTO( name )                 \
    INT32 name(                                        \
                    VSENDRECV_HANDLE *handle,        \
                    BOOLEAN wait);
typedef COLL_VTEST_PROTO( (*COLL_VTEST_F) );

#define COLL_VCLEANUP_PROTO( name )                 \
    INT32 name(                                        \
                    VSENDRECV_HANDLE *handle);
typedef COLL_VCLEANUP_PROTO( (*COLL_VCLEANUP_F) );

typedef struct {
    COLL_VINIT_F p_init;
    COLL_VSEND_F p_vsend;
    COLL_VRECV_F p_vrecv;
    COLL_VTEST_F p_test;
    COLL_VCLEANUP_F p_cleanup;
} COLL_PROTO_TYPE;


COLL_VINIT_PROTO (def_vinit);
COLL_VSEND_PROTO (def_vsend);
COLL_VRECV_PROTO (def_vrecv);
COLL_VTEST_PROTO (def_vtest);
COLL_VCLEANUP_PROTO (def_vcleanup);

extern DATA_REC                         *g_recs;
extern INT32 _vcoll_init_func(
                COLL_VINIT_F    vinit,
                COLL_VSEND_F    vsend,
                COLL_VRECV_F    vrecv,
                COLL_VTEST_F    vtest,
                COLL_VCLEANUP_F vcleanup,
                COLL_PROTO_TYPE *proto );


/******************************************************************************/


/*
** Combine topology and protocol into one structure
*/
typedef struct {
    INT32 cntr;
    COLL_GRP cgrp;
    COLL_PROTO_TYPE proto;
    VOID *user_struct;
} COLL_STRUCT;

extern INT32 _coll_struct_init( COLL_STRUCT *coll );


/******************************************************************************/


INT32
_bcast_short(
        CHAR data[], INT32 byte_len, 
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup,
        INT32 myrank, INT32 rootrank,
        COLL_STRUCT *core_st);

INT32
_bcast_long(CHAR **data_ptrs, INT32 dstride, INT32 type,
            INT32 *list, INT32 lstride, INT32 ngroup,
            INT32 myrank, INT32 rootrank,
            COLL_STRUCT *core_st, BOOLEAN dir_uplist);

INT32
_collect_short(
        CHAR *data_ptrs[], INT32 dstride,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup, 
        INT32 myrank, COLL_STRUCT *core_st);

INT32
_collect_long(
        CHAR *data_ptrs[], INT32 dstride,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup,
        INT32 myrank, COLL_STRUCT *core_st, BOOLEAN send_uplist);

INT32
_scatter(
        CHAR *data_ptrs[], INT32 dstride,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup, 
        INT32 myrank, INT32 rootrank,
        COLL_STRUCT *core_st);

INT32
_gather(CHAR *data_ptrs[], INT32 dstride, 
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup,
        INT32 myrank, INT32 rootrank,
        COLL_STRUCT *core_st);
 
INT32
_dist_reduce_long(
        PUMA_OP op, 
        CHAR *data_ptrs[], INT32 dstride, INT32 data_size,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup, INT32 myrank,
        CHAR work[], COLL_STRUCT *core_st, BOOLEAN send_uplist);

INT32
_reduce_short(
        PUMA_OP op,
        CHAR *data, INT32 data_elements, INT32 data_size,
        INT32 type,
        INT32 list[], INT32 lstride, INT32 ngroup,
        INT32 myrank, INT32 rootrank,
        CHAR work[], COLL_STRUCT *core_st);

INT32
_all_reduce_short(PUMA_OP op,
        CHAR *data, INT32 data_elements, INT32 data_size, INT32 type,
        INT32 *list, INT32 lstride, INT32 ngroup,
        INT32 myrank, CHAR *work, COLL_STRUCT *core_st);

INT32
_all_reduce_long(
        PUMA_OP op,
        CHAR *data_ptrs[], INT32 dstride, INT32 data_size, INT32 type,
        INT32 *list, INT32 lstride, INT32 ngroup, 
        INT32 myrank, CHAR *work, 
        COLL_STRUCT *core_st, BOOLEAN send_uplist);

INT32
_puma_collective_init(VOID);

INT32
_puma_collective_cleanup(VOID);

INT32
_ceillogp(INT32 p);


/******************************************************************************/


#define ASSERT_ON
#define VERBOSE_ERROR

#define TRACE_FUNC(s)   errno_trace(s)

#ifdef LINUX_PORTALS
#undef VERBOSE_ERROR
#undef TRACE_FUNC
#define TRACE_FUNC(s)   
#endif

#ifdef ASSERT_OFF
    /*
    ** Assert macro does nothing
    */
#define coll_assert( fname, str )                {}
#else /* ASSERT_OFF */
#ifdef VERBOSE_ERROR
        /*
        ** Assert macro calls perror
        */
#define coll_assert( rc, fname, str )                        \
        {                                                        \
            if ((rc)) {                                         \
                perror(str);                                        \
                perror_trace();                                        \
                return (rc);                                        \
            }                                                        \
        }
#else /* VERBOSE_ERROR */
        /*
        ** Assert macro returns error
        */
#define coll_assert( rc, fname, str )                        \
        {                                                        \
            if ((rc)) {                                         \
                TRACE_FUNC(fname);                                \
                return (rc);                                        \
            }                                                        \
        }
#endif /* VERBOSE_ERROR */
#endif /* ASSERT_OFF */

/******************************************************************************/
#endif
#endif /* COLLCORE_H */

