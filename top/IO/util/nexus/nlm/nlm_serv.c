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
#include <string.h>

#include "cmn.h"
#include "creds.h"
#include "nfs/nlmsvc.h"
#include "nfs/nfssvc.h"
#include "nfs/client.h"
#include "nlmsvc.h"

/*
 * Forward a nlm call.
 */
#define FWD_NLMCALL(f, argp, result, crp) \
	fwd_call(nlm_remote_server, (rpc_gfun_t )(f), (argp), (result), \
		 &nlm_rtrytimo, (crp))

static struct server *nlm_remote_server = NULL;		/* remote info */

#ifdef DEBUG
static const char unknown[] = "<UNKNOWN NLM PROC>";

struct nfs_svcdbg_conf nlm_svcdbg_switch[] = {
	{
		"NLM_TEST",
		(nfs_dbgfun_t )nlm_dbgsvc_testargs,
		(nfs_dbgfun_t )nlm_dbgsvc_testres
	},
	{
		"NLM_LOCK",
		(nfs_dbgfun_t )nlm_dbgsvc_lockargs,
		(nfs_dbgfun_t )nlm_dbgsvc_res
	},
	{
		"NLM_CANCEL",
		(nfs_dbgfun_t )nlm_dbgsvc_cancargs,
		(nfs_dbgfun_t )nlm_dbgsvc_res
	},
	{
		"NLM_UNLOCK",
		(nfs_dbgfun_t )nlm_dbgsvc_unlockargs,
		(nfs_dbgfun_t )nlm_dbgsvc_res
	},
	{
		"NLM_GRANTED",
		(nfs_dbgfun_t )nlm_dbgsvc_testargs,
		(nfs_dbgfun_t )nlm_dbgsvc_res
	},
	{
		"NLM_TEST_MSG",
		(nfs_dbgfun_t )nlm_dbgsvc_testargs,
		NULL
	},
	{
		"NLM_LOCK_MSG",
		(nfs_dbgfun_t )nlm_dbgsvc_lockargs,
		NULL
	},
	{
		"NLM_CANCEL_MSG",
		(nfs_dbgfun_t )nlm_dbgsvc_cancargs,
		NULL
	},
	{
		"NLM_UNLOCK_MSG",
		(nfs_dbgfun_t )nlm_dbgsvc_unlockargs,
		NULL
	},
	{
		"NLM_GRANTED_MSG",
		(nfs_dbgfun_t )nlm_dbgsvc_testargs,
		NULL
	},
	{
		"NLM_TEST_RES",
		(nfs_dbgfun_t )nlm_dbgsvc_testres,
		NULL
	},
	{
		"NLM_LOCK_RES",
		(nfs_dbgfun_t )nlm_dbgsvc_res,
		NULL
	},
	{
		"NLM_CANCEL_RES",
		(nfs_dbgfun_t )nlm_dbgsvc_res,
		NULL
	},
	{
		"NLM_UNLOCK_RES",
		(nfs_dbgfun_t )nlm_dbgsvc_res,
		NULL
	},
	{
		"NLM_GRANTED_RES",
		(nfs_dbgfun_t )nlm_dbgsvc_res,
		NULL
	},
	{
		unknown,					/* proc 16 */
		NULL,
		NULL
	},
	{
		unknown,					/* proc 17 */
		NULL,
		NULL
	},
	{
		unknown,					/* proc 18 */
		NULL,
		NULL
	},
	{
		unknown,					/* proc 19 */
		NULL,
		NULL
	},
	{
		"NLM3_SHARE",
		(nfs_dbgfun_t )nlm_dbgsvc_shareargs,
		(nfs_dbgfun_t )nlm_dbgsvc_shareres
	},
	{
		"NLM3_UNSHARE",
		(nfs_dbgfun_t )nlm_dbgsvc_shareargs,
		(nfs_dbgfun_t )nlm_dbgsvc_shareres
	},
	{
		"NLM3_NM_LOCK",
		(nfs_dbgfun_t )nlm_dbgsvc_lockargs,
		(nfs_dbgfun_t )nlm_dbgsvc_res
	},
	{
		"NLM3_FREE_ALL",
		(nfs_dbgfun_t )nlm_dbgsvc_notify
		NULL
	}
};
size_t nlm_svcdbg_switch_len =
    sizeof(nlm_svcdbg_switch) / sizeof(struct nfs_svcdbg_conf);
