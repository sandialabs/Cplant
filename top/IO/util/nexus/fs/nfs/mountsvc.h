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
 * $Id: mountsvc.h,v 1.2 2000/02/15 23:25:56 lward Exp $
 */

#include "rpcsvc/mount.h"

#ifdef DEBUG
#define MOUNT_SVCDBG_CHECK(lvl) \
	NFS_SVCDBG_CHECK(lvl)

#define MOUNT_SVCDBG_PROC_EXIT(rqstp, result) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= mount_svcdbg_switch_len \
		      ? NULL \
		      : &mount_svcdbg_switch[(rqstp)->rq_proc]; \
		LOG_DBG(MOUNT_SVCDBG_CHECK(1), \
			"MOUNT1 EXIT %s:", \
			svcdbg_confp == NULL \
			  ? "<bad proc num>" \
			  : svcdbg_confp->name); \
		if (NFS_SVCDBG_CHECK(1) && \
		    (result) != NULL && \
		    svcdbg_confp->atexit != NULL) \
			(*svcdbg_confp->atexit)((result), " "); \
	} while (0)

#define MOUNT_SVCDBG_PROC_ENTER(rqstp, argp, crp) \
	do { \
		struct nfs_svcdbg_conf *svcdbg_confp; \
 \
		svcdbg_confp = \
		    (rqstp)->rq_proc >= nfs_svcdbg_switch_len \
		      ? NULL \
		      : &mount_svcdbg_switch[(rqstp)->rq_proc]; \
		if (MOUNT_SVCDBG_CHECK(1)) { \
			LOG_DBG(MOUNT_SVCDBG_CHECK(1), \
				"MOUNT1 ENTRY %lu.%lu %s:", \
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
#define MOUNT_SVCDBG_CHECK(lvl) \
	((int )0)
#define MOUNT_SVCDBG_PROC_EXIT(rqstp, result)
#define MOUNT_SVCDBG_PROC_ENTER(rqstp, argp, crp)
#endif /* defined(DEBUG) */

#define MOUNT_SVC_RETURN(rtn, rqstp, result) \
	do { \
		MOUNT_SVCDBG_PROC_EXIT((rqstp), (result)); \
		return (rtn); \
	} while (0)

#define MOUNT_SVC_ENTER(rqstp, argp, crp) \
	do { \
		if ((crp) != NULL && !xlate_rpccreds((rqstp), (crp))) { \
			svcerr_auth((rqstp)->rq_xprt, AUTH_TOOWEAK); \
			MOUNT_SVC_RETURN(FALSE, (rqstp), NULL); \
		} \
		MOUNT_SVCDBG_PROC_ENTER((rqstp), (argp), (crp)); \
	} while (0)

struct creds;

#ifdef DEBUG
extern size_t mount_svcdbg_switch_len;
extern struct nfs_svcdbg_conf mount_svcdbg_switch[];
#endif

extern void mount_initialize_server(void);
extern void mount_proxy_server(const char *, u_long, u_long, const char *);
extern bool_t handle_mnt_1(dirpath *, fhstatus *, struct creds *);
extern bool_t handle_dump_1(void *, mountlist *, struct creds *);
extern bool_t handle_umnt_1(dirpath *, void *, struct creds *);
extern bool_t handle_umntall_1(void *, void *, struct creds *);
extern bool_t handle_export_1(void *, exports *, struct creds *);
extern bool_t handle_exportall_1(void *, exports *, struct creds *);

#ifdef DEBUG
void mount_dbgsvc_mountlist(mountlist *, char *);
void mount_dbgsvc_mountbody(mountbody *, char *);
void mount_dbgsvc_groups(groups *, char *);
void mount_dbgsvc_groupnode(groupnode *, char *);
void mount_dbgsvc_exports(exports *, char *);
void mount_dbgsvc_exportnode(exportnode *, char *);
void mount_dbgsvc_fhandle(fhandle *, char *);
void mount_dbgsvc_fhstatus(fhstatus *, char *);
void mount_dbgsvc_dirpath(dirpath *, char *);
#endif
