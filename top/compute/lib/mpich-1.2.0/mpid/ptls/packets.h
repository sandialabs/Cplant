
/* 
   This file defines the packet/message format that WILL be used in
   the next generation ADI.  
 */

#ifndef MPID_PKT_DEF
#define MPID_PKT_DEF

#define MPID_MIN(a,b) ((a) < (b) ? (a) : (b))

#define MPID_DO_HETERO(a) 
#define MPID_PKT_MSGREP_DECL 

#define MPID_PKT_FLOW_DECL

extern FILE *MPID_TRACE_FILE;

#ifdef MPID_DEBUG_ALL
#define MPID_TRACE_CODE(name,channel) {if (MPID_TRACE_FILE){\
fprintf( MPID_TRACE_FILE,"[%d] %20s on %4d at %s:%d\n", MPID_MyWorldRank, \
         name, channel, __FILE__, __LINE__ ); fflush( MPID_TRACE_FILE );}}
#define MPID_TRACE_CODE_PKT(name,channel,mode) {if (MPID_TRACE_FILE){\
fprintf( MPID_TRACE_FILE,"[%d] %20s on %4d (type %d) at %s:%d\n", \
	 MPID_MyWorldRank, name, channel, mode, __FILE__, __LINE__ ); \
	 fflush( MPID_TRACE_FILE );}}
#else
#define MPID_TRACE_CODE(name,channel)
#define MPID_TRACE_CODE_PKT(name,channel,mode)
#endif

/* chdef contains the definitions for a particular channel implementation */
#include "chdef.h"
/* channel contains the channel implementation */
#include "channel.h"

#endif
