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
** $Id: srvr_comm_ctl.c,v 1.33.2.1 2002/05/07 19:08:52 ktpedre Exp $ 
**
** Routines to manage control portals.  
**
** A control "portal" is actually a match list entry in the
** master control portal.  The "portal" number is in the
** match list entry's match bits.
**
** The caller specifies the maximum number of messages the
** control portal holds at any given time (maxMsgs).  Messages
** can be freed to make way for more.
**
** The control "portal" is implemented as two buffers, each
** long enough to hold maxMsgs control messages.  A memory
** descriptor points to each, and a sentinel memory descriptor
** follows the last one.  The sentinel has no storage, is never
** unlinked, and is a place marker for the end of the list of 
** memory descriptors.
**
** The library pulls messages out of the buffers and stores them 
** in a linked list for retrieval by the caller.  When the first buffer
** is used up, it is moved to follow the second buffer, and the
** second buffer becomes the first buffer.  This way there is
** always storage for at least maxMsgs incoming control messages.
**
** Control messages are used for 
**
**   o  sending brief messages with up to SRVR_USR_DATA_LEN bytes of
**      of user data
**
**   o  sending a PUT or GET request to a target
**
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include <srvr_lib.h>
#include <srvr_async.h>

/****************************
** The control portals flags
*****************************/

static char *msgFlags[5]={
"no message",
"unprocessed",
"processed",
"unread",
"read"
};

/***********************************************************************
** private functions - control portal manipulation
***********************************************************************/

static ptl_handle_eq_t send_event_queue = SRVR_INVAL_EQ_HANDLE;

/*
** Control messages are pulled out of buffer and saved
** in this linked list.  
*/

struct _inBuf{
   data_xfer_msg   *buf; 
   ptl_handle_md_t md;    
   ptl_md_t        mddef;
   data_xfer_msg   *nextread;
};

typedef struct _inBuf inBuf;
typedef struct _recvList recvList;

/*
** The order of the match list entries in the master control
** portal matches the order of the entries in this ctlPtl list.
*/

typedef struct _ctlPtl{

   int maxMsgs;
   int ptl;               /* unique identifier for ME mbits */

   inBuf buf1;
   inBuf buf2;

   ptl_handle_md_t mdSentinel;
    
   ptl_handle_me_t me;    /* match list entry handle */

   recvList *firstMsg;
   recvList *lastMsg;
   recvList *firstUnread;

   struct _ctlPtl *next;

} ctlPtl;

#define MAX_NUM_PTLS (SRVR_MAXCTLPTL-SRVR_MINCTLPTL+1)

static ctlPtl *srvrFirstCtlPtl = NULL;
static char ptlNums[MAX_NUM_PTLS];

static void initBufData(inBuf *buf);
static int deleteControlPortal(ctlPtl *cp);
static int makeControlPortal(int maxMsgs, int ptl);
static ctlPtl *findCtlPtl(int ptl);
static int flipBuffers(ctlPtl *cp);
static int processBuffer(ctlPtl *cp, data_xfer_msg *msg, int nmsgs);
static int processCtlPtlEvents(ctlPtl *cp);
static int updateFirstUnread(ctlPtl *cp);
static int recvCtlMsg(data_xfer_msg *msg, ctlPtl *cp);
static int freeCtlMsg(recvList *msg, ctlPtl *cp);

static void
initBufData(inBuf *buf)
{
    buf->buf = buf->nextread = NULL;
    buf->md = SRVR_INVAL_MD_HANDLE;
}

static int
deleteControlPortal(ctlPtl *cp)
{
ctlPtl *prev;
recvList *msg;
int found, hopcount, status;

  status = 0;

  if (!cp) return status;

  found = 0;

  if (cp == srvrFirstCtlPtl){        /* find and unlink from our list */
      srvrFirstCtlPtl = cp->next;
      found = 1;
  }
  else{
      hopcount = MAX_NUM_PTLS+1;

      prev = srvrFirstCtlPtl;

      while (prev && --hopcount){

	 if (prev->next == cp){
            prev->next = cp->next;
            found = 1;
	    break;
	 }
	 prev = prev->next;
      }
      if (!hopcount){
          log_msg("deleteControlPortal: ctlPtl list");
	  CPerrno = EOHHELL;
          status = -1;
      }
  }
  if (!found) return status;

  if (cp->me != SRVR_INVAL_ME_HANDLE){ /* unlink match list entry, this will */
      PtlMEUnlink(cp->me);   /* also free all attached memory descriptors */
  }

  if (cp->buf1.buf) free(cp->buf1.buf);    /* free message buffers */
  
  if (cp->buf2.buf) free(cp->buf2.buf);

  if (cp->firstMsg){

      msg = cp->firstMsg->next;

      hopcount = cp->maxMsgs * 2;

      while (msg && --hopcount){
         free(msg->prev);
	 msg = msg->next;
      }

      if (!hopcount){
          log_msg("deleteControlPortal: portal %d received message list",cp->ptl);
	  CPerrno = EOHHELL;
          status = -1;
      }
      else{
          free(cp->lastMsg);
      }
  }

  if ((cp->ptl >= 0) && (cp->ptl <= SRVR_MAXCTLPTL)){
     ptlNums[cp->ptl-SRVR_MINCTLPTL] = 0;
  }

  free(cp);

  return status;
}

