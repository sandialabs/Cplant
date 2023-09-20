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
#include "vfs.h"
#include "vnode.h"
#include "mountsvc.h"
#include "nfssvc.h"

IDENTIFY("$Id: mount_serv.c,v 1.1.10.1 2002/05/30 16:34:38 rklundt Exp $");

#ifdef DEBUG
struct nfs_svcdbg_conf mount_svcdbg_switch[] = {
        {
		"MOUNTPROC_NULL_1",
		NULL,
		NULL,
        },
        {
		"MOUNTPROC_MNT_1",
		(nfs_dbgfun_t )mount_dbgsvc_dirpath,
		(nfs_dbgfun_t )mount_dbgsvc_fhstatus,
        },
        {
		"MOUNTPROC_DUMP_1",
		NULL,
		(nfs_dbgfun_t )mount_dbgsvc_mountlist,
	},
        {
		"MOUNTPROC_UMNT_1",
		(nfs_dbgfun_t )mount_dbgsvc_dirpath,
		NULL
        },
	{
		"MOUNTPROC_UMNTALL_1",
		NULL,
		NULL
	},
	{
		"MOUNTPROC_EXPORT_1",
		NULL,
		(nfs_dbgfun_t )mount_dbgsvc_exports,
        },
	{
		"MOUNTPROC_EXPORTALL_1",
		NULL,
		(nfs_dbgfun_t )mount_dbgsvc_exports,
	}
};
size_t mount_svcdbg_switch_len =
    sizeof(mount_svcdbg_switch) / sizeof(struct nfs_svcdbg_conf);
#endif

void
mount_initialize_server(void)
{
}

bool_t
mountproc_null_1_svc(void *argp, void *result, struct svc_req *rqstp)
{

	MOUNT_SVC_ENTER(rqstp, argp, NULL);
	MOUNT_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
mountproc_mnt_1_svc(dirpath *argp, fhstatus *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct namei_data nd;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = rootvp ? v_get(rootvp, RWLK_NONE, 0) : EACCES;
	nd.nd_flags = 0;
	nd.nd_parent = rootvp;
	nd.nd_path = (const char *)*argp;
	nd.nd_vp = NULL;
	nd.nd_lkf = RWLK_READ;
	nd.nd_crp = &cr;
	if (!err)
		err = namei(&nd);
	v_rele(rootvp);
	if (!err) {
		nfs_mkfh(nd.nd_vp, (nfs_fh *)&result->fhstatus_u.fhs_fhandle);
		v_put(nd.nd_vp);
	}
	result->fhs_status = nfs_errstat(err);
	MOUNT_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
mountproc_dump_1_svc(void *argp, mountlist *result, struct svc_req *rqstp)
{
	struct creds cr;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	/*
	 * Not yet implemented.
	 */
	*result = NULL;
	MOUNT_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
mountproc_umnt_1_svc(dirpath *argp, void *result, struct svc_req *rqstp)
{
	struct creds cr;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	/*
	 * Not yet implemented.
	 */
	MOUNT_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
mountproc_umntall_1_svc(void *argp, void *result, struct svc_req *rqstp)
{
	struct creds cr;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	/*
	 * Not yet implemented.
	 */
	MOUNT_SVC_RETURN(TRUE, rqstp, result);
}

/*
 * We really only export `/' as we assemble our own name space. Do we want
 * per-host "views"? Almost certainly, we'll want to restrict access. Someday,
 * but not right now...
 */
static exportnode exports_list = {
	"/",
	NULL,
	NULL
};

bool_t
mountproc_export_1_svc(void *argp, exports *result, struct svc_req *rqstp)
{
	struct creds cr;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	/*
	 * Not yet implemented.
	 */
	*result = &exports_list;
	MOUNT_SVCDBG_PROC_EXIT(rqstp, result);
        if (!svc_sendreply(rqstp->rq_xprt, xdr_exports, (char *)result))
                svcerr_systemerr(rqstp->rq_xprt);
	*result = NULL;
	return FALSE;
}

bool_t
mountproc_exportall_1_svc(void *argp, exports *result, struct svc_req *rqstp)
{
	struct creds cr;

	MOUNT_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	/*
	 * Not yet implemented.
	 */
	*result = &exports_list;
	MOUNT_SVCDBG_PROC_EXIT(rqstp, result);
        if (!svc_sendreply(rqstp->rq_xprt, xdr_exports, (char *)result))
                svcerr_systemerr(rqstp->rq_xprt);
	*result = NULL;
	return FALSE;
}

int
mountprog_1_freeresult(SVCXPRT *transp IS_UNUSED,
		       xdrproc_t xdr_result,
		       caddr_t result)
{
	(void) xdr_free(xdr_result, result);

	return 1;
}
