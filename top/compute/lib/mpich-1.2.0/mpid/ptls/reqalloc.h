#ifndef MPIREQALLOC
#define MPIREQALLOC

/* Allocation of handles */
#include "sbcnst2.h"

extern MPID_SBHeader MPIR_shandles;
extern MPID_SBHeader MPIR_rhandles;
#define MPID_RecvInit( a ) {(a)->self_index = 0;(a)->ref_count=1;}
#define MPID_PRecvInit( a ) {(a)->self_index = 0;(a)->ref_count=1;}
#define MPID_SendInit( a ) {(a)->self_index = 0;(a)->ref_count=1;}
#define MPID_PSendInit( a ) {(a)->self_index = 0;(a)->ref_count=1;}
#ifdef MPIR_MEMDEBUG 
#define MPID_RecvAlloc( a ) {a=(MPIR_RHANDLE *)MALLOC(sizeof(MPIR_RHANDLE));((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_SendAlloc( a ) {a=(MPIR_SHANDLE *)MALLOC(sizeof(MPIR_SHANDLE));((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_PRecvAlloc( a ) {a=(MPIR_PRHANDLE *)MALLOC(sizeof(MPIR_PRHANDLE));((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_PSendAlloc( a ) {a=(MPIR_PSHANDLE *)MALLOC(sizeof(MPIR_PSHANDLE));((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_RecvFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);FREE( a );}
#define MPID_SendFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);FREE( a );}
#define MPID_PRecvFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);FREE( a );}
#define MPID_PSendFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);FREE( a );}
#else
#define MPID_RecvAlloc( a ) {a = (MPIR_RHANDLE *)MPID_SBalloc(MPIR_rhandles);((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_SendAlloc( a ) {a = (MPIR_SHANDLE *)MPID_SBalloc(MPIR_shandles);((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_PRecvAlloc( a ) {a = (MPIR_PRHANDLE *)MPID_SBalloc(MPIR_rhandles);((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_PSendAlloc( a ) {a = (MPIR_PSHANDLE *)MPID_SBalloc(MPIR_shandles);((MPIR_COMMON*)(a))->self_index=0;((MPIR_COMMON*)(a))->ref_count=1;}
#define MPID_RecvFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);MPID_SBfree( MPIR_rhandles, a );}
#define MPID_SendFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);MPID_SBfree( MPIR_shandles, a );}
#define MPID_PRecvFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);MPID_SBfree( MPIR_rhandles, a );}
#define MPID_PSendFree( a ) {if (((MPIR_COMMON*)(a))->self_index) MPIR_RmPointer(((MPIR_COMMON*)(a))->self_index);MPID_SBfree( MPIR_shandles, a );}
#endif

#endif
