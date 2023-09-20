/* $Id: ptlsdev.h,v 1.3 2001/05/21 15:53:58 pumatst Exp $ */

#ifndef PTLS_DEV_H
#define PTLS_DEV_H

/******************************************************************************
 Standard include files
 *****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <signal.h>

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

/******************************************************************************
 Ptls OS and library include files
 *****************************************************************************/
#include "libtrap.h"
#include "pt2pt.h"
#include "puma.h"
#include "portal.h"

/******************************************************************************
 Portals device macros
 *****************************************************************************/
/* rename everything*/
#define MPID_CH_InitMsgPass                               MPID_PTLS_InitMsgPass


#define MPID_TAG_UB                                                  0x7fffffff
#define MPIDPATCHLEVEL                                                      1.0

#define PTLS_BARRIER_TAG                                          (-1000000506)

/* turn these into null macros */
#define MPID_PTLS_Check_incoming( dev, is_blocking )
#define MPID_DeviceCheck( is_blocking )

#define PTLS_PORTAL_NO_ACK                                               (-1)

#define PTLS_DEFAULT_MATCH_LIST_SIZE                                     2048
#define PTLS_DEFAULT_HEAP_SIZE                                     (1024*1024)
#define PTLS_DEFAULT_SHORT_MSG_SIZE                                      4096
#define PTLS_DEFAULT_GETPUT_ML_SIZE                                        20
#define PTLS_DEFAULT_COLL_WORK_SIZE                                 (1024*10)

#define MPI_ENV_DEBUG_STRING                                      "MPI_DEBUG"
#define MPI_ENV_MATCH_LIST_SIZE_STRING                  "MPI_MATCH_LIST_SIZE"
#define MPI_ENV_HEAP_SIZE_STRING                              "MPI_HEAP_SIZE"
#define MPI_ENV_SHORT_MSG_SIZE_STRING                    "MPI_SHORT_MSG_SIZE"
#define MPI_ENV_COLL_WORK_SIZE_STRING                    "MPI_COLL_WORK_SIZE"
#define MPI_ENV_GETPUT_ML_SIZE_STRING                    "MPI_GETPUT_ML_SIZE"
#define MPI_ENV_DEFAULT_COLL_OPS_STRING                "MPI_DEFAULT_COLL_OPS"

#define INIT_ML( portal, desc_list, head_desc, list_size )                 \
{									   \
    PORTAL_DESCRIPTOR *ptl_desc;                                           \
    MATCH_LIST_TYPE   *match_list;                                         \
									   \
    ptl_desc = &_my_pcb->portal_table2[portal];			           \
    ptl_desc->mem_op     = UNASSIGNED_MD;				   \
    ptl_desc->stat_bits  = 0x00;					   \
    ptl_desc->active_cnt = 0;						   \
    PTL_CLR_FLAG(portal,&_my_pcb->portals_dropped);			   \
									   \
    match_list = &ptl_desc->mem_desc.match;			           \
    match_list->list_len = list_size;					   \
    if ( (match_list->u_lst  = 					           \
          match_list->kn_lst =                                             \
	  desc_list          =                                             \
	  (MATCH_DESC_TYPE *)malloc( match_list->list_len *                \
				     sizeof(MATCH_DESC_TYPE)) ) ==	   \
	  (MATCH_DESC_TYPE *)NULL ) {					   \
      perror( "malloc() failed" );					   \
      exit( 1 );							   \
    }									   \
									   \
    for ( i=0; i<match_list->list_len; i++ ) {			           \
      desc_list[i].ctl_bits            = MCH_NOT_ACTIVE;                   \
	desc_list[i].next              =			           \
	  desc_list[i].next_on_nobuf   =			           \
	    desc_list[i].next_on_nofit = 0;			           \
      desc_list[i].rank                = ANY_RANK;                         \
      desc_list[i].gid                 = ANY_GROUP;                        \
    }                                                                      \
									   \
    head_desc = &desc_list[0];  					   \
									   \
    ptl_desc->mem_op               = MATCHING;				   \
    ptl_desc->stat_bits            = PTL_ACTIVE;			   \
                                                                           \
}

