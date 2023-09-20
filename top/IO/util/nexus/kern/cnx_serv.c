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

#include "cnxsvc.h"

#include "vfs.h"
#include "vnode.h"

IDENTIFY("$Id");

#ifdef DEBUG
static struct cnx_svcdbg_conf cnx_svcdbg_switch[] = {
	{
		"<NO_SERVICE>",
		NULL,
		NULL
	},
	{
		"CNXPROC_MOUNT_1",
		(cnx_dbgfun_t )cnx_dbgsvc_mountarg,
		(cnx_dbgfun_t )cnx_dbgsvc_status,
	},
	{
		"CNXPROC_UNMOUNT_1",
		(cnx_dbgfun_t )cnx_dbgsvc_path,
		(cnx_dbgfun_t )cnx_dbgsvc_status,
	},
	{
		"CNXPROC_OFFER_1",
		(cnx_dbgfun_t )cnx_dbgsvc_offerarg,
		(cnx_dbgfun_t )cnx_dbgsvc_svcidres,
	},
	{
		"CNXPROC_STOP_1",
		(cnx_dbgfun_t )cnx_dbgsvc_svcid,
		(cnx_dbgfun_t )cnx_dbgsvc_status,
	}
};
static size_t cnx_svcdbg_switch_len =
    sizeof(cnx_svcdbg_switch) / sizeof(struct cnx_svcdbg_conf);

unsigned cnx_debug = 0;
#endif

void
cnx_initialize_server()
{
}

bool_t
cnxproc_mount_1_svc(cnx_mountarg *argp,
		    cnx_status *result,
		    struct svc_req *rqstp)
{
	struct creds cr;
	int	err;

	CNX_SVC_ENTER(rqstp, argp, &cr);
	err = 0;
	if (argp->path == NULL ||
	    strlen(argp->path) == 0 ||
	    (strlen(argp->path) == 1 && *argp->path == '/'))
		err =
		    mount_root(argp->type,
			       argp->arg,
			       sizeof(argp->arg),
			       &cr);
	else
		err =
		    mount_fs(argp->type,
			     argp->path,
			     argp->arg,
			     sizeof(argp->arg),
			     &cr);
	*result = cnx_errstat(err);
	CNX_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
cnxproc_unmount_1_svc(cnx_path *argp,
		      cnx_status *result,
		      struct svc_req *rqstp)
{
	struct creds cr;
	struct namei_data nd;
	int	err;
	struct vfs *vfsp;

	CNX_SVC_ENTER(rqstp, argp, &cr);

	/*
	 * Lookup the path given. From that, we'll determine the
	 * FS of interest.
	 */
	nd.nd_flags = 0;
	nd.nd_parent = rootvp;
	nd.nd_path = *argp;
	nd.nd_vp = NULL;
	nd.nd_lkf = RWLK_NONE;
	nd.nd_crp = &cr;
	err = namei(&nd);

	vfsp = NULL;
	if (!err) {
		vfsp = nd.nd_vp->v_vfsp;
		err = vfs_get(vfsp, RWLK_WRITE, 0);
		v_rele(nd.nd_vp);
	}
	if (!err) {
		err = vfs_umount(vfsp, &cr);
		if (err)
			vfs_put(vfsp);
	}
	*result = cnx_errstat(err);

	CNX_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
cnxproc_offer_1_svc(cnx_offerarg *argp,
		    cnx_svcidres *result,
		    struct svc_req *rqstp)
{
	struct creds cr;
	int	err;

	CNX_SVC_ENTER(rqstp, argp, &cr);

	(void )memset(result, 0, sizeof(cnx_svcidres));
	err = cnx_startsvc(argp->type, argp->arg, sizeof(argp->arg), &cr);
	result->status = cnx_errstat(err);
	if (result->status == CNX_OK)
		result->cnx_svcidres_u.id = 1;

	CNX_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
cnxproc_stop_1_svc(cnx_svcid *argp,
		   cnx_status *result,
		   struct svc_req *rqstp)
{

	CNX_SVC_ENTER(rqstp, argp, NULL);
	*result = cnx_errstat(ENOSYS);
	CNX_SVC_RETURN(TRUE, rqstp, result);
}

int
cnx_program_1_freeresult(SVCXPRT *transp IS_UNUSED,
			 xdrproc_t xdr_result,
			 caddr_t result)
{
	(void) xdr_free(xdr_result, result);

	return 1;
}