static ctlPtl 
*findCtlPtl(int ptl)
{
ctlPtl *cp;
int hopcount;

    cp = NULL;

    if ( (ptl < 0) || (ptl > SRVR_MAXCTLPTL) || (ptlNums[ptl-SRVR_MINCTLPTL]==0) ){
	return cp;
    }

    if (!srvrFirstCtlPtl){
	return cp;
    }

    cp = srvrFirstCtlPtl;

    hopcount = MAX_NUM_PTLS;

    while ((cp->ptl != ptl) && --hopcount){

        if (cp->next == NULL){
	    cp = NULL;
	    break;
        }
        else{
	    cp = cp->next;
        }
    }

    if (!hopcount){
        log_msg("findCtlPtl for portal %d\n",ptl);
	CPerrno = EOHHELL;
	cp = NULL;

    }
    return cp;
}
static int
flipBuffers(ctlPtl *cp)
{
int rc;
ptl_handle_md_t newmd;
inBuf temp;

   /*
   ** zero out buffer for re-use
   */
   memset((char *)(cp->buf1.buf), 0, sizeof(data_xfer_msg) * cp->maxMsgs);

   cp->buf1.nextread = cp->buf1.buf;

   /*
   ** Put it after buffer 2, right before the sentinel.  We need the
   ** sentinel, which is never unlinked, because it's possible 
   ** (but unlikely) buffer 2 was used up and unlinked already.
   */
   rc = PtlMDInsert(cp->mdSentinel,
                    cp->buf1.mddef,
                    PTL_UNLINK,
                    PTL_INS_BEFORE,
                    &newmd);

    if (rc != PTL_OK){
       P3ERROR;
    }

    /*
    ** buffer 2 becomes buffer 1,
    ** re-initialized buffer 1 becomes buffer 2
    */

    memcpy((char *)&temp, (char *)&(cp->buf1), sizeof(inBuf));

    memcpy((char *)&(cp->buf1), (char *)&(cp->buf2), sizeof(inBuf));

    memcpy((char *)&(cp->buf2), (char *)&temp, sizeof(inBuf));

    cp->buf2.md = newmd;
    
    return 0;
}
static int
processBuffer(ctlPtl *cp, data_xfer_msg *msg, int nmsgs)
{
int eventcount, i;

    eventcount = 0;

    for (i=0; i<nmsgs; i++){

        /*
	** Order of "if" tests is critical - message can arrive between
	** the first and second.  Check for NOMSG, then check for 
	** UNPROCESSED message.
	*/

        if (msg->flag == CTLMSG_NOMSG){

            break;    /* no more new messages */

        }
        else if (msg->flag == CTLMSG_UNPROCESSED){

            recvCtlMsg(msg, cp);
  
            msg->flag = CTLMSG_PROCESSED;

            msg++;

            eventcount++;
        }
        else{
#if 0
	    log_msg("processBuffer: message flag on portal %d is %d (%s)?\n",
	                cp->ptl, msg->flag, msgFlags[msg->flag]);
#else
            log_msg("processBuffer: message flag on portal %d is %d (currupt buf?)\n",
	                cp->ptl, msg->flag);
#endif

	    CPerrno = EOHHELL;  /* corrupt buffer */
	    return -1;
        }
    }
    return eventcount;
}
static int
processCtlPtlEvents(ctlPtl *cp)
{
int nmsgs, rc;
int eventCount, eventCount2;
data_xfer_msg *msg, *last;

    msg = cp->buf1.nextread;

    last = cp->buf1.buf + cp->maxMsgs - 1; 

    if ((msg < cp->buf1.buf) || (msg > last)){

        log_msg("processCtlPtlEvents: portal %d, bad nextread (%d %d %d)\n",
	         cp->ptl,
		 0, cp->maxMsgs-1, (msg - (data_xfer_msg *)(cp->buf1.buf)));

	CPerrno = EOHHELL;  /* corrupt buffer */
	return -1;
    }

    nmsgs = last - msg + 1;

    eventCount = processBuffer(cp, msg, nmsgs);

    cp->buf1.nextread += eventCount;

    if (eventCount < nmsgs){
        return eventCount;
    }

    /*
    ** Buffer 1 is full.  Process buffer 2 then re-initialize
    ** buffer 1 and move it to the back, buffer 2 becomes buffer 1.
    */

    eventCount2 = processBuffer(cp, cp->buf2.buf, cp->maxMsgs);

    cp->buf2.nextread = cp->buf2.buf + eventCount2;

    if (eventCount2 == cp->maxMsgs){
        /*
        ** Shouldn't happen, this means user used more control
        ** message buffers than they said they would.  We'll let
        ** them get away with it, but they may have lost some
        ** messages.  Sloppy.  We'll flip twice.
        */
        rc = flipBuffers(cp);
    }

    rc = flipBuffers(cp);

    if (rc < 0){
        return rc;
    }

    return eventCount + eventCount2;
}
static int
makeControlPortal(int maxMsgs, int ptl)
{
int rc, buflen;
ptl_md_t sentinelMdDef;
ctlPtl *cp;
ptl_process_id_t match_any;
ptl_match_bits_t mbits,ibits;

   if (!__SrvrLibInit                      ||       
      (maxMsgs < 1)                        || 
      (ptl < 0)     || (ptl > SRVR_MAXCTLPTL) ||
      ptlNums[ptl-SRVR_MINCTLPTL] ){

       CPerrno = EINVAL;
       return -1;
   }

   /*
   ** Our record of a control portal.  (It's actually a ME in a
   ** master control portal.)
   */
   cp = (ctlPtl *)malloc(sizeof(ctlPtl));

   if (!cp){
     CPerrno = ENOMEM;
     return -1;
   }

   initBufData(&(cp->buf1));
   initBufData(&(cp->buf2));

   cp->maxMsgs        = maxMsgs;
   cp->ptl            = SRVR_INVAL_PTL;
   cp->mdSentinel     = SRVR_INVAL_MD_HANDLE;
   cp->me             = SRVR_INVAL_ME_HANDLE;
   cp->firstMsg       = NULL;
   cp->lastMsg        = NULL;
   cp->firstUnread    = NULL;
   cp->next           = NULL;

   /*
   ** allocate buffers for incoming messages
   */
   buflen = sizeof(data_xfer_msg) * maxMsgs;

   cp->buf1.buf = (data_xfer_msg *)calloc(sizeof(data_xfer_msg) , maxMsgs);
   cp->buf2.buf = (data_xfer_msg *)calloc(sizeof(data_xfer_msg) , maxMsgs);

   if ( !cp->buf1.buf || !cp->buf2.buf){
       deleteControlPortal(cp);
       CPerrno = ENOMEM;
       return -1;
   }
   cp->buf1.nextread = cp->buf1.buf;
   cp->buf2.nextread = cp->buf2.buf;

   cp->buf1.mddef.start     = cp->buf1.buf;
   cp->buf1.mddef.length    = buflen;
   cp->buf1.mddef.threshold = maxMsgs;
   cp->buf1.mddef.max_offset = cp->buf1.mddef.length;
   cp->buf1.mddef.options   = PTL_MD_OP_PUT;
   cp->buf1.mddef.user_ptr  = NULL;
   cp->buf1.mddef.eventq    = SRVR_INVAL_EQ_HANDLE;

   cp->buf2.mddef.start     = cp->buf2.buf;
   cp->buf2.mddef.length    = buflen;
   cp->buf2.mddef.threshold = maxMsgs;
   cp->buf2.mddef.max_offset = cp->buf2.mddef.length;
   cp->buf2.mddef.options   = PTL_MD_OP_PUT;
   cp->buf2.mddef.user_ptr  = NULL;
   cp->buf2.mddef.eventq    = SRVR_INVAL_EQ_HANDLE;

   sentinelMdDef.start     = NULL;
   sentinelMdDef.length    = 0;
   sentinelMdDef.threshold = PTL_MD_THRESH_INF;
   sentinelMdDef.max_offset = sentinelMdDef.length;
   sentinelMdDef.options   = PTL_MD_OP_PUT;
   sentinelMdDef.user_ptr  = NULL;
   /*
   ** If you want to detect dropped messages, allocate an
   ** event queue for the sentinel.
   */
   sentinelMdDef.eventq    = SRVR_INVAL_EQ_HANDLE;

   /*
   ** Accept incoming messages from any source
   */
   match_any.addr_kind = PTL_ADDR_BOTH;
   match_any.rid = PTL_ID_ANY;
   match_any.gid = PTL_ID_ANY;
   match_any.nid = PTL_ID_ANY;
   match_any.pid = PTL_ID_ANY;

   mbits = (ptl_match_bits_t)ptl;
   ibits = 0xffffffffffff0000;

   /*
   ** Create a new match list entry at the front of the list.
   */
   if (srvrFirstCtlPtl){

       rc = PtlMEInsert( srvrFirstCtlPtl->me,
		 match_any, mbits, ibits,
		 PTL_RETAIN, PTL_INS_BEFORE, 
		 &(cp->me));

   }
   else{
       rc = PtlMEAttach(__SrvrLibNI,    /* network interface handle */
		    CONTROLPORTALS,   /* portal number */
		    match_any,      /* process ID match criteria */
		    mbits, ibits,   /* match bits */
		    PTL_RETAIN,    /* don't unlink me when all mds are unlinked */
		    &(cp->me));     /* handle for match list entry */

   }
   if (rc != PTL_OK){
       deleteControlPortal(cp);
       P3ERROR;
  }


  /*
  ** Attach the sentinel first
  */
  
  rc = PtlMDAttach(cp->me,
		   sentinelMdDef,
		   PTL_RETAIN,
		   &cp->mdSentinel);

  if (rc != PTL_OK){
       deleteControlPortal(cp);
       P3ERROR;
  }
  /*
  ** Now the two live message buffers
  */

  rc = PtlMDInsert(cp->mdSentinel,
                   cp->buf1.mddef,
		   PTL_UNLINK,
		   PTL_INS_BEFORE,
		   &(cp->buf1.md));

  if (rc != PTL_OK){
      deleteControlPortal(cp);
      P3ERROR;
  }

  rc = PtlMDInsert(cp->mdSentinel,
                   cp->buf2.mddef,
		   PTL_UNLINK,
		   PTL_INS_BEFORE,
		   &(cp->buf2.md));

  if (rc != PTL_OK){
      deleteControlPortal(cp);
      P3ERROR;
  }


  ptlNums[ptl-SRVR_MINCTLPTL] = 1;
  cp->ptl      = ptl;

  /*
  ** Link control portal entry into the front of the list
  */
  cp->next = srvrFirstCtlPtl;
  srvrFirstCtlPtl = cp;

  return 0;
}
static int
updateFirstUnread(ctlPtl *cp)
{
recvList *next;
int hopcount;

    hopcount = cp->maxMsgs * 2;

    next = cp->firstUnread;

    cp->firstUnread = NULL;

    while (next && --hopcount){

        if (next->msg.flag == CTLMSG_UNREAD){
	    cp->firstUnread = next;
	    break;
	}

	next = next->next;
    }

    if (!hopcount){
        log_msg("updateFirstUnread: on portal %d\n",cp->ptl);
        CPerrno = EOHHELL;
	return -1;
    }
    return 0;
}
static int
recvCtlMsg(data_xfer_msg *msg, ctlPtl *cp)
{
recvList *newguy;

    newguy = (recvList *)malloc(sizeof(struct _recvList));

    if (!newguy){
        CPerrno = ENOMEM;
	return -1;
    }

    if (cp->lastMsg){

	cp->lastMsg->next = newguy;
	
	newguy->next = NULL;
	newguy->prev = cp->lastMsg;

	cp->lastMsg = newguy;

	if (cp->firstUnread == NULL){
	    cp->firstUnread = newguy;
	}
    }
    else{
        cp->firstMsg = cp->lastMsg = cp->firstUnread = newguy;

	newguy->prev = newguy->next = NULL;
    }

    memcpy((char *)&(newguy->msg), (char *)msg,  sizeof(data_xfer_msg));

    newguy->msg.flag = CTLMSG_UNREAD;

    return 0;
}
static int
freeCtlMsg(recvList *msg, ctlPtl *cp)
{
recvList *list;
int hopcount, rc;

    hopcount = cp->maxMsgs * 2;

    list = cp->firstMsg;

    while (list && --hopcount){         /* verify it's on the list */

        if (list == msg){

	    break;
	}
	list = list->next;

    }

    if (list != msg){

	if (hopcount == 0){
	     log_msg("freeCtlMsg: received message list on portal %d\n",cp->ptl);
	     CPerrno = EOHHELL;  /* corrupt linked list */
	}

        CPerrno = EINVAL;       /* bad argument */
        return -1;
    }

    if (cp->firstMsg == msg){
        cp->firstMsg = msg->next;
    }
    if (cp->lastMsg == msg){
        cp->lastMsg = msg->prev;
    }
    /*
    ** Users don't get a handle until the message is read, so they
    ** should never be freeing an unread message.  We put this code
    ** here anyway, in the event the library may want to free an
    ** unread message. 
    */

    if (cp->firstUnread == msg){
        rc = updateFirstUnread(cp);

	if (rc){
	    return -1;
	}
    }

    if (msg->prev) msg->prev->next = msg->next;
    if (msg->next) msg->next->prev = msg->prev;

    free(msg);

    return 0;
}

