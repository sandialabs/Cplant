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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "cmn.h"
#include "smp.h"
#include "creds.h"
#include "rpc/rpc.h"

#include "client.h"

IDENTIFY("$Id: client.c,v 0.9 2001/07/18 18:57:31 rklundt Exp $");

/*
 * Client RPC handle management.
 *
 * Externally, client handles are related (tightly) to server records. The
 * record contains all information needed to create/destroy such RPC handles.
 * The server record also contains a linked list head that is used to
 * cache client handles for a particular server/program/version.
 *
 * Internally, cached handles are also maintained on the handles list.
 * If a request is made for another client handle and none are cached, one is
 * created.
 *
 * When a handle is idle it is added to the head of the handles list, as well as
 * the list of cached handles in the server record. If and when the system limit
 * is reached for existing file handles, one is removed from the tail of the
 * internal handles list and the new one created. If the internal handles list
 * is empty. The calling thread is blocked until an entry is deposited.
 *
 * TODO:
 *	Client handles for two or more server records for the same
 *	host/prog/vers should be shared in common.
 *
 *	Right now auth info is (re)created at every remote call from the
 *	credentials passed. Would it be better if they too were cached?
 *	This needs to be determined.
 */

/*
 * Note: Rapid creation/destruction of client handles eventually leads to a
 * problem with gethostbyname_r in .../rpc/clnt_generic.c. With caching
 * enabled, as it should be, this should only very rarely be encountered and
 * is not fatal. Still, it would be nice to eliminate the problem entirely.
 */

/*
 * Portions Copyrighted by Sun Microsystems. From the ONC/RPC RPCSRC 4.0
 * distribution.
 *
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

/*
 * NSTORES is the number of stores that the system will connect to. This
 * is used to determine the number of active client handles cached
 * in the system. The definition of DESIRED_CACHED_CLIENT_HANDLES, below,
 * is predicated on nservers threads of execution. If thread creation is
 * automatic, 10 is assumed.
 */
#ifndef DESIRED_CACHED_CLIENT_HANDLES
#define DESIRED_CACHED_CLIENT_HANDLES	(NSTORES * (nservers ? nservers : 10))
#endif

/*
 * Number of existing client handles desired in the system.
 *
 * NB: The actual number may be momentarily greater.
 */
unsigned desired_cached_client_handles = 0;

static mutex_t mutex = MUTEX_INITIALIZER;		/* package mutex */
static cond_t cond = COND_INITIALIZER;			/* alloc sleep */
static struct client_handle_list handles =		/* idle handles */
    TAILQ_HEAD_INITIALIZER(handles);
static struct client_handle_list free_entries_list =	/* free entries */
    TAILQ_HEAD_INITIALIZER(handles);

static unsigned sleepers = 0;				/* # alloc waiters */
static unsigned nhandles = 0;				/* # handles */

/*
 * Create a context to be used in the acquisition of file handles to a
 * particular server/prog/vers/proto.
 */
struct server *
client_context_create(const char *host,
		      u_long prog,
		      u_long vers,
		      const char *protocol,
		      u_short port)
{
	struct server *srvr;

	srvr = m_alloc(sizeof(struct server));
	if (srvr == NULL)
		return NULL;
	client_context_setup(srvr, host, prog, vers, protocol, port);
	srvr->host = m_alloc(strlen(host) + 1);
	if (srvr->host != NULL)
		(void )strcpy((char *)srvr->host, (char *)host);
	srvr->protocol = m_alloc(strlen(host) + 1);
	if (srvr->protocol != NULL)
		(void )strcpy((char *)srvr->protocol, (char *)protocol);

	if (srvr->host == NULL || srvr->protocol == NULL) {
		if (srvr->host != NULL)
			free((char *)srvr->host);
		if (srvr->protocol != NULL)
			free((char *)srvr->protocol);
		free(srvr);
		srvr = NULL;
	}

	return srvr;
}

/*
 * Destroy a previously created context. The server record pointed at is
 * no longer valid at return.
  */
