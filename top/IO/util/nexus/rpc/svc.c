/* @(#)svc.c	2.4 88/08/11 4.0 RPCSRC; from 1.44 88/02/08 SMI */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc.c 1.41 87/10/13 Copyr 1984 Sun Micro";
#endif

/*
 * svc.c, Server-side remote procedure call interface.
 *
 * There are two sets of procedures here.  The xprt routines are
 * for handling transport handles.  The svc routines handle the
 * list of service routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"

#include "private.h"

IDENTIFY("$Id: svc.c,v 0.3 1999/12/20 22:45:47 lward Exp $");

static struct xprtcntl {
	SVCXPRT	*xc_xport;
	u_long	xc_waiters	: 16,
		xc_busy		: 16;
	cond_t xc_cond;
} *xports = NULL;

#define NULL_SVC ((struct svc_callout *)0)
#define	RQCRED_SIZE	400		/* this size is excessive */

static mutex_t xprt_lock = MUTEX_INITIALIZER;

/*
 * We open a pipe (to nowhere but ourself) in order to handle the
 * problem of noticing new transports while a thread is blocked in
 * select.
 */
static int pipedes[2];
static unsigned pipecount;

extern size_t _rpc_dtablesize();

/*
 * The services list
 * Each entry represents a set of procedures (an rpc program).
 * The dispatch routine takes request structs and runs the
 * apropriate procedure.
 */
static struct svc_callout {
	struct svc_callout *sc_next;
	u_long		    sc_prog;
	u_long		    sc_vers;
	void		    (*sc_dispatch)();
	u_long		    sc_waiters		: 16,
			    sc_busy		: 16;
	cond_t	    sc_cond;
} *svc_head = NULL;

static mutex_t svc_lock = MUTEX_INITIALIZER;

static struct svc_callout *svc_find();

/* ***************  SVCXPRT related stuff **************** */

/*
 * Initialize transports control.
 */
static void
xprt_init()
{
	struct xprtcntl *myxports;
	int fd;

	myxports =
	  (struct xprtcntl *)mem_alloc(FD_SETSIZE * sizeof(struct xprtcntl));
	for (fd = 0; fd < FD_SETSIZE; fd++) {
		myxports[fd].xc_xport = (SVCXPRT *)0;
		myxports[fd].xc_waiters = 0;
		myxports[fd].xc_busy = 0;
		cond_init(&myxports[fd].xc_cond);
	}

	if (pipe(pipedes) != 0) {
		(void )fprintf(stderr, "can't create control pipe\n");
		exit(1);
	}
	xports = myxports;
}

/*
 * The select() used in the svc_run routine has no way to be made
 * aware of new transports being registered until it receives input from
 * an existing descriptor. We get around that little problem here.
 */
int
_rpc_aware_select(n, readfdsp, writefdsp, exceptfdsp, timeout)
	int n;
	fd_set *readfdsp;
	fd_set *writefdsp;
	fd_set *exceptfdsp;
	struct timeval *timeout;
{
	int	nfd;
	int	rtn;

	FD_SET(pipedes[0], readfdsp);
	nfd = _rpc_dtablesize();
	if (n >= nfd)
		n = nfd - 1;
	rtn = select(n + 1, readfdsp, writefdsp, exceptfdsp, timeout);
	if (rtn > 0 && FD_ISSET(pipedes[0], readfdsp)) {
		char	c;

		mutex_lock(&xprt_lock);
		pipecount--;
		mutex_unlock(&xprt_lock);
		(void )read(pipedes[0], &c, 1);
		rtn--;
	}
	FD_CLR(pipedes[0], readfdsp);

	return rtn;
}

/*
 * Activate a transport handle.
 */
void
xprt_register(xprt)
	SVCXPRT *xprt;
{
	register int sock = xprt->xp_sock;
	static once_t once_control = ONCE_INIT;

	if (xports == NULL)
		thread_once(&once_control, xprt_init);
	if (sock < 0 || sock >= FD_SETSIZE || sock >= (int )_rpc_dtablesize())
		return;
	mutex_lock(&xprt_lock);
	/* there can be only one per fildes */
	while (xports[sock].xc_xport != (SVCXPRT *)0) {
		mutex_unlock(&xprt_lock);
		xprt_unregister(xports[sock].xc_xport);
		mutex_lock(&xprt_lock);
	}
	xports[sock].xc_xport = xprt;
	FD_SET(sock, &svc_fdset);
	if (!pipecount) {
		char	c;

		c = '\0';
		(void )write(pipedes[1], &c, 1);
		pipecount++;
	}
	mutex_unlock(&xprt_lock);
}

/*
 * De-activate a transport handle. 
 */