/***********************************************************************
** exported functions
***********************************************************************/
int 
srvr_init_control_ptl(int max_num_msgs)
{
int ptl, i, rc;

    if (max_num_msgs < 1){
        CPerrno = EINVAL;
        return SRVR_INVAL_PTL;
    }

    ptl = SRVR_INVAL_PTL;

    /*
    ** get an unused portal num
    */
    for (i=SRVR_MINSYSPTL; i<=SRVR_MAXSYSPTL; i++){
	if (!ptlNums[i-SRVR_MINCTLPTL]){
	    ptl = i;
	}
    }

    if (ptl == SRVR_INVAL_PTL){
	CPerrno = ERESOURCE;
    }
    else{
        rc = makeControlPortal(max_num_msgs, ptl);

        if (rc){
            ptl = SRVR_INVAL_PTL;
        }
    }

    return ptl;
}
int 
srvr_init_control_ptl_at(int max_num_msgs, ptl_pt_index_t ptl)
{
int rc;

    if ((max_num_msgs < 1) ||
        (ptl < SRVR_MINUSERPTL)  || (ptl > SRVR_MAXUSERPTL) ||
         ptlNums[ptl-SRVR_MINCTLPTL] ) {

        CPerrno = EINVAL;
        return -1;
    }

    rc = makeControlPortal(max_num_msgs, ptl);

    if (rc){
	return -1;
    }

    return 0;
}
int
srvr_release_control_ptl(ptl_pt_index_t ptl)
{
ctlPtl *cp;

    if ((ptl < SRVR_MINCTLPTL) || (ptl > SRVR_MAXCTLPTL)){
        CPerrno = EINVAL;
        return -1;
    }

    cp = findCtlPtl(ptl);

    if (cp == NULL){
       return 0;
    }

    deleteControlPortal(cp);
 
    return 0;
}
void
release_all_control_portals()
{
ctlPtl *cp;
int hopcount;

    hopcount = MAX_NUM_PTLS;

    cp = srvrFirstCtlPtl;

    while (cp && hopcount--){

        deleteControlPortal(cp);

	cp = cp->next;

    }
    if (hopcount == 0){
        log_error("release_all_control_portals: corrupt control portal structures");
    }
}