void
client_context_destroy(struct server *srvr)
{
	struct client_handle_list_entry *entry;

	/*
	 * Destroy all the associated client handles.
	 *
	 * There's a chance that these handles may still be useful. However,
	 * since they need to be associated with a server record and this
	 * is the only one we know about, they're really destroyed. They'll
	 * be recreated if they are still needed.
	 */
	mutex_lock(&mutex);
	while ((entry = TAILQ_FIRST(&srvr->cache)) != NULL) {
		TAILQ_REMOVE(&entry->srvr->cache, entry, link);
#ifdef INSTRUMENT
		assert(srvr->cache_length--);
#endif
		TAILQ_REMOVE(&handles, entry, hlink);
		assert(nhandles--);
		mutex_unlock(&mutex);
		clnt_destroy(entry->clnt);
		free(entry);
		mutex_lock(&mutex);
	}
	mutex_unlock(&mutex);

	free((char *)srvr->host);
	free((char *)srvr->protocol);
	free(srvr);
}

#ifndef NFS_ADDRESS_IS_INTERFACE
/*
 * Get this host's hostname.
 *
 * Beware: Returned data is static.
 */
static const char *
hostname(void)
{
	static char *s = NULL;
	int	err;
	static char buf[128];
	static mutex_t	my_mutex = MUTEX_INITIALIZER;

	if (s != NULL)
		return s;

	mutex_lock(&my_mutex);
	err = gethostname(buf, sizeof(buf) - 1);
	if (err)
		panic("hostname: gethostname failed (%s)", strerror(errno));
	buf[sizeof(buf) - 1] = '\0';
	s = buf;
	mutex_unlock(&my_mutex);

	return s;
}
#endif

#ifdef NFS_ADDRESS_IS_INTERFACE
/*
 * Internal gethostbyaddr_r
 */
static int
my_gethostbyaddr_r(const char *addr,
		   int len,
		   int type,
		   struct hostent *result_buf,
		   char *buf,
		   size_t buflen,
		   struct hostent **result,
		   int *h_errnop)
{

	/*
	 * TODO:
	 * We really should use a cache. This is going to be terribly
	 * slow in some circumstances.
	 */
	return gethostbyaddr_r(addr,
			       len,
			       type,
			       result_buf,
			       buf,
			       buflen,
			       result,
			       h_errnop);
}
#endif

#ifndef NFS_ADDRESS_IS_INTERFACE
#define _MIGHT_BE_UNUSED	IS_UNUSED
#else
#define _MIGHT_BE_UNUSED
#endif
/*
 * Create new RPC authorization.
 */
static AUTH *
auth_create(CLIENT *client _MIGHT_BE_UNUSED, struct creds *crp)
{
	int	gids[NGRPS];
	size_t	indx;
	int	*ip;
#ifdef NFS_ADDRESS_IS_INTERFACE
	int	fd;
	struct sockaddr_in sin;
	size_t	len;
	char	*buf;
	struct hostent h, *hp;
	int	herrnum;
#endif
	AUTH	*aup;

	/*
	 * Translate credentials to NFS form.
	 */
	for (indx = 0, ip = gids; indx < NGRPS; indx++, ip++)
		*ip = indx >= crp->cr_ngroups ? -1 : crp->cr_groups[indx];

#ifdef NFS_ADDRESS_IS_INTERFACE
	/*
	 * We need to build the UNIX auth so that the requesting machine
	 * name translates to the name associated with the outgoing
	 * address.
	 */
	if (!CLNT_CONTROL(client, CLGET_FD, &fd)) {
		LOG(LOG_ERR, "auth_create: can't get connection descriptor");
		return NULL;
	}
	len = sizeof(sin);
	if (getsockname(fd, (struct sockaddr *)&sin, &len) != 0) {
		LOG(LOG_ERR, "auth_create: getsockname: %s", strerror(errno));
		return NULL;
	}
	buf = m_alloc(BUFSIZ);
	if (buf == NULL) {
		LOG(LOG_ERR, "auth_create: buf: %s", strerror(errno));
		return NULL;
	}
	if (my_gethostbyaddr_r((char *)&sin.sin_addr,
			       sizeof(sin.sin_addr),
			       AF_INET,
			       &h,
			       buf,
			       BUFSIZ,
			       &hp,
			       &herrnum) != 0) {
		LOG(LOG_ERR,
		        "auth_create: gethostbyaddr: %s",
			hstrerror(herrnum));
		free(buf);
		return NULL;
	}
#endif

	/*
	 * Finally, build the NFS auth.
	 */
	aup =
	    authunix_create(
#ifndef NFS_ADDRESS_IS_INTERFACE
			    hostname(),
#else
			    hp->h_name,
#endif
			    crp->cr_uid,
			    gids[0],
			    indx,
			    gids);
#ifdef NFS_ADDRESS_IS_INTERFACE
	free(buf);
#endif
	if (aup == NULL)
		LOG(LOG_ERR, "auth_create: authunix_create failed");
	return aup;
}
#undef _MIGHT_BE_UNUSED

