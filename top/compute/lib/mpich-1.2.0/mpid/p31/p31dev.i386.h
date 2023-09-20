#ifndef P31_DEV_H
#define P31_DEV_H

/******************************************************************************
 Standard include files
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <signal.h>

/******************************************************************************
 P31 include file
 *****************************************************************************/
#include "p30.h"
#include "myrnal.h"

#include "p31priv.h"
#include "p31_default_long_msg.i386.h"

/******************************************************************************
 Portals device macros
 *****************************************************************************/
#define MPID_CH_InitMsgPass                               MPID_P31_InitMsgPass

#define MPID_TAG_UB                                                  0x7fffffff
#define MPIDPATCHLEVEL                                                      1.0

#define P31_BARRIER_TAG                                          (-1000000506)

/* turn these into null macros */
#define MPID_P31_Check_incoming( dev, is_blocking )
#define MPID_DeviceCheck( is_blocking )

#define P31_DEFAULT_IRECV_MAX                                           1024
#define P31_DEFAULT_LONG_MAX                                            1024
#define P31_DEFAULT_UNEX_MAX                                            2048
#define P31_DEFAULT_UNEX_BLOCK_SIZE                              2*1024*1024
#define P31_DEFAULT_NUM_UNEX_BLOCKS                                        3

#define MPI_ENV_DEBUG_STRING                                      "MPI_DEBUG"
#define MPI_ENV_DUMP_QUEUES_STRING                          "MPI_DUMP_QUEUES"
#define MPI_ENV_IRECV_MAX_STRING                              "MPI_IRECV_MAX"
#define MPI_ENV_LONG_MAX_STRING                                "MPI_LONG_MAX"
#define MPI_ENV_UNEX_MAX_STRING                                "MPI_UNEX_MAX"
#define MPI_ENV_LONG_MSG_STRING                                "MPI_LONG_MSG"
#define MPI_ENV_INFO_STRING                                        "MPI_INFO"
#define MPI_ENV_DUMP_UNEX_Q_STRING                          "MPI_DUMP_UNEX_Q"
#define MPI_ENV_UNEX_BLOCK_SIZE_STRING                  "MPI_UNEX_BLOCK_SIZE"

/*
** Matchbits on posted recv
**
** 0123 4567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
**     |  context    |      source     |            message
**     |     id      |                 |              tag
**  ^--- protocol
*/

#if 1
#define P31_PROTOCOL_MASK 0xf000000000000000
#define P31_CONTEXT_MASK  0x0fff000000000000
#define P31_SOURCE_MASK   0x0000ffff00000000
#define P31_TAG_MASK      0x00000000ffffffff
#else
#define P31_PROTOCOL_MASK 0xf0000000
#define P31_CONTEXT_MASK  0x0ff00000
#define P31_SOURCE_MASK   0x000ff000
#define P31_TAG_MASK      0x00000fff
#endif

#if 1
#define P31_SHORT_MSG     0x1000000000000000
#define P31_LONG_MSG      0x2000000000000000
#define P31_READY_MSG     0x4000000000000000
#else
#define P31_SHORT_MSG     0x10000000
#define P31_LONG_MSG      0x20000000
#define P31_READY_MSG     0x40000000
#endif

/* for receiving */
#define P31_SET_SHORT_UNEX_BITS( match, ignore )                        \
{                                                                       \
    match  = P31_SHORT_MSG;                                             \
    ignore = P31_CONTEXT_MASK | P31_SOURCE_MASK | P31_TAG_MASK;         \
}

#define P31_SET_LONG_UNEX_BITS( match, ignore )                 \
{                                                               \
    match  = P31_LONG_MSG;                                      \
    ignore = P31_CONTEXT_MASK | P31_SOURCE_MASK | P31_TAG_MASK; \
}

#define P31_SET_RECV_BITS( match, ignore, context, src, tag )	\
{								\
    ptl_match_bits_t bb;                                        \
    match  = 0;							\
    ignore = P31_PROTOCOL_MASK;					\
								\
    match  = context;						\
    match &= 0x00000000ffffffff;                                \
    match  = (match << 16);					\
								\
    if ( src == MPI_ANY_SOURCE ) {				\
	match  = (match << 32);					\
	ignore |= P31_SOURCE_MASK;				\
    } else {							\
        bb = src;                                         \
        bb &= 0x00000000ffffffff;                         \
	match  |= bb;						\
	match   = (match << 32);				\
    }								\
								\
    if ( tag == MPI_ANY_TAG ) {					\
	ignore |= P31_TAG_MASK;					\
    } else {							\
        bb = tag;                                               \
        bb &= 0x00000000ffffffff;                     \
	match |= bb;						\
    }								\
}