/*
**
** Semantics of this function changed from P2 version to P3.
** 
** If handle is !NULL, it's src_nid, src_pid and src_type
** fields may be used for matching.  In that case, we'll return
** the first message in the portal that matches.
** Caller should SRVR_CLEAR_HANDLE(handle) before setting matching
** criteria, if any.
**
** Return values:
**   0   no new messages, or none that match if matching 
**       criteria was given
**
**   -1  error  
**
**   1   new message or new matching message was found.
**
** If handle is NULL, we just consume the message after setting
** the msg_type and xfer_len fields (if they're not NULL).
**
** Otherwise, we set handle fields required by
** request processing routines.  Set msg_type to message type
** in data_xfer_req.  Write the data xfer request length to xfer_len.
** Sets user_data to point to user data section in data_xfer_req.
** Note this user data section does not persist after control message 
** is freed.
*/
int
srvr_get_next_control_msg(ptl_pt_index_t ptl, control_msg_handle *handle,
	      int *msg_type, int *xfer_len, char **user_data)
{

int rc, hopcount;
ctlPtl *cp;
data_xfer_msg *msgbuf;
recvList *list;
ptl_id_t match_nid, match_pid;
int match_type;

    cp = findCtlPtl(ptl);

    if (cp == NULL){
       CPerrno = EINVAL;
       return -1;
    }
    rc = processCtlPtlEvents(cp);

    if (rc == -1){   /* this is really bad */
        deleteControlPortal(cp);
	return -1;
    }

    if (cp->firstUnread == NULL){

	return 0;   /* no new messages in control portal */
    }

    if (handle->msg != NULL){
        /*
	** caller was supposed to do this
	*/
        SRVR_CLEAR_HANDLE(*handle);
    }

    if (handle){
        match_nid = SRVR_HANDLE_NID(*handle);
        match_pid = SRVR_HANDLE_PID(*handle);
        match_type = SRVR_HANDLE_TYPE(*handle);
    }
    else{
        match_nid = SRVR_ANY_NID;
        match_pid = SRVR_ANY_PID;
        match_type = SRVR_ANY_TYPE;
    }

    list = cp->firstUnread;

    if ( (match_nid != SRVR_ANY_NID) ||      /* search for a matching message */
         (match_pid != SRVR_ANY_PID) ||
         (match_type != SRVR_ANY_TYPE) ){

        hopcount = cp->maxMsgs * 2;

        while (list && --hopcount){

            if ( (list->msg.flag == CTLMSG_UNREAD)                                &&
	         ((match_nid==SRVR_ANY_NID)   || (match_nid==list->msg.src_nid))  &&
                 ((match_pid==SRVR_ANY_PID)   || (match_pid==list->msg.src_pid))  &&
                 ((match_type==SRVR_ANY_TYPE) || (match_type==list->msg.msg_type)) ){

                break;
            }

	    list = list->next;
	}

	if (hopcount == 0){
	    log_msg("srvr_get_next_control_msg: searching list for matching message\n");
	    CPerrno = EOHHELL;  /* corrupt internal data */
	    deleteControlPortal(cp);
	    return -1;
	}

	if (!list){           /* no matching unread messages */
	    return 0;
	}

    }

    msgbuf = &(list->msg);

    if (msg_type != NULL){
	*msg_type = msgbuf->msg_type;
    }
    if (xfer_len != NULL){
	*xfer_len = msgbuf->req_len;
    }

    if (handle != NULL){
 
	handle->msg = list;

	if (user_data != NULL){
	    *user_data = msgbuf->user_data;
	}

    }
    list->msg.flag = CTLMSG_READ;

    if (cp->firstUnread == list){

        rc = updateFirstUnread(cp);

	if (rc){
	    return -1;
	}
    }

    if (handle == NULL)
    {
        /*
        ** Just consume the control message since they can't
        ** free it without a handle.
        ** The user_data in the control message is invalid after
        ** message is freed, since it may be overwritten at any time.
        */
	if (user_data) *user_data = NULL;

        rc = freeCtlMsg(list, cp);

	if (rc){
            if (CPerrno != EINVAL){
                deleteControlPortal(cp);  /* bad error */
            }
	    return -1;
	}
    }

    return 1;
}
int
srvr_free_control_msg(ptl_pt_index_t ptl, control_msg_handle *handle)
{
ctlPtl *cp;
int rc, status;

    status = 0;

    cp = findCtlPtl(ptl);

    if (cp == NULL){
       CPerrno = EINVAL;
       return -1;
    }

    if (!handle || (handle->msg == NULL)){
        return 0;          /* nothing to free */
    }

    rc = freeCtlMsg(handle->msg, cp);

    if (rc){
	if (CPerrno != EINVAL){
	    deleteControlPortal(cp);  /* bad error */
	}
	status = -1;
    }

    return status;
}
int
srvr_free_all_control_msgs(ptl_pt_index_t ptl)
{
ctlPtl *cp;
int rc;
int nmsgs;

    cp = findCtlPtl(ptl);

    if (cp == NULL){
       CPerrno = EINVAL;
       return -1;
    }

    nmsgs = cp->maxMsgs; 

    deleteControlPortal(cp);

    rc = makeControlPortal(nmsgs, ptl);

    return rc;
}