void
xprt_unregister(xprt) 
	SVCXPRT *xprt;
{ 
	register int sock = xprt->xp_sock;

	if (sock < 0 || sock >= FD_SETSIZE || sock >= (int )_rpc_dtablesize())
		return;
	mutex_lock(&xprt_lock);
	if (xports[sock].xc_xport != xprt) {
		mutex_unlock(&xprt_lock);
		return;
	}
	assert(++xports[sock].xc_waiters);
	if (xports[sock].xc_waiters <= 1) {
		while (xports[sock].xc_busy)
			cond_wait(&xports[sock].xc_cond, &xprt_lock);
		xports[sock].xc_xport = (SVCXPRT *)0;
		FD_CLR(sock, &svc_fdset);
	} else {
		while (xports[sock].xc_waiters)
			cond_wait(&xports[sock].xc_cond, &xprt_lock);
	}
	xports[sock].xc_waiters--;
	if (xports[sock].xc_waiters)
		cond_signal(&xports[sock].xc_cond);
	mutex_unlock(&xprt_lock);
}


/* ********************** CALLOUT list related stuff ************* */

/*
 * Add a service program to the callout list.
 * The dispatch routine will be called when a rpc request for this
 * program number comes in.
 */
bool_t
svc_register(xprt, prog, vers, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)();
	int protocol;
{
	struct svc_callout *prev;
	register struct svc_callout *s;
	bool_t rtnval;

	rtnval = TRUE;
	mutex_lock(&svc_lock);
	if ((s = svc_find(prog, vers, &prev)) != NULL_SVC) {
		if (s->sc_dispatch == dispatch) {
			assert(++s->sc_busy);
			goto pmap_it;  /* he is registering another xptr */
		}
		mutex_unlock(&svc_lock);
		return (FALSE);
	}
	s = (struct svc_callout *)mem_alloc(sizeof(struct svc_callout));
	if (s == (struct svc_callout *)0) {
		mutex_unlock(&svc_lock);
		return (FALSE);
	}
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_next = svc_head;
	s->sc_waiters = 0;
	s->sc_busy = 1;
	cond_init(&s->sc_cond);
	svc_head = s;
pmap_it:
	mutex_unlock(&svc_lock);
	/* now register the information with the local binder service */
	if (protocol) {
		rtnval = pmap_set(prog, vers, protocol, xprt->xp_port);
	}
	mutex_lock(&svc_lock);
	s->sc_busy--;
	if (s->sc_waiters)
		cond_signal(&s->sc_cond);
	mutex_unlock(&svc_lock);
	return (rtnval);
}

/*
 * Remove a service program from the callout list.
 */
void
svc_unregister(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	register struct svc_callout *s;

	mutex_lock(&svc_lock);
	if ((s = svc_find(prog, vers, &prev)) == NULL_SVC) {
		mutex_unlock(&svc_lock);
		return;
	}
	if (prev == NULL_SVC) {
		svc_head = s->sc_next;
	} else {
		prev->sc_next = s->sc_next;
	}
	s->sc_next = NULL_SVC;
	while (s->sc_busy)
		cond_wait(&s->sc_cond, &svc_lock);
	assert(++s->sc_busy);
	mutex_unlock(&svc_lock);
	/* now unregister the information with the local binder service */
	(void)pmap_unset(prog, vers);
	mutex_lock(&svc_lock);
	assert(!--s->sc_busy);
	assert(!s->sc_waiters);
	mutex_unlock(&svc_lock);
	cond_destroy(&s->sc_cond);
	mem_free((char *) s, (u_int) sizeof(struct svc_callout));
}

/*
 * Search the callout list for a program number, return the callout
 * struct.
 *
 * NOTE: The service lock should be held.
 */
static struct svc_callout *
svc_find(prog, vers, prev)
	u_long prog;
	u_long vers;
	struct svc_callout **prev;
{
	register struct svc_callout *s, *p;

	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if ((s->sc_prog == prog) && (s->sc_vers == vers))
			goto done;
		p = s;
	}
done:
	*prev = p;
	return (s);
}

/* ******************* REPLY GENERATION ROUTINES  ************ */

/*
 * Send a reply to an rpc request
 */
bool_t
svc_sendreply(xprt, xdr_results, xdr_location)
	register SVCXPRT *xprt;
	xdrproc_t xdr_results;
	caddr_t xdr_location;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY;  
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf; 
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;
	return (SVC_REPLY(xprt, &rply)); 
}

/*
 * No procedure error reply
 */
void
svcerr_noproc(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Can't decode args error reply
 */
void
svcerr_decode(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply); 
}

/*
 * Some system error
 */
void
svcerr_systemerr(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = SYSTEM_ERR;
	SVC_REPLY(xprt, &rply); 
}

/*
 * Authentication error reply
 */
void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
}

/*
 * Auth too weak error reply
 */
void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
{

	svcerr_auth(xprt, AUTH_TOOWEAK);
}

/*
 * Program unavailable error reply
 */