#define UNLINK_RECV_MLE( rhandle )                                	   \
{                                                                          \
  MATCH_DESC_TYPE *prev_match_desc;                                        \
  int              prev_match_index;                                       \
  int              match_index;                                            \
  /* Deactivate the mle */                                                 \
  rhandle->recv_match_desc->ctl_bits = MCH_NOT_ACTIVE;		   	   \
  									   \
  match_index = (int)(rhandle->recv_match_desc - recv_head_desc); 	   \
  prev_match_index = recv_previous_list[match_index];                      \
  prev_match_desc = &recv_desc_list[ prev_match_index ];                   \
  prev_match_desc->next          = 	                                   \
  prev_match_desc->next_on_nobuf =                                         \
    rhandle->recv_match_desc->next;                               	   \
  prev_match_desc->next_on_nofit = -1;                                     \
                                                                           \
  recv_previous_list[match_index] = 0;                                     \
                                                                           \
  if ( recv_last_reg_desc == rhandle->recv_match_desc ) {         	   \
    recv_last_reg_desc = prev_match_desc;                                  \
    recv_last_reg_index = prev_match_index;				   \
  }                                                                        \
  else {                                                                   \
    recv_previous_list[rhandle->recv_match_desc->next] =          	   \
      prev_match_index;                                                    \
  }                                                                        \
                                                                           \
  recv_first_free++;							   \
  recv_free_list[recv_first_free] = match_index;                           \
									   \
}

#define PTLS_SYNC_ACK        (ACK_MSG_DROPPED + 1 )

#define ANY_GROUP             (-1)
#ifndef ANY_RANK
#define ANY_RANK              (-1)
#endif

#define PTLS_READY_MSG_MASK   0x8000
#define PTLS_PULLED_MSG_MASK  0x4000
#define PTLS_LONG_MSG_MASK    0x2000
#define PTLS_SHORT_MSG_MASK   0x0000

/* PTLS matchbits on posted recv */

/*

01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
| |   context   | |   local src   | |            message              |
| |      id     | |      rank     | |              tag                |
 ^__
 message type
*/

#define PTLS_SEND_SET_MATCHBITS( match, contextid, lrank, typemask, tag )          \
   (match).shorts.s0    = (typemask);                                              \
   (match).shorts.s0   |= (UINT16)(contextid);                                     \
   (match).shorts.s1    = (UINT16)(lrank);                                         \
   (match).ints.i1      = (UINT32)(tag);

#define PTLS_RECV_SET_MATCHBITS( match, ignore, contextid, lrank, tag )            \
   (match).shorts.s0    = (UINT16)(contextid);                                     \
   (ignore).shorts.s0   = PTLS_LONG_MSG_MASK | PTLS_READY_MSG_MASK;                \
   if ( (lrank) == MPI_ANY_SOURCE ) {                                              \
     (match).shorts.s1  = 0x0000;                                                  \
     (ignore).shorts.s1 = 0xffff;                                                  \
   }                                                                               \
   else {                                                                          \
     (match).shorts.s1  = (UINT16)(lrank);                                         \
     (ignore).shorts.s1 = 0x0000;                                                  \
   }                                                                               \
   if ( (tag)   == MPI_ANY_TAG ) {                                                 \
     (match).ints.i1    = 0x00000000;                                              \
     (ignore).ints.i1   = 0xffffffff;                                              \
   }                                                                               \
   else {                                                                          \
     (match).ints.i1    = (UINT32)(tag);                                           \
     (ignore).ints.i1   = 0x00000000;                                              \
   }

#define PTLS_GET_CONTEXTID( contextid, chameleon )                                 \
   (contextid) = (chameleon).shorts.s0 &                                           \
                 ~PTLS_READY_MSG_MASK  &                                           \
                 ~PTLS_PULLED_MSG_MASK &                                           \
                 ~PTLS_LONG_MSG_MASK;

#define PTLS_GET_LRANK( lrank, chameleon )                                         \
   (lrank)     = (chameleon).shorts.s1;

#define PTLS_GET_TAG( tag, chameleon )                                             \
    (tag)      = (chameleon).ints.i1;