/*
** Control portals exist to send requests to GET data from a remote
** process' memory, or PUT data in a remote process' memory.  But
** they carry SRVR_USR_DATA_LEN bytes of user data, so they can also
** be used to send a short message, with no subsequent data transfer
** expected.
*/

int
srvr_send_it(int nid, int pid, ptl_pt_index_t ptl,
             int msg_type, char *user_data, int len,
             int retptl, int reqlen, int retbits)
{
data_xfer_msg msg;
ptl_process_id_t sendto;
ptl_handle_md_t md;
ptl_md_t mddef;
ptl_match_bits_t mbits;
ptl_size_t noOffset;
ptl_hdr_data_t userData;
int rc;

    if (send_event_queue == SRVR_INVAL_EQ_HANDLE){
        rc = PtlEQAlloc(__SrvrLibNI, 2, &send_event_queue);

	if (rc != PTL_OK){
	    P3ERROR;
	}
    }

    msg.flag     = CTLMSG_UNPROCESSED;
    msg.src_nid  = _my_pnid;
    msg.src_pid  = _my_ppid;

    msg.msg_type = msg_type;

    msg.req_len  = reqlen;    /* These three fields are for puts   */
    msg.ret_ptl  = retptl;    /* and gets initiated with a control */
    msg.mbits    = retbits;   /* message.                          */
 
    if (len && user_data){
        memcpy(msg.user_data, user_data, len);
    }
    mddef.start = (void *)&msg;
    mddef.length = sizeof(data_xfer_msg);
    mddef.threshold = 1;
    mddef.max_offset = mddef.length;
    mddef.options   = 0;
    mddef.user_ptr  = NULL;
    mddef.eventq    = send_event_queue;

    rc = PtlMDBind(__SrvrLibNI, mddef, &md);

    if (rc != PTL_OK){
        P3ERROR;
    }

    sendto.nid = nid;
    sendto.pid = pid;
    sendto.addr_kind = PTL_ADDR_NID;

    mbits = ptl;   /* ptl is actually a ME in master control portal */
    noOffset = 0;
    userData  = 0;

    rc = PtlPut(md,
		PTL_NOACK_REQ,
		sendto,
		CONTROLPORTALS,
		SRVR_ACL_ANY,
		mbits,
		noOffset, userData);
 
 
    if (rc != PTL_OK){
        PtlMDUnlink(md);
	P3ERROR;
    }
 
    rc = srvr_await_send_completion(&mddef);

    if (rc){

      if (CPerrno == ESENDTIMEOUT){

          /*
          ** Not necessarily a bad thing - other node may not
          ** be up yet, or destination process may have gone away.
          **
          ** This call does a PtlMDUnlink.
          */
          srvrHandleSendTimeout(mddef, md);

          return -1;
      }
    }

    PtlMDUnlink(md);

    return rc;   /* 0 (send OK), -1 (send failure, see CPerrno) */
}
/*
** Wait up to SEND_TIMEOUT seconds for the outgoing
** message to leave the node.
** We assume PTL_NOACK_REQ, since we don't look for
** an ACK on the event queue.
*/
int
srvr_await_send_completion(ptl_md_t *mddef)
{
ptl_event_t ev;
time_t t1;
int rc;
       
    t1 = time(NULL);

    while ((time(NULL) - t1) < SEND_TIMEOUT){

        rc = PtlEQGet(mddef->eventq, &ev);

        if (rc == PTL_EQ_EMPTY){

	    continue; 

	} 
	else if ((rc == PTL_OK) && (ev.type == PTL_EVENT_SENT)){
	    return 0;
	} 
	else if ((rc == PTL_EQ_DROPPED) && (ev.type == PTL_EVENT_SENT)){
            log_msg("an event was dropped on libsrvr send event queue");
	    return 0;
	}
	else{
	    P3ERROR;
	}
    }


    CPerrno = ESENDTIMEOUT;

    return -1;
    
}
int
srvr_send_to_control_ptl(int nid, int pid, ptl_pt_index_t ptl,
	       int msg_type, char *user_data, int len)
{
int rc;

    if ( user_data && ((len > SRVR_USR_DATA_LEN) || (len < 0))) {
        CPerrno = EINVAL;
        return -1;
    }
    if ( (ptl < SRVR_MINCTLPTL) || (ptl > SRVR_MAXCTLPTL)){
        CPerrno = EINVAL;
        return -1;
    }

    rc = srvr_send_it(nid, pid, ptl,
                      msg_type, user_data, len,
		      0, 0, 0);

    return rc;   /* 0 (send OK), -1 (send failure, see CPerrno) */
}