#endif

static struct timeval nlm_rtrytimo = { 1, 0 };

void
nlm_initialize_server(void)
{
}

void
nlm_proxy_server(const char *host,
		 u_long prog,
		 u_long vers,
		 const char *protocol)
{
	if (nlm_remote_server != NULL)
		panic("nlm_proxy_server: already registered");

	nlm_remote_server = client_context_create(host, prog, vers, protocol);
	if (nlm_remote_server == NULL)
		panic("nlm_proxy_server: can't create client context");
}

bool_t
nlm_test_1_svc(struct nlm_testargs *argp,
	       nlm_testres *result,
	       struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	result->cookie.n_len = 0;
	result->cookie.n_bytes = NULL;
	result->stat.stat = nlm_denied;
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_lock_1_svc(struct nlm_lockargs *argp,
	       nlm_res *result,
	       struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	result->cookie.n_len = 0;
	result->cookie.n_bytes = NULL;
	result->stat.stat = nlm_denied_nolocks;
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_cancel_1_svc(struct nlm_cancargs *argp,
		 nlm_res *result,
		 struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	result->cookie.n_len = 0;
	result->cookie.n_bytes = NULL;
	result->stat.stat = nlm_denied_nolocks;
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_unlock_1_svc(struct nlm_unlockargs *argp,
		 nlm_res *result,
		 struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	result->cookie.n_len = 0;
	result->cookie.n_bytes = NULL;
	result->stat.stat = nlm_denied_nolocks;
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_granted_1_svc(struct nlm_testargs *argp,
		  nlm_res *result,
		  struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	result->cookie.n_len = 0;
	result->cookie.n_bytes = NULL;
	result->stat.stat = nlm_denied_nolocks;
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_test_msg_1_svc(struct nlm_testargs *argp,
		   void *result,
		   struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_lock_msg_1_svc(struct nlm_lockargs *argp,
		   void *result,
		   struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_cancel_msg_1_svc(struct nlm_cancargs *argp,
		     void *result,
		     struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_unlock_msg_1_svc(struct nlm_unlockargs *argp,
		     void *result,
		     struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_granted_msg_1_svc(struct nlm_testargs *argp,
		      void *result,
		      struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_test_res_1_svc(nlm_testres *argp,
		   void *result,
		   struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_lock_res_1_svc(nlm_res *argp,
		   void *result,
		   struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_cancel_res_1_svc(nlm_res *argp,
		     void *result,
		     struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_unlock_res_1_svc(nlm_res *argp,
		     void *result,
		     struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_granted_res_1_svc(nlm_res *argp,
		      void *result,
		      struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

int
nlm_prog_1_freeresult(SVCXPRT *transp,
		      xdrproc_t xdr_result,
		      caddr_t result)
{

	(void) xdr_free(xdr_result, result);

	return 1;
}

bool_t
nlm_share_3_svc(nlm_shareargs *argp,
		nlm_shareres *result,
		struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	LOG(LOG_WARNING, "nlm_share_3_svc: call not supported");
	NLM_SVC_RETURN(FALSE, rqstp, result);
}

bool_t
nlm_unshare_3_svc(nlm_shareargs *argp,
		  nlm_shareres *result,
		  struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	LOG(LOG_WARNING, "nlm_unshare_3_svc: call not supported");
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_nm_lock_3_svc(nlm_lockargs *argp,
		  nlm_res *result,
		  struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	LOG(LOG_WARNING, "nlm_nm_lock_3_svc: call not supported");
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nlm_free_all_3_svc(nlm_notify *argp,
		   void *result,
		   struct svc_req *rqstp)
{
	struct creds cr;

	NLM_SVC_ENTER(rqstp, argp, &cr);
	LOG(LOG_WARNING, "nlm_free_all_3_svc: call not supported");
	NLM_SVC_RETURN(TRUE, rqstp, result);
}

int
nlm_prog_3_freeresult(SVCXPRT *transp,
		      xdrproc_t xdr_result,
		      caddr_t result)
{

	(void) xdr_free(xdr_result, result);

	return 1;
}
