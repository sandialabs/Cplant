#ifndef P30_DEV_H
#define P30_DEV_H

/******************************************************************************
 Standard include files
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <signal.h>

/******************************************************************************
 P30 include file
 *****************************************************************************/
#include "p30.h"
#include "myrnal.h"

#include "p30priv.h"
#include "p30_default_long_msg.alpha.h"

/******************************************************************************
 Portals device macros
 *****************************************************************************/
#define MPID_CH_InitMsgPass                               MPID_P30_InitMsgPass

#define MPID_TAG_UB                                                  0x7fffffff
#define MPIDPATCHLEVEL                                                      1.0

#define P30_BARRIER_TAG                                          (-1000000506)

/* turn these into null macros */
#define MPID_P30_Check_incoming( dev, is_blocking )
#define MPID_DeviceCheck( is_blocking )

#define P30_DEFAULT_IRECV_MAX                                           1024
#define P30_DEFAULT_LONG_MAX                                            1024
#define P30_DEFAULT_UNEX_MAX                                            1024

#define MPI_ENV_DEBUG_STRING                                      "MPI_DEBUG"
#define MPI_ENV_DUMP_QUEUES_STRING                          "MPI_DUMP_QUEUES"
#define MPI_ENV_IRECV_MAX_STRING                              "MPI_IRECV_MAX"
#define MPI_ENV_LONG_MAX_STRING                                "MPI_LONG_MAX"
#define MPI_ENV_UNEX_MAX_STRING                                "MPI_UNEX_MAX"
#define MPI_ENV_LONG_MSG_STRING                                "MPI_LONG_MSG"
#define MPI_ENV_INFO_STRING                                        "MPI_INFO"
#define MPI_ENV_DUMP_UNEX_Q_STRING                          "MPI_DUMP_UNEX_Q"

/*
** Matchbits on posted recv
**
** 01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
**    |  context    |      source     |            message
**    |     id      |                 |              tag
**  ^--- protocol
*/

#define P30_PROTOCOL_MASK 0xf000000000000000
#define P30_CONTEXT_MASK  0x0fff000000000000
#define P30_SOURCE_MASK   0x0000ffff00000000
#define P30_TAG_MASK      0x00000000ffffffff

#define P30_SHORT_MSG     0x1000000000000000
#define P30_LONG_MSG      0x2000000000000000
#define P30_READY_MSG     0x4000000000000000

/* for receiving */
#define P30_SET_SHORT_UNEX_BITS( match, ignore )                        \
{                                                                       \
    match  = P30_SHORT_MSG;                                             \
    ignore = P30_CONTEXT_MASK | P30_SOURCE_MASK | P30_TAG_MASK;         \
}

#define P30_SET_LONG_UNEX_BITS( match, ignore )                 \
{                                                               \
    match  = P30_LONG_MSG;                                      \
    ignore = P30_CONTEXT_MASK | P30_SOURCE_MASK | P30_TAG_MASK; \
}

#define P30_SET_RECV_BITS( match, ignore, context, src, tag )	\
{								\
    match  = 0;							\
    ignore = P30_PROTOCOL_MASK;					\
								\
    match  = context;						\
    match  = (match << 16);					\
								\
    if ( src == MPI_ANY_SOURCE ) {				\
	match  = (match << 32);					\
	ignore |= P30_SOURCE_MASK;				\
    } else {							\
	match  |= src;						\
	match   = (match << 32);				\
    }								\
								\
    if ( tag == MPI_ANY_TAG ) {					\
	ignore |= P30_TAG_MASK;					\
    } else {							\
	match  |= tag;						\
    }								\
}

#define P30_IS_SHORT( match ) ( match & P30_SHORT_MSG )
#define P30_IS_LONG( match )  ( match & P30_LONG_MSG  )
#define P30_IS_SYNC( event )  ( event->hdr_data != 0 )

#define P30_GET_SOURCE( match, src )			\
{							\
    src = (int)((P30_SOURCE_MASK & match) >> 32);	\
}

#define P30_GET_TAG( match, tag )               \
{                                               \
    tag = (int)(match);                         \
}

#define P30_GET_CONTEXT( match, context )		\
{							\
    context = (int)((P30_CONTEXT_MASK & match) >> 48);	\
}

/* for sending */
#define P30_SET_SHORT_SEND_BITS( match, context, src, tag)	\
{								\
    match  = context;						\
    match  = (match << 16);					\
    match |= src;						\
    match  = (match << 32);					\
    match |= tag | P30_SHORT_MSG;				\
}

#define P30_SET_LONG_SEND_BITS( match, context, src, tag )	\
{								\
    match  = context;						\
    match  = (match << 16);					\
    match |= src;						\
    match  = (match << 32);					\
    match |= tag | P30_LONG_MSG;				\
}

#define P30_SET_READY_SEND_BITS( match, context, src, tag)	\
{								\
    match  = context;						\
    match  = (match << 16);					\
    match |= src;						\
    match  = (match << 32);					\
    match |= tag | P30_READY_MSG;				\
}

/******************************************************************************
 *****************************************************************************/
#define P30_CALL( func, name )			        \
if ( (_mpi_p30_errno = func) != PTL_OK ) {		\
    fprintf(stderr,"%d: (%s:%d) %s failed : %s\n",	\
	    MPID_MyWorldRank, __FILE__, __LINE__, #name,	\
	    ptl_err_str[_mpi_p30_errno]);		\
    MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL ); \
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
 P30 device include files
 *****************************************************************************/
#include "p30init.h"
#include "p30sshort.h"
#include "p30short.h"
#include "p30long.h"
#include "p30ready.h"
#include "p30debug.h"
#include "p30recv.h"

/******************************************************************************
 Portals device global variables
 *****************************************************************************/
/* adi2init */
extern int                 MPID_n_pending;
extern MPID_DevSet        *MPID_devset;

#endif /* P30_DEV_H */