#define P31_IS_SHORT( match ) ( match & P31_SHORT_MSG )
#define P31_IS_LONG( match )  ( match & P31_LONG_MSG  )
#define P31_IS_SYNC( event )  ( event->hdr_data != 0 )

#define P31_GET_SOURCE( match, src )			\
{							\
    src = (int)((P31_SOURCE_MASK & match) >> 32);	\
}

#define P31_GET_TAG( match, tag )               \
{                                               \
    tag = (int)(match);                         \
}

#define P31_GET_CONTEXT( match, context )		\
{							\
    context = (int)((P31_CONTEXT_MASK & match) >> 48);	\
}

/* for sending */
#define P31_SET_SHORT_SEND_BITS( match, context, src, tag)	\
{								\
    ptl_match_bits_t bb;                                 \
    match  = context;						\
    match &= 0x00000000ffffffff;                                \
    match  = (match << 16);					\
     bb = src;                                        \
     bb &= 0x00000000ffffffff;                         \
    match |= bb;						\
    match  = (match << 32);					\
     bb  = tag;                                        \
     bb &= 0x00000000ffffffff;                         \
    match |= bb | P31_SHORT_MSG;				\
}

#define P31_SET_LONG_SEND_BITS( match, context, src, tag )	\
{								\
    ptl_match_bits_t bb;                           \
    match  = context;						\
    match &= 0x00000000ffffffff;                                \
    match  = (match << 16);					\
    bb = src;                                        \
    bb &= 0x00000000ffffffff;                        \
    match |= bb;						\
    match  = (match << 32);					\
    bb = tag;                                        \
    bb &= 0x00000000ffffffff;                        \
    match |= bb | P31_LONG_MSG;				\
}

#define P31_SET_READY_SEND_BITS( match, context, src, tag)	\
{								\
    ptl_match_bits_t bb;                          \
    match  = context;						\
    match &= 0x00000000ffffffff;                                \
    match  = (match << 16);					\
    bb = src;                                     \
    bb &= 0x00000000ffffffff;                     \
    match |= bb;						\
    match  = (match << 32);					\
    bb = tag;                                     \
    bb &= 0x00000000ffffffff;                     \
    match |= bb | P31_READY_MSG;				\
}

/******************************************************************************
 *****************************************************************************/
#define P31_CALL( func, name )			        \
if ( (_mpi_p31_errno = func) != PTL_OK ) {		\
    fprintf(stderr,"%d: (%s:%d) %s failed : %s\n",	\
	    MPID_MyWorldRank, __FILE__, __LINE__, #name,	\
	    ptl_err_str[_mpi_p31_errno]);		\
    MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL ); \
}

/******************************************************************************
 MPID include files
 *****************************************************************************/
#include "cookie.h"
#include "mpi.h"
#include "mpid.h"
#include "comm.h"
#include "datatype.h"
#include "req.h"
#include "dev.h"
#include "mpid_debug.h"
#include "mpi_error.h"
#include "mpimem.h"
#include "mpiimpl.h"
#include "reqalloc.h"
#include "sbcnst2.h"
#include "mpid_bind.h"
#include "mpi_error.h"
#include "mpicoll.h"
#include "mpiops.h"
#include "attach.h"
#include "cmnargs.h"
#include "mpid_time.h"
#include "packets.h"
#include "queue.h"

#if POINTER_64_BITS
#define PTRINT unsigned long
#else
#define PTRINT unsigned int
#endif

/******************************************************************************
 P31 device include files
 *****************************************************************************/
#include "p31init.h"
#include "p31sshort.h"
#include "p31short.h"
#include "p31long.h"
#include "p31ready.h"
#include "p31debug.h"
#include "p31recv.h"

/******************************************************************************
 Portals device global variables
 *****************************************************************************/
/* adi2init */
extern int                 MPID_n_pending;
extern MPID_DevSet        *MPID_devset;

#endif /* P31_DEV_H */