/*
 * Create a new client handle. The bulk of this routine was lifted from
 * clnt_create() in .../rpc/clnt_generic.c from the RPCSRC 4.0 distribution.
 */
static CLIENT *
create_new_client_handle(struct server *srvr)
{
	char *buf;
	struct hostent h, *hp;
	int host_errno;
	struct protoent p, *pp;
	struct sockaddr_in sin;
	int sock;
	struct timeval tv;
	CLIENT *client;
	int protonum;
#if EXCLUSIVE_HANDLE_CREATE
	static mutex_t mymutex = MUTEX_INITIALIZER;
#endif

#if EXCLUSIVE_HANDLE_CREATE
	mutex_lock(&mymutex);
#endif
	buf = m_alloc(BUFSIZ);
	if (buf == NULL) {
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
#if EXCLUSIVE_HANDLE_CREATE
		mutex_unlock(&mymutex);
#endif
		return (NULL);
	}
	if (gethostbyname_r(srvr->host, &h, buf, BUFSIZ, &hp, &host_errno) != 0)
		hp = NULL;
	if (hp == NULL)
		rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
	if (hp != NULL && hp->h_addrtype != AF_INET) {
		/*
		 * Only support INET for now
		 */
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = EAFNOSUPPORT; 
		hp = NULL;
	}
	if (hp != NULL) {
		sin.sin_family = hp->h_addrtype;
		sin.sin_port = htons(srvr->port);
		bzero(sin.sin_zero, sizeof(sin.sin_zero));
		bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
	}
	if (hp == NULL) {
		free(buf);
#if EXCLUSIVE_HANDLE_CREATE
		mutex_unlock(&mymutex);
#endif
		return (NULL);
	}
	if (getprotobyname_r(srvr->protocol, &p, buf, BUFSIZ, &pp) != 0)
		pp = NULL;
	if (pp == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT; 
		mutex_unlock(&rpc_global_lock);
		free(buf);
#if EXCLUSIVE_HANDLE_CREATE
		mutex_unlock(&mymutex);
#endif
		return (NULL);
	}
	protonum = pp->p_proto;
	pp = NULL;
	sock = RPC_ANYSOCK;
	client = NULL;
	switch (protonum) {
	case IPPROTO_UDP:
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		client =
		    clntudp_create(&sin,
				   srvr->program,
				   srvr->vers,
				   tv,
				   &sock);
		if (client == NULL)
			break;
#ifdef NFS_ADDRESS_IS_INTERFACE
		/*
		 * The routine auth_create(), above, will need to determine
		 * the host name for the sending interface. To acquire that
		 * information it uses getsockname() on the file descriptor
		 * just created. However, if we don't set the remote address,
		 * the system can use any free port for the send. This connect
		 * fixes the port.
		 */
		if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno; 
			clnt_destroy(client);
			client = NULL;
		}
#endif
		break;
	case IPPROTO_TCP:
		client =
		    clnttcp_create(&sin,
				   srvr->program,
				   srvr->vers,
				   &sock,
				   0, 0);
		break;
	default:
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = EPFNOSUPPORT; 
		break;
	}
	free(buf);
#if 0
	if (client != NULL)
		clnt_control(client, CLSET_TIMEOUT, &tv);
#endif

#if EXCLUSIVE_HANDLE_CREATE
	mutex_unlock(&mymutex);
