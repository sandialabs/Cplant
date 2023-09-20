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
**  $Id: srvr_comm_data.c,v 1.17 2002/03/06 21:35:22 ktpedre Exp $
**
** Data portal - Holds buffers required for put or get operations
**  that originate in the local process.  The "put" request sends a
**  message to remote processes to come retreive the data.  The
**  "get" sends a message to a process requesting data be sent
**  back.
**
** A buffer attached to a data portal is identified by a slot (the
** handle), which is just the match list index.  The match list
** entry has a single memory descriptor, which describes the buffer
** to be read (put) or written (get).
**
** The slot number is encoded in the entry's match bits.
**
** Functions used by calling codes:
**
** srvr_test_read_buf()
** srvr_test_write_buf()
** srvr_delete_buf()
**
** Functions used by library:
**
** srvr_init_data_ptl()
** srvr_grow_data_ptl()
** srvr_add_data_buf()
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "srvr_lib.h"
#include "srvr_err.h"


typedef struct _dataPtl{

    volatile int nslots;
    int inuse;

    ptl_handle_me_t *me_handle; /* handles for match list entries */
    ptl_md_t        *mddef;      /* one per match list entry */
    ptl_handle_md_t *md_handle;  

    ptl_handle_eq_t  eq;    /* one long event queue for all data buffers */

    /*
    ** On GET requests, we count the bytes that have arrived in
    ** our write buffer.  On PUT requests, we count the accesses
    ** that have been made by remote processes to our read buffer.
    */
    int             *completionCount;  

} dataPtl;

static dataPtl dP;
static int processEventQueue();
static int srvr_grow_data_ptl(int to);
static int srvr_release_data_ptl();
static int processEventQueue();

/***********************************************************************
** Initialize data portal with 10 unused slots.  Can add more later if
** necessary.
***********************************************************************/

static void
null_mddef(ptl_md_t *md)
{
    md->start      = NULL;
    md->length     = 0;
    md->threshold  = 0;
    md->max_offset = 0;
    md->options    = 0;
    md->user_ptr   = NULL;
    md->eventq     = SRVR_INVAL_EQ_HANDLE;
}
void
srvr_null_dp()    /* we call this in server_library_init() */
{
    dP.nslots     = 0;
    dP.inuse      = 0;
    dP.me_handle  = NULL;
    dP.md_handle  = NULL;
    dP.mddef      = NULL;
    dP.eq         = SRVR_INVAL_EQ_HANDLE;
    dP.completionCount = NULL;
}
static int
srvr_grow_data_ptl(int to)
{
int i, rc, from;
ptl_match_bits_t matchbits, ignorebits;
ptl_handle_me_t *me;
ptl_handle_md_t *mdh;
ptl_process_id_t id;
ptl_md_t *md;
int *cc;

    from = dP.nslots;

    if (to <= from) return 0;

    if (to > MAX_MES){
        CPerrno = ERESOURCE;
	return -1;
    }

    me = (ptl_handle_me_t *)malloc(to * sizeof(ptl_handle_me_t));

    mdh = (ptl_handle_md_t *)malloc(to * sizeof(ptl_handle_md_t));

    md = (ptl_md_t *)malloc(to * sizeof(ptl_md_t));

    cc = (int *)malloc(to * sizeof(int));

    if (!me || !md || !mdh || !cc){

        CPerrno = ENOMEM;

	if (me) free(me);
	if (md) free(md);
	if (mdh) free(mdh);
	if (cc) free(cc);

	srvr_release_data_ptl();

	return SRVR_INVAL_PTL;
    }

    if (from > 0){

        memcpy((void *)me, (void *)dP.me_handle, sizeof(ptl_handle_me_t) * from);

        memcpy((void *)mdh, (void *)dP.md_handle, sizeof(ptl_handle_md_t) * from);

        memcpy((void *)md, (void *)dP.mddef, sizeof(ptl_md_t) * from);

        memcpy((void *)cc, (void *)dP.completionCount, sizeof(int) * from);

	free(dP.me_handle);
	free(dP.md_handle);
	free(dP.mddef);
	free(dP.completionCount);
    }
    else{
        rc = PtlEQAlloc(__SrvrLibNI, EVENTQUEUE_LENGTH, &(dP.eq));

	if (rc != PTL_OK){
	    P3ERROR;
	}
    }

    dP.me_handle = me;
    dP.md_handle = mdh;
    dP.mddef      = md;
    dP.completionCount= cc;

    ignorebits = 0;

    id.nid = PTL_ID_ANY;
    id.pid = PTL_ID_ANY;
    id.addr_kind = PTL_ADDR_NID;

    for (i=from; i<to; i++){

        matchbits = i;

	/*
	** match list entry
	*/

	if (i == 0){

            rc = PtlMEAttach(__SrvrLibNI,
		     DATAPORTALS,
		     id,
		     matchbits, ignorebits,
		     PTL_RETAIN,
		     dP.me_handle);

	} else{

  	    rc = PtlMEInsert(dP.me_handle[i-1],
			 id,
			 matchbits, ignorebits,
			 PTL_RETAIN,
			 PTL_INS_AFTER,
			 dP.me_handle + i);
        }

	if (rc != PTL_OK){
	    srvr_release_data_ptl();
	    P3ERROR;
	}
	/*
	** it's single memory descriptor
	*/
        null_mddef(dP.mddef + i);

	dP.md_handle[i] = SRVR_INVAL_MD_HANDLE;
	dP.completionCount[i] = 0;

    }

    dP.nslots = to;

    return 0;
}
#if 0
/*
** do we need this?  the first "add_data_buf" call will initialize
** the data portal
*/
static int dPinit=0;