void 
svcerr_noprog(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;  

	rply.rm_direction = REPLY;   
	rply.rm_reply.rp_stat = MSG_ACCEPTED;  
	rply.acpted_rply.ar_verf = xprt->xp_verf;  
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Program version mismatch error reply
 */
void  
svcerr_progvers(xprt, low_vers, high_vers)
	register SVCXPRT *xprt; 
	u_long low_vers;
	u_long high_vers;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = low_vers;
	rply.acpted_rply.ar_vers.high = high_vers;
	SVC_REPLY(xprt, &rply);
}

/* ******************* SERVER INPUT STUFF ******************* */

/*
 * Get server side input from some transport.
 *
 * Statement of authentication parameters management:
 * This function owns and manages all authentication parameters, specifically
 * the "raw" parameters (msg.rm_call.cb_cred and msg.rm_call.cb_verf) and
 * the "cooked" credentials (rqst->rq_clntcred).
 * However, this function does not know the structure of the cooked
 * credentials, so it make the following assumptions: 
 *   a) the structure is contiguous (no pointers), and
 *   b) the cred structure size does not exceed RQCRED_SIZE bytes. 
 * In all events, all three parameters are freed upon exit from this routine.
 * The storage is trivially management on the call stack in user land, but
 * is mallocated in kernel land.
 */

void
svc_getreq(rdfds)
	int rdfds;
{
#if 0
	fd_set readfds;

	FD_ZERO(&readfds);
	readfds.fds_bits[0] = rdfds;
	svc_getreqset(&readfds);
#else
	assert(0);
#endif
}

void
_t_svc_getrequest(sock, f)
	int sock;
	void (*f)();
{
	SVCXPRT myxport;
	enum xprt_stat stat;
	int	err;
	struct rpc_msg msg;
	int prog_found;
	u_long low_vers;
	u_long high_vers;
	struct svc_req r;
	register SVCXPRT *xprt;
	char cred_area[2*MAX_AUTH_BYTES + RQCRED_SIZE];
	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r.rq_clntcred = &(cred_area[2*MAX_AUTH_BYTES]);


	if (sock < 0 || sock >= FD_SETSIZE || sock >= (int )_rpc_dtablesize())
		return;
	mutex_lock(&xprt_lock);
	myxport = *xports[sock].xc_xport;
	assert(++xports[sock].xc_busy);
	mutex_unlock(&xprt_lock);
	xprt = &myxport;
	/* now receive msgs from xprtprt (support batch calls) */
	do {
		err = SVC_RECV(xprt, &msg);
		/*
		 * We don't support batching and so we can safely
		 * tell svc_run we've acquired this request now.
		 */
		if (f != NULL)
			(*f)(sock);

		if (err) {

			/* now find the exported program and call it */
			register struct svc_callout *s;
			enum auth_stat why;

			r.rq_xprt = xprt;
			r.rq_prog = msg.rm_call.cb_prog;
			r.rq_vers = msg.rm_call.cb_vers;
			r.rq_proc = msg.rm_call.cb_proc;
			r.rq_cred = msg.rm_call.cb_cred;
			/* first authenticate the message */
			if ((why= _authenticate(&r, &msg)) != AUTH_OK) {
				svcerr_auth(xprt, why);
				goto call_done;
			}
			/* now match message with a registered service*/
			prog_found = FALSE;
			low_vers = 0 - 1;
			high_vers = 0;
			mutex_lock(&svc_lock);
			for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
				if (s->sc_prog == r.rq_prog) {
					if (s->sc_vers == r.rq_vers) {
						assert(++s->sc_busy);
						break;
					} /* found correct prog/vers */
					prog_found = TRUE;
					if (s->sc_vers < low_vers)
						low_vers = s->sc_vers;
					if (s->sc_vers > high_vers)
						high_vers = s->sc_vers;
				}   /* found correct program */
			}
			mutex_unlock(&svc_lock);
			if (s != NULL_SVC) {
				(*s->sc_dispatch)(&r, xprt);
				mutex_lock(&svc_lock);
				s->sc_busy--;
				if (s->sc_waiters && !s->sc_busy)
					cond_signal(&s->sc_cond);
				mutex_unlock(&svc_lock);
			} else if (prog_found)
				svcerr_progvers(xprt, low_vers, high_vers);
			else
				svcerr_noprog(xprt);
			/* Fall through to ... */
		}
	call_done:
		stat = SVC_STAT(xprt);
		mutex_lock(&xprt_lock);
		xports[sock].xc_busy--;
		if (xports[sock].xc_waiters && !xports[sock].xc_busy)
			cond_signal(&xports[sock].xc_cond);
		mutex_unlock(&xprt_lock);
		if ((stat = SVC_STAT(xprt)) == XPRT_DIED){
			SVC_DESTROY(xprt);
			break;
		}
	} while (stat == XPRT_MOREREQS);
}

void
svc_getrequest(sock)
	int sock;
{

	_t_svc_getrequest(sock, NULL);
}