#endif

	return (client);
}

/*
 * Create a new client handle.
 */
static CLIENT *
client_create_handle(struct server *srvr)
{
	struct client_handle_list_entry *entry;
	CLIENT	*clnt;
	struct timeval tv;
	static struct timeval tzero = {0, 0};
	static unsigned long destroys = 0;

	mutex_lock(&mutex);
	if (!desired_cached_client_handles)
		desired_cached_client_handles = DESIRED_CACHED_CLIENT_HANDLES;
	while (nhandles >= desired_cached_client_handles) {
		/*
		 * First, try to make room by destroying an aged entry.
		 */
		entry = TAILQ_LAST(&handles, client_handle_list);
		if (entry != NULL) {
			TAILQ_REMOVE(&entry->srvr->cache, entry, link);
#ifdef INSTRUMENT
			assert(entry->srvr->cache_length--);
#endif
			TAILQ_REMOVE(&handles, entry, hlink);
			clnt = entry->clnt;
			entry->clnt = NULL;
			entry->srvr = NULL;
			TAILQ_INSERT_HEAD(&free_entries_list, entry, link);
			assert(nhandles--);
			mutex_unlock(&mutex);
			clnt_destroy(clnt);
			mutex_lock(&mutex);
			if (gettimeofday(&tv, NULL) == 0) {
				/*
				 * If the cache is flushed too often then
				 * we're thrashing and should tell the
				 * administrator.
				 */
				if (tv.tv_sec - tzero.tv_sec > 1 ||
				    (tv.tv_sec > tzero.tv_sec &&
				     tv.tv_usec > tzero.tv_usec)) {
					if (tv.tv_sec - tzero.tv_sec == 1 &&
					    destroys &&
					    (((1000000 +
					       (tv.tv_usec - tzero.tv_usec)) /
					      destroys) <
					     (1000000 / (3 * (nservers + 1)))))
						LOG(LOG_WARNING,
 "create_new_client_handle: thrashing -- adjust DESIRED_CACHED_CLIENT_HANDLES");
					tzero = tv;
					destroys = 0;
				}
				destroys++;
			}
			continue;
		}

		/*
		 * Ok, wait until we can make room. Then try again.
		 */
		assert(++sleepers);
		cond_wait(&cond, &mutex);
		assert(sleepers--);
	}
	mutex_unlock(&mutex);

	/*
	 * Create a new client handle.
	 */
	clnt = create_new_client_handle(srvr);
	if (clnt == NULL) {
		LOG(LOG_ERR,
		    "client_get_handle: [%lu,%lu] %s",
		    srvr->program, srvr->vers,
		    clnt_spcreateerror(srvr->host));
		return NULL;
	}
	auth_destroy(clnt->cl_auth);			/* dynamic! */
	clnt->cl_auth = NULL;
	mutex_lock(&mutex);
	assert(++nhandles);				/* record new */
	mutex_unlock(&mutex);

	return clnt;
}

/*
 * Find an idle client RPC handle or create a new one.
 */
static CLIENT *
client_get_handle(struct server *srvr, struct timeval *timotvp)
{
	CLIENT	*clnt;
	struct client_handle_list_entry *entry;

	clnt = NULL;
	/*
	 * Try to get an idle one from the server's handle cache.
	 */
	mutex_lock(&mutex);
	entry = TAILQ_FIRST(&srvr->cache);
	if (entry != NULL) {
		/*
		 * Found one! Clear it from the entry and put the
		 * empty entry on the free entries list for later
		 * reuse.
		 */
		TAILQ_REMOVE(&srvr->cache, entry, link);
#ifdef INSTRUMENT
		assert(srvr->cache_length--);
#endif
		TAILQ_REMOVE(&handles, entry, hlink);
		clnt = entry->clnt;
		entry->clnt = NULL;
		entry->srvr = NULL;
		TAILQ_INSERT_HEAD(&free_entries_list, entry, link);
	}
	mutex_unlock(&mutex);
	if (clnt == NULL) {
		/*
		 * None in the cache, try to create a new one.
		 */
		clnt = client_create_handle(srvr);
	}
	if (!(timotvp == NULL || clnt == NULL))
		clnt_control(clnt, CLSET_TIMEOUT, timotvp);
	return clnt;
}

