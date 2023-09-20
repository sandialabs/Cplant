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
#include "rpcsvc/cnx_prot.h"
#include "cmn.h"
#include "rwlk.h"

/*
 * CNX service support.
 *
 * $Id: cnxsvc.h,v 1.1 1999/11/29 19:49:29 lward Exp $
 */

struct creds;

/*
 * Format of a CONNEX service table entry.
 */
struct cnxconf {
	const char *cnxconf_name;			/* name */
	int	(*cnxconf_startsvc)(const void *,
				    size_t,
				    struct creds *);	/* start func */
};

typedef void (*cnx_dbgfun_t)(void *, const char *);

struct cnx_svcdbg_conf {
	const char *name;				/* proc name */
	cnx_dbgfun_t atentry;				/* entry debug_func */
	cnx_dbgfun_t atexit;				/* exit debug_func */
};

typedef bool_t (*rpc_gfun_t)(void *, void *, CLIENT *);

#ifdef DEBUG
#define CNX_SVCDBG_CHECK(lvl) \
	(cnx_debug >= (lvl))

#define CNX_SVCDBG_PROC_EXIT(rqstp, result) \
	do { \
		struct cnx_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= cnx_svcdbg_switch_len \
		      ? NULL \
		      : &cnx_svcdbg_switch[(rqstp)->rq_proc]; \
		LOG_DBG(CNX_SVCDBG_CHECK(1), \
			"CNX EXIT %s:", \
			svcdbg_confp == NULL \
			  ? "<bad proc num>" \
			  : svcdbg_confp->name); \
		if (CNX_SVCDBG_CHECK(1) && \
		    (result) != NULL && \
		    svcdbg_confp->atexit != NULL) \
			(*svcdbg_confp->atexit)((result), " "); \
	} while (0)

#define CNX_SVCDBG_PROC_ENTER(rqstp, argp, crp) \
	do { \
		struct cnx_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= cnx_svcdbg_switch_len \
		      ? NULL \
		      : &cnx_svcdbg_switch[(rqstp)->rq_proc]; \
		if (CNX_SVCDBG_CHECK(1)) { \
			LOG_DBG(CNX_SVCDBG_CHECK(1), \
				"CNX ENTRY %s:", \
				svcdbg_confp == NULL \
				  ? "<bad proc num>" \
				  : svcdbg_confp->name); \
			if ((crp) != NULL) \
				dbg_creds((crp), " "); \
		} \
		if (CNX_SVCDBG_CHECK(1) && svcdbg_confp->atentry != NULL) \
			(*svcdbg_confp->atentry)((argp), " "); \
	} while (0)
#else /* is !defined(DEBUG) */
#define CNX_SVCDBG_CHECK(lvl) \
	((int )0)
#define CNX_SVCDBG_PROC_EXIT(rqstp, result)
#define CNX_SVCDBG_PROC_ENTER(rqstp, argp, crp)
#endif /* defined(DEBUG) */

#define CNX_SVC_RETURN(rtn, rqstp, result) \
	do { \
		CNX_SVCDBG_PROC_EXIT((rqstp), (result)); \
		return (rtn); \
	} while (0)

#define CNX_SVC_ENTER(rqstp, argp, crp) \
	do { \
		if ((crp) != NULL && !xlate_rpccreds((rqstp), (crp))) { \
			svcerr_auth((rqstp)->rq_xprt, AUTH_TOOWEAK); \
			CNX_SVC_RETURN(FALSE, (rqstp), NULL); \
		} \
		CNX_SVCDBG_PROC_ENTER((rqstp), (argp), (crp)); \
	} while (0)

#ifdef DEBUG
extern unsigned cnx_debug;

extern size_t cnx_svcdbg_switch_len;
extern struct cnx_svcdbg_conf cnx_svcdbg_switch[];
#endif

extern struct cnxconf cnxconftbl[];

extern void cnx_initialize_server(void);
extern void cnx_service_init(void);

extern cnx_status cnx_errstat(int);
extern int cnx_startsvc(const char *, void *, size_t, struct creds *);

#ifdef DEBUG
extern void cnx_dbgsvc_mountarg(cnx_mountarg *, const char *);
extern void cnx_dbgsvc_status(cnx_status *, const char *);
extern void cnx_dbgsvc_path(cnx_path *, const char *);
extern void cnx_dbgsvc_offerarg(cnx_offerarg *, const char *);
extern void cnx_dbgsvc_svcidres(cnx_svcidres *, const char *);
extern void cnx_dbgsvc_svcid(cnx_svcid *, const char *);
#endif
