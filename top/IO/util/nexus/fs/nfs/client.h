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
#include "queue.h"

/*
 * $Id: client.h,v 1.2 1980/01/06 13:02:21 lward Exp $
 */

TAILQ_HEAD(client_handle_list, client_handle_list_entry);

struct server;

/*
 * Idle RPC client handle list member.
 */
struct client_handle_list_entry {
	TAILQ_ENTRY(client_handle_list_entry) link;	/* srvr cache link */
	TAILQ_ENTRY(client_handle_list_entry) hlink;	/* all handles link */
	CLIENT	*clnt;					/* handle */
	struct server *srvr;				/* related srvr */
};

/*
 * Description, context and RPC client handles for a remote server.
 */
struct server {
	const char *host;				/* remote host name */
	u_long	program;				/* RPC prog # */
	u_long	vers;					/* RPC prog vers # */
	const char *protocol;				/* protocol */
	u_short port;					/* port number */
	struct client_handle_list cache;		/* cached handles */
#ifdef INSTRUMENT
	u_long cache_length;				/* current length */
	u_long max_cache_length;			/* longest list */
	u_long uses;					/* # calls */
#endif
};

/*
 * Initialize a new server record.
 */
#define __client_context_common_setup(srvr, h, prog, v, prot, prt) \
	do { \
		(srvr)->host = (h); \
		(srvr)->program = (prog); \
		(srvr)->vers = (v); \
		(srvr)->protocol = (prot); \
		(srvr)->port = (prt); \
		TAILQ_INIT(&(srvr)->cache); \
	} while (0)
#ifdef INSTRUMENT
#define client_context_setup(srvr, h, prog, v, prot, prt) \
	do { \
		__client_context_common_setup((srvr), \
					      (h), \
					      (prog), \
					      (v), \
					      (prot), \
					      (prt)); \
		(srvr)->cache_length = 0; \
		(srvr)->max_cache_length = 0; \
		(srvr)->uses = 0; \
	} while (0)
#else
#define client_context_setup(srvr, h, prog, v, prot, prt) \
	__client_context_common_setup((srvr), (h), (prog), (v), (prot), (prt))
#endif

extern struct server *client_context_create(const char *,
	u_long, u_long, const char *, u_short);
extern void client_context_destroy(struct server *);
extern CLIENT *client_activate(struct server *, struct timeval *,
	struct creds *);
extern void client_idle(struct server *, CLIENT *);
extern void client_destroy(struct server *, CLIENT *);