/*
 * Destroy an active client RPC handle.
 */
void
client_destroy(struct server *srvr IS_UNUSED, CLIENT *clnt)
{
	struct client_handle_list_entry *entry;

	/*
	 * Shed credentials.
	 */
	if (clnt->cl_auth != NULL) {
		auth_destroy(clnt->cl_auth);
		clnt->cl_auth = NULL;
	}

	/*
	 * Destroy the handle.
	 */
	clnt_destroy(clnt);

	/*
	 * Any associated free list entry should be destroyed. It doesn't
	 * matter if there are more handles than free list entries.
	 */
	mutex_lock(&mutex);
	entry = TAILQ_LAST(&free_entries_list, client_handle_list);
	if (entry != NULL) {
		TAILQ_REMOVE(&free_entries_list, entry, link);
		free(entry);
	}
	assert(nhandles--);
	if (sleepers)
		cond_signal(&cond);
	mutex_unlock(&mutex);
}

/*
 * Put a no longer active RPC client handle on the idle list of the
 * given server record.
 */
static void
client_put(struct server *srvr, CLIENT *clnt)
{
	struct client_handle_list_entry *entry;

	mutex_lock(&mutex);
	/*
	 * The handle creation routine can create more handles than
	 * desired. If it has, destroy the extra now.
	 *
	 * FIX ME!
	 * We should be looking at the most idle handle. This certainly
	 * isn't it.
	 */
	if (nhandles > desired_cached_client_handles) {
		mutex_unlock(&mutex);
		client_destroy(srvr, clnt);
		return;
	}
	/*
	 * Grab first entry on the free entries list.
	 */
	entry = TAILQ_FIRST(&free_entries_list);
	if (entry != NULL) {
		BUG_CHECK(entry->clnt == NULL);
		TAILQ_REMOVE(&free_entries_list, entry, link);
	}
	mutex_unlock(&mutex);
	if (entry == NULL) {
		/*
		 * Need a new list entry.
		 */
		entry = m_alloc(sizeof(struct client_handle_list_entry));
		if (entry == NULL) {
			LOG(LOG_ERR,
			    "client_put: can't create free list entry");
			clnt_destroy(clnt);
			return;
		}
	}

	mutex_lock(&mutex);
	entry->clnt = clnt;
	entry->srvr = srvr;
	TAILQ_INSERT_HEAD(&srvr->cache, entry, link);
#ifdef INSTRUMENT
	srvr->cache_length++;
	if (srvr->cache_length > srvr->max_cache_length)
		srvr->max_cache_length = srvr->cache_length;
#endif
	TAILQ_INSERT_HEAD(&handles, entry, hlink);
	if (sleepers)
		cond_signal(&cond);
	mutex_unlock(&mutex);
}

/*
 * Activate an idle client RPC handle or create a new one.
 */
CLIENT *
client_activate(struct server *srvr, struct timeval *timotvp, struct creds *crp)
{
	CLIENT	*clnt;

	/*
	 * Get a handle.
	 */
	clnt = client_get_handle(srvr, timotvp);
	if (clnt == NULL)
		return NULL;

	/*
	 * Set up credentials in the handle just acquired.
	 */
	clnt->cl_auth = auth_create(clnt, crp);
	if (clnt->cl_auth == NULL) {
		LOG(LOG_ERR, "client_activate: can't create auth");
		client_put(srvr, clnt);
		return NULL;
	}

#ifdef INSTRUMENT
	mutex_lock(&mutex);
	srvr->uses++;
	mutex_unlock(&mutex);
#endif
	return clnt;
}

/*
 * Idle an active client RPC handle.
 */
void
client_idle(struct server *srvr, CLIENT *clnt)
{

	/*
	 * Shed credentials.
	 */
	if (clnt->cl_auth != NULL) {
		auth_destroy(clnt->cl_auth);
		clnt->cl_auth = NULL;
	}

	/*
	 * Return it to the idle list.
	 */
	client_put(srvr, clnt);
}
