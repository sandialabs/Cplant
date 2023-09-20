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
**  $Id: srvr_comm.c,v 1.8 2002/03/06 21:35:22 ktpedre Exp $
*/

#include <string.h>
#include <stdlib.h>
#include <p30.h>
#include <p30/coll.h>
#include "srvr_lib.h"
#include "srvr_coll.h"
#include "srvr_async.h"
#include "puma.h"

char              __SrvrLibInit = 0;
ptl_handle_ni_t   __SrvrLibNI;
int               SrvrDbg=0;
int               __SrvrDebug = 0;

#define N_DEBUG_TYPES 10

static char *p30_debug_types[N_DEBUG_TYPES]={
"PTL_DEBUG_PUT",
"PTL_DEBUG_GET",
"PTL_DEBUG_REPLY",
"PTL_DEBUG_ACK",
"PTL_DEBUG_DROP",
"PTL_DEBUG_REQUEST",
"PTL_DEBUG_DELIVERY",
"PTL_DEBUG_UNLINK",
"PTL_DEBUG_THRESHOLD",
"PTL_DEBUG_API"
};

int
server_library_init()
{
#ifdef ACL_ENTRIES_IMPLEMENTED
ptl_process_id_t src;
#endif
int i, rc;
unsigned int debugFlags;

    if (__SrvrLibInit){
        CPerrno = EINVAL;
	return -1;
    }

    if (!__p30_initialized){
        if( (rc=PtlInit()) ){
	    CPerrno = EPORTAL;
	    if (___proc_type == SERV_TYPE){
                fprintf(stderr, "server_library_init: calling PtlInit()\n");
	    }
	    return -1;
        }
    }
    if (!__p30_myr_initialized){

        if( (rc = PtlNIInit( PTL_IFACE_MYR, 
                         MYRNAL_MAX_PTL_SIZE,
			 MYRNAL_MAX_ACL_SIZE,
			 &__SrvrLibNI ))) {

  	    CPerrno = EPORTAL;
            srvr_p30errno = rc;
	    if (___proc_type == SERV_TYPE){
                fprintf(stderr, "server_library_init: calling PtlNIInit()\n");
	    }
	    return -1;
        }
    }
    else{
        memcpy((void *)&__SrvrLibNI,
               (void *)&__myr_ni_handle,
               sizeof(ptl_handle_ni_t));
    }
 
#ifdef ACL_ENTRIES_IMPLEMENTED

    src.nid = src.pid = src.gid = src.rid = PTL_ID_ANY;
    src.addr_kind = PTL_ADDR_BOTH;

    rc = PtlACEntry(__SrvrLibNI, SRVR_ACL_ANY, src, CONTROLPORTALS);

    if (rc != PTL_OK){
	CPerrno = EPORTAL;
        srvr_p30errno = rc;
        fprintf(stderr, "server_library_init: calling PtlACEntry(0)\n");
	return -1;
    }
    rc = PtlACEntry(__SrvrLibNI, SRVR_ACL_ANY, src, DATAPORTALS);

    if (rc != PTL_OK){
	CPerrno = EPORTAL;
        srvr_p30errno = rc;
        fprintf(stderr, "server_library_init: calling PtlACEntry(1)\n");
	return -1;
    }
#endif

    srvr_null_dp();    /* initialize data portal structure */

    debugFlags = 0;

    for (i=0; i<N_DEBUG_TYPES; i++){
        if (getenv(p30_debug_types[i])){
	    debugFlags |= (1 << i);
	}
    }
    if (debugFlags){
        PtlNIDebug(__SrvrLibNI, debugFlags);
    }

    init_pending_buf_list();

    __SrvrLibInit = 1;

    return 0;
}
int
srvr_p30_barrier(time_t tmout)
{
int rc;
time_t torig;

    /*
    ** This barrier is for application group formed
    ** by SET_NID, SET_PID ioctl calls.
    */
    torig = __p30_myr_timeout;
    __p30_myr_timeout = tmout;

    rc = PtlNIBarrier(__SrvrLibNI);

    __p30_myr_timeout = torig;

    if (rc != PTL_OK){
        CPerrno = EPORTAL;
	srvr_p30errno = rc;
	return -1;
    }
    return 0;
}
int
srvr_p30_bcast(int *val, time_t tmout)
{
int rc;
time_t torig;

    /*
    ** Can broadcast an int
    */
    torig = __p30_myr_timeout;
    __p30_myr_timeout = tmout;

    rc = PtlBroadcast_all(__SrvrLibNI, 0, val);

    __p30_myr_timeout = torig;

    if (rc != PTL_OK){
        CPerrno = EPORTAL;
	srvr_p30errno = rc;
	return -1;
    }
    return 0;
}
void
server_library_done()
{
    server_coll_done();
    release_all_control_portals();

    PtlNIFini(__SrvrLibNI);

    PtlFini();
}

/*
** We may timeout on a PtlPut or a PtlGet.  This could be
** because the target node is down, because we sent an
** rts and the cts got lost on the way back, or because
** we sent a message and the ack got lost on the way back,
** you get the picture.
**
** So we are happy to give up and carry on.
**
** However, the layer below will continue to try to
** send the message or wait for the "get" data forever.  
** To cancel the request, we must unlink the memory 
** descriptor associated with it.  We are guaranteed that 
** once we return from the PtlMDUnlink call, the messaging 
** layer will not write our get buffer or our event queue 
** on behalf of this operation.  However, between the time we 
** gave up and the time we return from PtlMDUnlink, an event
** may have been posted to the queue.  So lets clear out
** the queue after unlinking the memory descriptor.
*/
int
srvrHandleSendTimeout(ptl_md_t mddef, ptl_handle_md_t md)
{
int ecount, rc, i;
ptl_event_t ev;

    rc = PtlMDUnlink(md);

    if (rc != PTL_OK){
        P3ERROR;
    }

    if (mddef.eventq != SRVR_INVAL_EQ_HANDLE){
	rc = PtlEQCount(mddef.eventq, &ecount);

	if (rc != PTL_OK){
	    P3ERROR;
	}

	if (ecount > 0){
	    for (i=0; i<ecount; i++){
		PtlEQGet(mddef.eventq, &ev);
	    }
	}
    }

    return 0;
}