/* srvr_send_to_control_ptls() sends a srvr control message to multiple 
 * targets.  nid, pid, ptl arguments are all arrays of size 
 * num_targets (unless sflag=1, in which case we send to the same pid
 * and ptl for each nid -- then pid and ptl can be a single int*).
 *
 * unlike srvr_send_to_control_ptl(), this function doesn't wait for the
 * PTL_EVENT_SENT to occur before returning.  It remembers the PtlPut()s
 * that it does and check them each time its called for PTL_EVENT_SENTs.  
 * Upon completion, the buffer being sent and all other resources are freed.
 */

#define SRVR_MAX_ASYNC_SENDS 250
#define STALL_TMOUT 100000       /* how long each stall takes in microseconds */
#define MAX_BLOCK   5            /* maximum number of seconds srvr_send_..._ptls can block */

int
srvr_send_to_control_ptls(int num_targets, int *nid, int *pid, ptl_pt_index_t *ptl,
                          int msg_type, char *user_data, int len, int sflag, int *wait_for)
{
    int rc;
    int i, j=0, ptlput_id;
    data_xfer_msg    *msg;       /* srvr message */
    ptl_handle_eq_t  eventq;     /* fresh event queue for each call */
    ptl_event_t      ev;
    ptl_process_id_t sendto;     /* identifies target to send to */
    ptl_md_t         mdef;       /* portal memory descriptor for srvr message */
    ptl_handle_md_t  md;
    ptl_match_bits_t mbits;
    int              num_ptlputs = 0;
    int              num_sent    = 0;
    int              *wait_array;
    int              *ptlput_array;
    int              *target_nids;
    time_t           start;

    /* printf("before prune_bufs():"); print_bufs(); */
    prune_bufs();
    /* printf("after prune_bufs():"); print_bufs(); */

    /* protect against the case of sending to zero targets */
    if (num_targets <= 0) log_error("Error in srvr_send_to_control_ptls(): num_targets == 0");

    /* make sure user_data is small enough to fit in ctl message */
    if ( user_data && ((len > SRVR_USR_DATA_LEN) || (len < 0))) {
        log_error("Error in srvr_send_to_control_ptls(): trying to send %d bytes, SRVR_USR_DATA_LEN = %d)",
                  len, SRVR_USR_DATA_LEN);
    }

    /* allocate a new msg. it can't be a stack var because if something
     * goes wrong, it will be remembered and freed later (in prune_bufs)
     * */
    msg = malloc(sizeof(data_xfer_msg));
    if (msg == NULL) log_error("Error in srvr_send_to_control_ptls(): can't allocate new message");

    wait_array = malloc(num_targets * sizeof(int));
    if (wait_array == NULL) log_error("Error in srvr_send_to_control_ptls(): can't allocate wait array");
    for (i = 0; i < num_targets; i++) wait_array[i] = 0;

    ptlput_array = malloc(num_targets * sizeof(int));
    if (ptlput_array == NULL) log_error("Error in srvr_send_to_control_ptls(): can't allocate ptlput array");
    for (i = 0; i < num_targets; i++) ptlput_array[i] = 0;

    /* each call gets a new event queue */
    rc = PtlEQAlloc(__SrvrLibNI, num_targets, &eventq);
    if (rc != PTL_OK) log_error("Error in srvr_send_to_control_ptls(): PtlEQAlloc() rc=%d", rc);

    /* fill in srvr message fields */
    msg->flag     = CTLMSG_UNPROCESSED;
    msg->src_nid  = _my_pnid;
    msg->src_pid  = _my_ppid;
    msg->msg_type = msg_type;
    msg->req_len  = 0;
    msg->ret_ptl  = 0;
    msg->mbits    = 0;
    if (len && user_data) {
        memcpy(&(msg->user_data), user_data, len);
    }

    /* fill in portal memory descriptor fields */
    mdef.start     = (void *)msg;
    mdef.length    = sizeof(data_xfer_msg);
    mdef.threshold = PTL_MD_THRESH_INF;

    mdef.max_offset = mdef.length;
    mdef.options   = 0;
    mdef.user_ptr  = NULL;
    mdef.eventq = eventq;

    /* bind the memory descriptor */
    rc = PtlMDBind(__SrvrLibNI, mdef, &md);
    if (rc != PTL_OK) log_error("Error in srvr_send_to_control_ptls(): PtlMDBind() rc=%d", rc);

    sendto.addr_kind = PTL_ADDR_NID;

    start = time(NULL);
    for (i = 0; i < num_targets; i++) {

        if (check_nid(nid[i])) {
            /* there's already an outstanding send to this node.  chances are the portal
             * sequence numbers are out-of-wack between these two nodes.  we don't want
             * to send any additional messages to this node
             * */
            log_warning("In srvr_send_to_control_ptls(): skipping send to target nid %d. portals seq #'s may be out of sync", nid[i]);
            log_warning("Suggest rebooting (or cplant stop/start) nid %d to resync portals numbers", nid[i]);
            continue; 
        }

        if (!sflag) j = i;
        sendto.nid = nid[i];
        sendto.pid = pid[j];

        /* encode the PtlPut() number in the upper 32 buts of the 64 bit match bits. 
         * Code Problem:
         * note that on i386 linux with older libc/glibc the match bits will only be 
         * 32 bits since sizeof(unsigned long long) = 4 bytes.  This stuff will fail 
         * on those systems.  With a newer glibc, sizeof(unsigned long long) = 8 bytes.  
         * The portals spec states that there are ptl_match_bits_t is capable of holding
         * unsigned 64 bit integer values.
         * */
        mbits = i;
        mbits = mbits << 32;
        mbits = mbits | ptl[j];
	
        rc = PtlPut(md, PTL_NOACK_REQ, sendto, CONTROLPORTALS, SRVR_ACL_ANY, mbits, 0, i);
        if (rc != PTL_OK) {
            if (rc == PTL_FAIL) {
                /* PTL_FAIL means portals can't send any more messages right now.  sleep a while and try again */
                log_warning("In srvr_send_to_control_ptls(): PtlPut() returned PTL_FAIL once");
                usleep(200000);
                rc = PtlPut(md, PTL_NOACK_REQ, sendto, CONTROLPORTALS, SRVR_ACL_ANY, mbits, 0, i);
                if (rc != PTL_OK) {
                    /* we're probably not going to get any more PtlPuts() out. stop trying */
                    log_warning("In srvr_send_to_control_ptls(): PtlPut() rc=PTL_FAIL twice");
                    break;
                }
            }
            else {
                /* errors other than PTL_FAIL are considered fatal */
                log_error("Error in srvr_send_to_control_ptls(): PtlPut() rc=%d", rc);
            }
        }

        /* remember that PtlPut() i succeeded */
        ++num_ptlputs;
        ptlput_array[i] = 1;
        wait_array[i]   = 1;

        /* hack to try to avoid the case where portals runs out of send "cookies" (it returns 
         * PTL_FAIL to indicate this).
         * */
        /* if (i && ((i % 100) == 0)) usleep(50000); */

	/* clear eventq of PTL_EVENT_SENT events, if any */
        while (1) {
            rc = PtlEQGet(eventq, &ev);

            if (rc == PTL_OK) {
                if (ev.type == PTL_EVENT_SENT) {
                    ptlput_id = (ev.match_bits >> 32);

                    /* sanity checks */
                    if ((ptlput_id < 0) || (wait_array[ptlput_id] == 0)) 
                        log_error("Error occured in srvr_send_to_control_ptls(): invalid PtlPut index (%d)", ptlput_id);

                    wait_array[ptlput_id] = 0;
                    ++num_sent;
                }
                else log_error("Error in srvr_send_to_control_ptls(): Invalid event (ev.type = %d)", ev.type);
            }
            else if (rc == PTL_EQ_EMPTY) {
                    if ((num_ptlputs - num_sent) <= SRVR_MAX_ASYNC_SENDS)
                        break;

                    if ((time(NULL) - start) > MAX_BLOCK)
                        break;

                    log_warning("In srvr_send_to_control_ptls(): %d async sends in flight, waiting till <= %d",
                        num_ptlputs - num_sent, SRVR_MAX_ASYNC_SENDS);

                    usleep(STALL_TMOUT);

            }
            else log_error("Error in srvr_send_to_control_ptls(): PtlEQGet returned rc=%d", rc);
        }

        if ((time(NULL) - start) > MAX_BLOCK) {
            log_warning("In srvr_send_to_control_ptls(): timeout spinning for PTL_EVENT_SENTs");
            log_warning("In srvr_send_to_control_ptls(): num_ptlputs=%d, num_sent=%d", num_ptlputs, num_sent);
            break;
        }

    } /* end foreach(target) */

    if (num_ptlputs != num_targets) {
        log_warning("In srvr_send_to_control_ptls(): skipped %d out of %d targets", num_targets - num_ptlputs, num_targets);
    }

    if (wait_for != NULL)
        for (i = 0; i < num_targets; i++) wait_for[i] = ptlput_array[i];
    free(ptlput_array);

    if (num_sent == num_ptlputs) {
        /* sanity check */
        for (i = 0; i < num_targets; i++) {
            if (wait_array[i] != 0) 
                log_error("Error in srvr_send_to_control_ptls(): wait_array not sane (wait_array[%d] = %d)", i, wait_array[i]);
        }

        free(msg);
        free(wait_array);
     
        rc = PtlMDUnlink(md);
        if (rc != PTL_OK) log_error("Error in srvr_send_to_control_ptls(): PtlMDUnlink() returned error (rc=%d)", rc);
        
        rc = PtlEQFree(eventq);
        if (rc != PTL_OK) log_error("Error in srvr_send_to_control_ptls(): PtlEQFree() returned error (rc=%d)", rc);
    }
    else {
        /* have to make a copy of the target_nids to pass to add_buf().  add_buf will
         * free this buffer, along with msg, eventq, and sent_array once all posted
         * PtlPuts() happen.
         * */
        target_nids = malloc(num_targets * sizeof(int));
        if (target_nids == NULL) log_error("Error in srvr_send_to_control_ptls(): can't allocate target_nids buf");
        memcpy(target_nids, nid, num_targets * sizeof(int));

        /* remember the buf. We have to wait for all PtlPuts() to happen before we can free msg. */
        add_buf((char *)msg, md, eventq, num_targets, target_nids, wait_array);

        /* printf("after add_buf(): "); print_bufs(); */
        /* print_target_nids(); */
    }

    /* return actual number of PtlPuts() that succeeded */
    return num_ptlputs;
}

void
srvr_display_data_xfer_msg(data_xfer_msg *msg)
{
    printf("flag %d (%s)\n",msg->flag,msgFlags[msg->flag]);
    printf("from %d/%d   type %d\n",msg->src_nid,msg->src_pid,msg->msg_type);
    printf("ptl %d len %d mbits %ld\n",
            msg->ret_ptl, msg->req_len, (long int) msg->mbits);
}
