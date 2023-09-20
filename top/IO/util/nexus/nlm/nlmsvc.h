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
#include "rpcsvc/nlm_prot.h"

#ifdef DEBUG
#define NLM_SVCDBG_CHECK(lvl) \
	NFS_SVCDBG_CHECK(lvl)

#define NLM_SVCDBG_PROC_EXIT(rqstp, result) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= nlm_svcdbg_switch_len \
		      ? NULL \
		      : &nlm_svcdbg_switch[(rqstp)->rq_proc]; \
		LOG_DBG(NLM_SVCDBG_CHECK(1), \
			"NLM1 EXIT %s:", \
			svcdbg_confp == NULL \
			  ? "<bad proc num>" \
			  : svcdbg_confp->name); \
		if (NFS_SVCDBG_CHECK(1) && svcdbg_confp->atexit != NULL) \
			(*svcdbg_confp->atexit)((result), " "); \
	} while (0)

#define NLM_SVCDBG_PROC_ENTER(rqstp, argp, crp) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= nfs_svcdbg_switch_len \
		      ? NULL \
		      : &nlm_svcdbg_switch[(rqstp)->rq_proc]; \
		if (NLM_SVCDBG_CHECK(1)) { \
			LOG_DBG(NLM_SVCDBG_CHECK(1), \
				"NLM1 ENTRY %s:", \
				svcdbg_confp == NULL \
				  ? "<bad proc num>" \
				  : svcdbg_confp->name); \
			if ((crp) != NULL) \
				dbg_creds((crp), " "); \
		} \
		if (NFS_SVCDBG_CHECK(1) && svcdbg_confp->atentry != NULL) \
			(*svcdbg_confp->atentry)((argp), " "); \
	} while (0)
#else /* is !defined(DEBUG) */
#define NLM_SVCDBG_CHECK(lvl) \
	((int )0)
#define NLM_SVCDBG_PROC_EXIT(rqstp, result)
#define NLM_SVCDBG_PROC_ENTER(rqstp, argp, crp)
#endif /* defined(DEBUG) */

#define NLM_SVC_RETURN(rtn, rqstp, result) \
	do { \
		NLM_SVCDBG_PROC_EXIT((rqstp), (result)); \
		return (rtn); \
	} while (0)

#define NLM_SVC_ENTER(rqstp, argp, crp) \
	do { \
		if ((crp) != NULL && !nfs_getcreds((rqstp), (crp))) { \
			svcerr_auth((rqstp)->rq_xprt, AUTH_TOOWEAK); \
			NLM_SVC_RETURN(FALSE, (rqstp), NULL); \
		} \
		NLM_SVCDBG_PROC_ENTER((rqstp), (argp), (crp)); \
	} while (0)

struct creds;

#ifdef DEBUG
extern size_t nlm_svcdbg_switch_len;
extern struct nfs_svcdbg_conf nlm_svcdbg_switch[];
#endif

extern void nlm_initialize_server(void);
extern void nlm_proxy_server(const char *, u_long, u_long, const char *);

#ifdef DEBUG
#endif