static int
srvr_init_data_ptl()
{
int rc;

    if (dPinit){
        return -1;
    }
    srvr_null_dp();

    rc = srvr_grow_data_ptl(10);

    if (!rc){
        dPinit = 1;
    }

    return rc;
}
#endif

static int
srvr_release_data_ptl()
{
int i;

     if (dP.me_handle){
         for (i=dP.nslots-1; i>=0 ; i--){
	     PtlMEUnlink(dP.me_handle[i]);
         }
         free(dP.me_handle);
     }
     if (dP.md_handle){
         free(dP.md_handle);
     }
     if (dP.mddef){
         free(dP.mddef);
     }
     if (dP.completionCount){
         free(dP.completionCount);
     }
     if (dP.eq != SRVR_INVAL_EQ_HANDLE){
         PtlEQFree(dP.eq);
     }

     srvr_null_dp();

     return 0;
}
/*
** Return the slot number, or -1 on error
**
**  type:   READ_DATA_BUFFER (for libsrvr put requests)
**          WRITE_DATA_BUFFER (for libsrvr get requests)
*/
int
srvr_add_data_buf(void *buf, int len, int type)    
{
int rc, i, slot;
ptl_md_t *md;

    if (  (len <= 0) ||
          (buf == NULL) ||
	 ((type != READ_DATA_BUFFER) && (type != WRITE_DATA_BUFFER)) ) {

        CPerrno = EINVAL;
	return -1;
    }

    /*
    ** find a free slot, add new slots if needed
    */
    slot = -1;

    if ((dP.nslots - dP.inuse) == 0){

        rc = srvr_grow_data_ptl(dP.nslots + 10);

	if (rc){
	    return rc;
	}

	slot = dP.nslots - 1;
    }
    else{
    
	for (i = 0; i < dP.nslots; i++){

	    if (dP.md_handle[i] == SRVR_INVAL_MD_HANDLE){
		slot = i;
		break;
	    }
	}
    }

    if (slot < 0){
        CPerrno = EOHHELL;   /* shouldn't happen */
	return -1;
    }

    md = dP.mddef + slot;

    md->start = buf;
    md->length = len;
    md->threshold = PTL_MD_THRESH_INF;
    md->max_offset = len;

    if (type == WRITE_DATA_BUFFER){
	md->options = (PTL_MD_OP_PUT | PTL_MD_MANAGE_REMOTE);
    }
    else if (type == READ_DATA_BUFFER){
	md->options = (PTL_MD_OP_GET | PTL_MD_MANAGE_REMOTE);
    }

    md->user_ptr = (void *)((long)slot);
    md->eventq   = dP.eq;

    rc = PtlMDAttach(dP.me_handle[slot],   
                     dP.mddef[slot],
		     PTL_UNLINK,
		     dP.md_handle + slot);

    if (rc != PTL_OK){
        srvr_release_data_ptl();
	P3ERROR;
    }

    dP.completionCount[slot] = 0;

    dP.inuse++;

    return slot;
}
int
srvr_delete_buf(int slot)
{
int rc;

    if ( (slot < 0) || (slot >= dP.nslots)){
        CPerrno = EINVAL;
	return -1;
    }

    if (dP.md_handle[slot] == SRVR_INVAL_MD_HANDLE){
        CPerrno = EINVAL;
	return -1;
    }

    rc = PtlMDUnlink(dP.md_handle[slot]);

    if (rc != PTL_OK){
	srvr_release_data_ptl();
        P3ERROR;	
    }
    dP.md_handle[slot] = SRVR_INVAL_MD_HANDLE;
    dP.completionCount[slot] = 0;

    null_mddef(dP.mddef + slot);

    dP.inuse--;

    return 0;
}
/*
** Check if data has come into the write memory buffer
**
**    1  if buffer has arrived
**    0  if buffer had NOT arrived 
*/
int
srvr_test_write_buf(int slot)
{
int rc;

    if ( (slot < 0) || 
         (slot >= dP.nslots) ||

	 (dP.md_handle[slot] == SRVR_INVAL_MD_HANDLE) ||

	 (! (dP.mddef[slot].options & PTL_MD_OP_PUT) )     ){

	 CPerrno = EINVAL;
	 return -1;
    }

    /*
    ** Did we already determine on a previous call that the
    ** data has come in?
    */
    if (dP.completionCount[slot] == dP.mddef[slot].length){
        return 1;
    }

    /*
    ** Take a spin through the event queue to update
    ** data buffer completion counts.
    */

    rc = processEventQueue();

    if (rc){
        return -1;
    }

    if (dP.completionCount[slot] == dP.mddef[slot].length){
        return 1;
    }
    else{
        return 0;
    }
}
static int
processEventQueue()
{
ptl_event_t ev;
int rc, slot, hopcount;

    hopcount = EVENTQUEUE_LENGTH + 1;

    while (hopcount--){

        rc = PtlEQGet(dP.eq, &ev);

        if ((rc == PTL_OK) || (rc == PTL_EQ_DROPPED)){

	    if (rc == PTL_EQ_DROPPED){
	        log_msg("warning - events were dropped in processEventQueue");
	    }

	    slot = (int)((long)ev.md.user_ptr);

	    if (dP.mddef[slot].options & PTL_MD_OP_GET){
	        /*
		** A Portals 3 "GET" corresponds to a libsrvr
		** "put" operation.  (I know, it's confusing.
		** In a "put" operation, the initiator wishes
		** to put a buffer in remote process' memory,
		** so the server library sends a message to 
		** remote process(es) to come and get some data.)
		*/

	        dP.completionCount[slot]++;  /* save number of accesses */
	    }
	    else if (dP.mddef[slot].options & PTL_MD_OP_PUT){
	        /*
		** A Portals 3 "PUT" corresponds to a libsrvr "get"
		** operation.  (In a "get" operation, the initiator
		** wishes to get a buffer from a remote process,
		** so the server library sends a message to the
		** remote process requesting it to send us some data.)
		*/

	        dP.completionCount[slot] += ev.mlength; /* save bytes written */
	    }
	}
	else if (rc == PTL_EQ_EMPTY){
	    return 0;
	}
	else{
	    P3ERROR;
	}
    }

    if (hopcount == 0){
        CPerrno = EOHHELL;  /* shouldn't ever happen */
	return -1;
    }
    
    return 0;
}
/*
** test if "count" accesses have been made to the read data buffer
** by remote processes
**
**    1  at least "count" accesses have been made, and the data has been
**         sent out of the buffer and the buffer can safely be reused.
**    0  fewer than "count" accesses have been made, or "count" accesses
**         have been made but we can't tell if the data is yet done 
**         being sent out (because there is activity on the portal).
**   -1  some error
*/
int
srvr_test_read_buf(int slot, int count)
{
int rc;

    if ( (slot < 0) || 
         (slot >= dP.nslots) ||

	 (dP.md_handle[slot] == SRVR_INVAL_MD_HANDLE) ||

	 (! (dP.mddef[slot].options & PTL_MD_OP_GET) )     ){

	 CPerrno = EINVAL;
	 return -1;
    }

    /*
    ** Did we already determine on a previous call that the
    ** data has come in?
    */
    if (dP.completionCount[slot] >= count){
        return 1;
    }

    /*
    ** Take a spin through the event queue to update
    ** data buffer completion counts.
    */

    rc = processEventQueue();

    if (rc){
        return -1;
    }

    if (dP.completionCount[slot] >= count){
        return 1;
    }
    else{
        return 0;
    }
}
/*
** a debugging helper
*/
static void
display_md(ptl_md_t *md, FILE *fp)
{
    fprintf(fp,"start %p length %d threshold %d max_offset %d options %x user %d, eq %d\n",
        md->start,md->length,md->threshold,md->max_offset,md->options,
	(int)((long)md->user_ptr), md->eventq);
}

static void
display_data_portal(FILE *fp)
{
int inuse=0, i;

    fprintf(fp,"Data portal contents:\n");

    fprintf(fp,"%d slots, %d inuse\n",dP.nslots,dP.inuse);

    for (i=0; i<dP.nslots; i++){

        if (dP.md_handle[i] == SRVR_INVAL_MD_HANDLE){
            fprintf(fp, "%d) not in use\n",i);
	}
	else{
	    inuse++;

	    if (dP.mddef[i].options & PTL_MD_OP_GET){
	        fprintf(fp,"%d) PUT ",i); display_md(dP.mddef + i, fp);
	    }
	    else if (dP.mddef[i].options & PTL_MD_OP_PUT){
	        fprintf(fp,"%d) GET ",i); display_md(dP.mddef + i, fp);
	    }
	    else{
                fprintf(fp, "%d) Invalid MD options 0x%x\n",
		                i,  dP.mddef[i].options);
	    }
	    fprintf(fp,"\tmd handle %d, mle handle %d, completion count %d\n",
		  dP.md_handle[i],dP.me_handle[i],dP.completionCount[i]);
	}
    }
    fprintf(fp,"\nTotal inuse found %d\n",inuse);

}
