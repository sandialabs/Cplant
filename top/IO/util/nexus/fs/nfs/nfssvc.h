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
#include "rpcsvc/nfs_prot.h"
#include "rwlk.h"

/*
 * NFS V2 service support.
 *
 * $Id: nfssvc.h,v 1.3 2000/03/15 00:08:07 lward Exp $
 */

/*
 * Some systems (Linux 2.2.14 at least) don't like it when they see the same
 * file identifier twice for different files. They really should be maintaining
 * their internal tables by the file handle. However, in order to *try* to
 * accomodate, we make all FS identifiers the following and the file identifier
 * becomes the real FS id + the file id.
 */
#define AGGREGATE_IDENTIFIER	0xfadedace

typedef void (*nfs_dbgfun_t)(void *, const char *);

struct nfs_svcdbg_conf {
	const char *name;				/* proc name */
	nfs_dbgfun_t atentry;				/* entry debug_func */
	nfs_dbgfun_t atexit;				/* exit debug_func */
};

typedef bool_t (*rpc_gfun_t)(void *, void *, CLIENT *);

#ifdef DEBUG
#define NFS_SVCDBG_CHECK(lvl) \
	(nfs_debug >= (lvl))

#define NFS_SVCDBG_PROC_EXIT(rqstp, result) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= nfs_svcdbg_switch_len \
		      ? NULL \
		      : &nfs_svcdbg_switch[(rqstp)->rq_proc]; \
		LOG_DBG(NFS_SVCDBG_CHECK(1), \
			"NFS2 EXIT %s:", \
			svcdbg_confp == NULL \
			  ? "<bad proc num>" \
			  : svcdbg_confp->name); \
		if (NFS_SVCDBG_CHECK(1) && \
		    (result) != NULL && \
		    svcdbg_confp->atexit != NULL) \
			(*svcdbg_confp->atexit)((result), " "); \
	} while (0)

#define NFS_SVCDBG_PROC_ENTER(rqstp, argp, crp) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= nfs_svcdbg_switch_len \
		      ? NULL \
		      : &nfs_svcdbg_switch[(rqstp)->rq_proc]; \
		if (NFS_SVCDBG_CHECK(1)) { \
			LOG_DBG(NFS_SVCDBG_CHECK(1), \
				"NFS2 ENTRY %lu.%lu %s:", \
				(rqstp)->rq_prog, \
				(rqstp)->rq_vers, \
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
#define NFS_SVCDBG_CHECK(lvl) \
	((int )0)
#define NFS_SVCDBG_PROC_EXIT(rqstp, result)
#define NFS_SVCDBG_PROC_ENTER(rqstp, argp, crp)
#endif /* defined(DEBUG) */

#define NFS_SVC_RETURN(rtn, rqstp, result) \
	do { \
		NFS_SVCDBG_PROC_EXIT((rqstp), (result)); \
		return (rtn); \
	} while (0)

#define NFS_SVC_ENTER(rqstp, argp, crp) \
	do { \
		if ((crp) != NULL && !xlate_rpccreds((rqstp), (crp))) { \
			svcerr_auth((rqstp)->rq_xprt, AUTH_TOOWEAK); \
			NFS_SVC_RETURN(FALSE, (rqstp), NULL); \
		} \
		NFS_SVCDBG_PROC_ENTER((rqstp), (argp), (crp)); \
	} while (0)

#ifdef DEBUG
extern unsigned nfs_debug;

extern size_t nfs_svcdbg_switch_len;
extern struct nfs_svcdbg_conf nfs_svcdbg_switch[];
#endif

#ifdef INSTRUMENT
/*
 * Statistics about the NFS server.
 */
struct nfssrv_stats {
	unsigned long encodes;
	unsigned long encodes_missed;
	unsigned long handle_inserts;
	unsigned long handle_collisions;
	unsigned long handle_lookups;
	unsigned long handle_misses;
};
extern struct nfssrv_stats nfssrv_stats;
extern mutex_t nfssrv_stats_mutex;
#endif

struct creds;
struct server;

struct vnode;
struct vstat;

extern void nfs_initialize_server(void);
extern void nfs_service_init(void);
#if 0
extern void nfs_proxy_server(const char *, u_long, u_long, const char *);
#endif

#if 0
extern bool_t fwd_call(struct server *,
		       bool_t (*)(void *, void *, CLIENT *),
		       void *, void *,
		       struct timeval *, struct creds *, struct svc_req *);
#endif
extern int nfs_getv(nfs_fh *, struct vnode **, rwty_t);
extern int nfs_getattrs(struct vnode *, fattr *);
extern int nfs_setattrs(struct vnode *, sattr *, struct creds *);
extern nfsstat nfs_errstat(int);
extern void nfs_mkfh(struct vnode *, nfs_fh *);
extern void nfs_sattrxlate(sattr *, struct vstat *);

#ifdef DEBUG
extern void nfs_dbgsvc_nfsstat(nfsstat *, char *);
extern void nfs_dbgsvc_nfs_fh(nfs_fh *, char *);
extern void nfs_dbgsvc_nfstime(nfstime *, char *);
extern void nfs_dbgsvc_fattr(fattr *, char *);
extern void nfs_dbgsvc_sattr(sattr *, char *);
extern void nfs_dbgsvc_attrstat(attrstat *, char *);
extern void nfs_dbgsvc_sattrargs(sattrargs *, char *);
extern void nfs_dbgsvc_diropargs(diropargs *, char *);
extern void nfs_dbgsvc_diropokres(diropokres *, char *);
extern void nfs_dbgsvc_diropres(diropres *, char *);
extern void nfs_dbgsvc_readlinkres(readlinkres *, char *);
extern void nfs_dbgsvc_readargs(readargs *, char *);
extern void nfs_dbgsvc_readokres(readokres *, char *);
extern void nfs_dbgsvc_readres(readres *, char *);
extern void nfs_dbgsvc_writeargs(writeargs *, char *);
extern void nfs_dbgsvc_createargs(createargs *, char *);
extern void nfs_dbgsvc_renameargs(renameargs *, char *);
extern void nfs_dbgsvc_linkargs(linkargs *, char *);
extern void nfs_dbgsvc_symlinkargs(symlinkargs *, char *);
extern void nfs_dbgsvc_nfscookie(nfscookie *, char *);
extern void nfs_dbgsvc_readdirargs(readdirargs *, char *);
extern void nfs_dbgsvc_entry(entry *, char *);
extern void nfs_dbgsvc_dirlist(dirlist *, char *);
extern void nfs_dbgsvc_readdirres(readdirres *, char *);
extern void nfs_dbgsvc_statfsokres(statfsokres *, char *);
extern void nfs_dbgsvc_statfsres(statfsres *, char *);
#endif