#define PTLS_PULL_SET_MATCHBITS( match, ignore )                                   \
   (ignore).shorts.s0   = 0x0000;                                                  \
   (match).shorts.s0   |= PTLS_PULLED_MSG_MASK;

#define PTLS_MSG_WAS_PULLED( match )                                               \
   ( match.shorts.s0 & PTLS_PULLED_MSG_MASK )

#define PTLS_MSG_WAS_NOT_PULLED( match )                                           \
   ( match.shorts.s0 ^ PTLS_PULLED_MSG_MASK )
 
#define PTLS_SHORT_OVERFLOW_MATCHBITS( match, ignore )                             \
   (match).shorts.s0    = 0x0000;			       	                   \
   (ignore).shorts.s0   = 0x1fff; /* first 3 bits must be zero */	           \
   (match).shorts.s1    = 0x0000;             		      		           \
   (ignore).shorts.s1   = 0xffff;                                      	           \
   (match).ints.i1      = 0x00000000;			      		           \
   (ignore).ints.i1     = 0xffffffff;			      		      
							        
#define PTLS_LONG_OVERFLOW_MATCHBITS( match, ignore )                              \
   (match).shorts.s0    = 0x0000;			        		   \
   (ignore).shorts.s0   = 0x3fff;  /* first 2 bits must be zero */		   \
   (match).shorts.s1    = 0x0000;              		        		   \
   (ignore).shorts.s1   = 0xffff;                                      		   \
   (match).ints.i1      = 0x00000000;			        		   \
   (ignore).ints.i1     = 0xffffffff;			        		      
							        
#define PTLS_RECV_IS_COMPLETED( shandle )     ( (shandle)->completer == 0 )
#define PTLS_RECV_IS_NOT_COMPLETED( shandle ) ( (shandle)->completer != 0 )

/* PUMA message types         */
#define PTLS_MSG_UNKNOWN     -1
#define PTLS_MSG_SHORT        0
#define PTLS_MSG_LONG         1
#define PTLS_MSG_SHORT_SYNC   2
#define PTLS_MSG_READY        3

#define PTLS_SEND_IS_COMPLETED( shandle )     ( (shandle)->completer == 0 )
#define PTLS_SEND_IS_NOT_COMPLETED( shandle ) ( (shandle)->completer != 0 )

#define PORTAL_SEND( buf, num_bytes, dest_offset, dest_group, dest_rank,       \
                     dest_portal, return_ptl, rply_length, rply_offset,        \
                     send_info )                                               \
{                                                                              \
									       \
   (send_info).dst_offset          = (UINT32)dest_offset;  		       \
   (send_info).reply_offset        = (UINT32)rply_offset;		       \
   (send_info).reply_len           = (UINT32)rply_length;	               \
   (send_info).return_portal       = (PORTAL_INDEX)return_ptl;		       \
   (send_info).send_flag           = 0;					       \
   									       \
   if ( send_user_msg2( (CHAR *)(buf),					       \
                        (num_bytes),					       \
                        (dest_rank),					       \
                        (dest_portal),  				       \
                        0,						       \
                        &(send_info) ) )  				       \
     fprintf(stderr,"send_user_msg2() failed\n");                              \
                                                                               \
}

#define PTLS_ONE_SIDED_COOKIE 0xea14beaf

#define PTLS_MALLOC_ERROR( bytes )                                             \
    fprintf(stderr,"ERROR: malloc() couldn't allocated %d bytes\n", bytes )

#define PTLS_MALLOC( addr )  malloc( addr )
#define PTLS_FREE( addr )    free( addr )

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
 Portals device include files
 *****************************************************************************/
#include "ptlsinit.h"
#include "ptlssshort.h"
#include "ptlsshort.h"
#include "ptlslong.h"
#include "ptlsready.h"
#include "ptlspriv.h"
#include "ptlscoll.h"
#include "ptlsdebug.h"
#include "ptlsrecv.h"

/******************************************************************************
 Portals device global variables
 *****************************************************************************/
/* adi2init */
extern int                 MPID_n_pending;
extern MPID_DevSet        *MPID_devset;

#endif /* PTLS_DEV_H */
