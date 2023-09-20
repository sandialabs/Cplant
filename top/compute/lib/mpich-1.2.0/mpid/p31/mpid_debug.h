#ifdef MPID_DEBUG_ALL  
/***************************************************************************/
/* This variable controls debugging output                                 */
/***************************************************************************/
#include <stdio.h>

#define MPI_NO_MPEDBG /* don't want MPE debugging stuff */

extern int MPID_DebugFlag;
extern FILE *MPID_DEBUG_FILE;

#ifndef ANSI_ARGS
#if defined(__STDC__) || defined(__cplusplus) || defined(HAVE_PROTOTYPES)
#define ANSI_ARGS(a) a
#else
#define ANSI_ARGS(a) ()
#endif
#endif

/* Use these instead of printf to simplify finding stray error messages */
#ifndef FPRINTF
#define FPRINTF fprintf
#define PRINTF printf
#define SPRINTF sprintf
#endif

/* This is pretty expensive.  It should be an option ... */
#ifdef DEBUG_INIT_MEM
#define DEBUG_INIT_STRUCT(s,size) memset(s,0xfa,size)		
#else
#define DEBUG_INIT_STRUCT(s,size)
#endif

#else
#define DEBUG_PRINT_PKT(msg,pkt)
#define DEBUG_PRINT_MSG(msg)
#define DEBUG_PRINT_ARGS(msg) 
#define DEBUG_PRINT_SEND_PKT(msg,pkt)
#define DEBUG_PRINT_BASIC_SEND_PKT(msg,pkt)
#define DEBUG_PRINT_FULL_SEND_PKT(msg,pkt)
#define DEBUG_PRINT_RECV_PKT(msg,pkt)
#define DEBUG_PRINT_FULL_RECV_PKT(msg,pkt)
#define DEBUG_PRINT_PKT_DATA(msg,pkt)
#define DEBUG_PRINT_LONG_MSG(msg,pkt)     
#define DEBUG_TEST_FCN(fcn,msg)
#define DEBUG_INIT_STRUCT(s,size)
#endif


