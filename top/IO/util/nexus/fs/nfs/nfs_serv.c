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
#include <stdlib.h>					/* for free() */
#include <string.h>

#include "cmn.h"
#include "creds.h"
#include "vnode.h"
#include "vfs.h"
#include "direct.h"
#include "nfssvc.h"

/*
 * NFS V2 server.
 */

IDENTIFY("$Id: nfs_serv.c,v 1.5.4.1.2.1 2002/03/19 19:09:36 rklundt Exp $");

#ifdef DEBUG
unsigned nfs_debug = 0;					/* debug level */

struct nfs_svcdbg_conf nfs_svcdbg_switch[] = {
	{
		"NFSPROC_NULL_2",
		NULL,
		NULL
	},
	{
		"NFSPROC_GETATTR_2",
		(nfs_dbgfun_t )nfs_dbgsvc_nfs_fh,
		(nfs_dbgfun_t )nfs_dbgsvc_attrstat,
	},
	{
		"NFSPROC_SETATTR_2",
		(nfs_dbgfun_t )nfs_dbgsvc_sattrargs,
		(nfs_dbgfun_t )nfs_dbgsvc_attrstat,
	},
	{
		"NFSPROC_ROOT_2",
		NULL,
		NULL
	},
	{
		"NFSPROC_LOOKUP_2",
		(nfs_dbgfun_t )nfs_dbgsvc_diropargs,
		(nfs_dbgfun_t )nfs_dbgsvc_diropres,
	},
	{
		"NFSPROC_READLINK_2",
		(nfs_dbgfun_t )nfs_dbgsvc_nfs_fh,
		(nfs_dbgfun_t )nfs_dbgsvc_readlinkres,
	},
	{
		"NFSPROC_READ_2",
		(nfs_dbgfun_t )nfs_dbgsvc_readargs,
		(nfs_dbgfun_t )nfs_dbgsvc_readres,
	},
	{
		"NFSPROC_WRITECACHE_2",
		NULL,
		NULL
	},
	{
		"NFSPROC_WRITE_2",
		(nfs_dbgfun_t )nfs_dbgsvc_writeargs,
		(nfs_dbgfun_t )nfs_dbgsvc_attrstat,
	},
	{
		"NFSPROC_CREATE_2",
		(nfs_dbgfun_t )nfs_dbgsvc_createargs,
		(nfs_dbgfun_t )nfs_dbgsvc_diropres,
	},
	{
		"NFSPROC_REMOVE_2",
		(nfs_dbgfun_t )nfs_dbgsvc_diropargs,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	},
	{
		"NFSPROC_RENAME_2",
		(nfs_dbgfun_t )nfs_dbgsvc_renameargs,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	},
	{
		"NFSPROC_LINK_2",
		(nfs_dbgfun_t )nfs_dbgsvc_linkargs,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	},
	{
		"NFSPROC_SYMLINK_2",
		(nfs_dbgfun_t )nfs_dbgsvc_symlinkargs,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	},
	{
		"NFSPROC_MKDIR_2",
		(nfs_dbgfun_t )nfs_dbgsvc_createargs,
		(nfs_dbgfun_t )nfs_dbgsvc_diropres,
	},
	{
		"NFSPROC_RMDIR_2",
		(nfs_dbgfun_t )nfs_dbgsvc_diropargs,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	},
	{
		"NFSPROC_READDIR_2",
		(nfs_dbgfun_t )nfs_dbgsvc_readdirargs,
		(nfs_dbgfun_t )nfs_dbgsvc_readdirres,
	},
	{
		"NFSPROC_STATFS_2",
		(nfs_dbgfun_t )nfs_dbgsvc_nfs_fh,
		(nfs_dbgfun_t )nfs_dbgsvc_statfsres,
	}
#ifdef ENFS
	,
	{
		"ENFSPROC_SYNCHRONIZE_1",
		(nfs_dbgfun_t )nfs_dbgsvc_nfs_fh,
		(nfs_dbgfun_t )nfs_dbgsvc_nfsstat,
	}
#endif
};

size_t nfs_svcdbg_switch_len =
    sizeof(nfs_svcdbg_switch) / sizeof(struct nfs_svcdbg_conf);
#endif

#ifdef INSTRUMENT
struct nfssrv_stats nfssrv_stats = {
	0,						/* encodes */
	0,						/* encodes_missed */
	0,						/* handle inserts */
	0,						/* handle collisions */
	0,						/* handle lookups */
	0						/* handle misses */
};
mutex_t nfssrv_stats_mutex = MUTEX_INITIALIZER;		/* stats mutex */
#endif

/*
 * Initialize
 */
void
nfs_initialize_server(void)
{

	nfs_service_init();
}

bool_t
nfsproc_null_2_svc(void *argp, void *result, struct svc_req *rqstp)
{

	NFS_SVC_ENTER(rqstp, argp, NULL);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_getattr_2_svc(nfs_fh *argp, attrstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(argp, &vp, RWLK_READ);
	if (!err) {
		err = nfs_getattrs(vp, &result->attrstat_u.attributes);
		v_put(vp);
	}
	result->status = nfs_errstat(err);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_setattr_2_svc(sattrargs *argp, attrstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->file, &vp, RWLK_WRITE);
	if (!err) {
		err = nfs_setattrs(vp, &argp->attributes, &cr);
		if (!err)
			err = nfs_getattrs(vp, &result->attrstat_u.attributes);
		v_put(vp);
	}
	result->status = nfs_errstat(err);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_root_2_svc(void *argp, void *result, struct svc_req *rqstp)
{

	NFS_SVC_ENTER(rqstp, argp, NULL);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_lookup_2_svc(diropargs *argp, diropres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *dvp, *vp;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->dir, &dvp, RWLK_READ);
	if (!err) {
		err = v_lookup(dvp, argp->name, &cr, RWLK_READ, &vp);
		v_rele(dvp);
	}
	if (!err) {
		err = nfs_getattrs(vp, &result->diropres_u.diropres.attributes);
		if (!err)
			nfs_mkfh(vp, &result->diropres_u.diropres.file);
		v_put(vp);
	}
	result->status = nfs_errstat(err);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_readlink_2_svc(nfs_fh *argp, readlinkres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;
	char	*bp;
	size_t	buflen;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = 0;
	buflen = NFS_MAXPATHLEN;
	bp = m_alloc(buflen);
	if (bp == NULL)
		err = ENOMEM;
	if (!err)
		err = nfs_getv(argp, &vp, RWLK_READ);
	if (!err) {
		err = VOP_READLINK(vp, bp, (u_int32_t *)&buflen);
		if (!err)
			result->readlinkres_u.data = bp;
		v_put(vp);
	}
	result->status = nfs_errstat(err);
	NFS_SVCDBG_PROC_EXIT(rqstp, result);
	if (!svc_sendreply(rqstp->rq_xprt, xdr_readlinkres, (char *)result))
		svcerr_systemerr(rqstp->rq_xprt);
	if (bp != NULL)
		free(bp);
	result->readlinkres_u.data = NULL;
	return FALSE;
}

bool_t
nfsproc_read_2_svc(readargs *argp, readres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;
	char	*bp;
	size_t	buflen;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = 0;
	buflen = min(NFS_MAXDATA, argp->count);
	bp = m_alloc(buflen);
	if (bp == NULL)
		err = ENOMEM;
	if (!err)
		err = nfs_getv(&argp->file, &vp, RWLK_NONE);
	if (!err) {
		err = VOP_READ(vp, argp->offset, bp, (u_int32_t *)&buflen, &cr);
		if (!err)
			err = VOP_LOCK(vp, RWLK_READ, 0);
		if (!err) {
			err =
			    nfs_getattrs(vp,
			    		 &result->readres_u.reply.attributes);
			VOP_UNLOCK(vp);
		}
		if (!err) {
			result->readres_u.reply.data.data_val = bp;
			result->readres_u.reply.data.data_len = buflen;
		}
		v_rele(vp);
	}
	result->status = nfs_errstat(err);
	NFS_SVCDBG_PROC_EXIT(rqstp, result);
	if (!svc_sendreply(rqstp->rq_xprt, xdr_readres, (char *)result))
		svcerr_systemerr(rqstp->rq_xprt);
	if (bp != NULL)
		free(bp);
	result->readres_u.reply.data.data_val = NULL;
	result->readres_u.reply.data.data_len = 0;
	return FALSE;
}

bool_t
nfsproc_writecache_2_svc(void *argp, void *result, struct svc_req *rqstp)
{

	NFS_SVC_ENTER(rqstp, argp, NULL);
	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_write_2_svc(writeargs *argp, attrstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;
	size_t	buflen;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->file, &vp, RWLK_NONE);
	if (!err) {
		buflen = argp->data.data_len;
		err =
		    VOP_WRITE(vp,
			      argp->offset,
			      argp->data.data_val,
			      (u_int32_t *)&buflen,
			      &cr);
		if (!err && buflen != argp->data.data_len) {
			LOG(LOG_ERR,
			    "%lu/%lu: short write (%Zu of %lu)",
			    vp->v_vfsp->vfs_dev,
			    vp->v_ino,
			    buflen,
			    argp->data.data_len);
			err = EIO;
		}
		if (!err)
			err = VOP_LOCK(vp, RWLK_READ, 0);
		if (!err) {
			err = nfs_getattrs(vp, &result->attrstat_u.attributes);
			VOP_UNLOCK(vp);
		}
		v_rele(vp);
	}
	result->status = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_create_2_svc(createargs *argp, diropres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *dvp, *vp;
	struct vstat vstbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->where.dir, &dvp, RWLK_WRITE);
	if (!err) {
		nfs_sattrxlate(&argp->attributes, &vstbuf);
		err =
		    VOP_CREATE(dvp, argp->where.name, &vstbuf,
		    	       &cr, RWLK_READ, &vp);
		v_put(dvp);
	}
	if (!err) {
		err = nfs_getattrs(vp, &result->diropres_u.diropres.attributes);
		if (!err)
			nfs_mkfh(vp, &result->diropres_u.diropres.file);
		v_put(vp);
	}
	result->status = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_remove_2_svc(diropargs *argp, nfsstat *result, struct svc_req *rqstp)
{
	int	err;
	struct creds cr;
	struct vnode *dvp, *vp;
	struct vlkop vlkops[2];
	struct vstat vstbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	dvp = NULL;
	err = nfs_getv(&argp->dir, &dvp, RWLK_READ);
	vp = NULL;
	if (!err)
		err = v_lookup(dvp, argp->name, &cr, RWLK_NONE, &vp);
	if (!err && v_cmp(dvp, vp) == 0)
		err = EISDIR;
	if (!err) {
		vlkops[0].vlkop_vp = dvp;
		vlkops[0].vlkop_lkf = RWLK_WRITE;
		vlkops[1].vlkop_vp = vp;
		vlkops[1].vlkop_lkf = RWLK_WRITE;
		err = v_multilock(vlkops, 2);
	}
	if (!err) {
		err = VOP_GETATTR(vp, &vstbuf);
		if (!err && VS_ISDIR(vstbuf.vst_mode))
			err = EISDIR;
		if (err) {
			VOP_UNLOCK(vp);
			VOP_UNLOCK(dvp);
		}
	}
	if (!err) {
		err = VOP_REMOVE(dvp, argp->name, vp, &cr);
		VOP_UNLOCK(dvp);
		if (!err)
			vp = NULL;
		else
			VOP_UNLOCK(vp);
	}
	if (vp != NULL)
		v_rele(vp);
	if (dvp != NULL)
		v_rele(dvp);
	*result = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_rename_2_svc(renameargs *argp, nfsstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *fdvp, *tdvp, *vp;
	int	samedir;
	struct vlkop vlkops[3];

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	fdvp = NULL;
	err = nfs_getv(&argp->from.dir, &fdvp, RWLK_READ);
	vp = NULL;
	if (!err)
		err = v_lookup(fdvp, argp->from.name, &cr, RWLK_NONE, &vp);
	tdvp = NULL;
	if (!err)
		err = nfs_getv(&argp->to.dir, &tdvp, RWLK_NONE);
	if (!err && fdvp->v_vfsp != tdvp->v_vfsp)
		err = EXDEV;
	if (!err) {
		samedir = v_cmp(fdvp, tdvp) == 0 ? 1 : 0;
		vlkops[0].vlkop_vp = fdvp;
		vlkops[0].vlkop_lkf = RWLK_WRITE;
		vlkops[1].vlkop_vp = vp;
		vlkops[1].vlkop_lkf = RWLK_WRITE;
		if (!samedir) {
			vlkops[2].vlkop_vp = tdvp;
			vlkops[2].vlkop_lkf = RWLK_WRITE;
		}
		err = v_multilock(vlkops, samedir ? 2 : 3);
		if (!err) {
			err =
			    VOP_RENAME(fdvp, argp->from.name, vp,
			    	       tdvp, argp->to.name, &cr);
			VOP_UNLOCK(vp);
			VOP_UNLOCK(tdvp);
			if (!samedir)
				VOP_UNLOCK(fdvp);
		}
	}
	if (vp != NULL)
		v_rele(vp);
	if (tdvp != NULL)
		v_rele(tdvp);
	if (fdvp != NULL)
		v_rele(fdvp);
	*result = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_link_2_svc(linkargs *argp, nfsstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp, *tdvp;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->from, &vp, RWLK_NONE);
	if (!err)
		err = nfs_getv(&argp->to.dir, &tdvp, RWLK_WRITE);
	if (!err && vp->v_vfsp != tdvp->v_vfsp)
		err = EXDEV;
	if (!err)
		err = VOP_LINK(vp, tdvp, argp->to.name, &cr);
	if (tdvp != NULL)
		v_put(tdvp);
	if (vp != NULL)
		v_rele(vp);
	*result = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_symlink_2_svc(symlinkargs *argp, nfsstat *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *fdvp;
	struct vstat vstbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->from.dir, &fdvp, RWLK_WRITE);
	if (!err) {
		nfs_sattrxlate(&argp->attributes, &vstbuf);
		err =
		    VOP_SYMLINK(fdvp, argp->from.name, argp->to, &vstbuf, &cr);
		v_put(fdvp);
	}
	*result = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_mkdir_2_svc(createargs *argp, diropres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *dvp, *vp;
	struct vstat vstbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	err = nfs_getv(&argp->where.dir, &dvp, RWLK_WRITE);
	if (!err) {
		nfs_sattrxlate(&argp->attributes, &vstbuf);
		err =
		    VOP_MKDIR(dvp, argp->where.name, &vstbuf,
		    	      &cr, RWLK_READ, &vp);
		v_put(dvp);
	}
	if (!err) {
		err = nfs_getattrs(vp, &result->diropres_u.diropres.attributes);
		if (!err)
			nfs_mkfh(vp, &result->diropres_u.diropres.file);
		v_put(vp);
	}
	result->status = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_rmdir_2_svc(diropargs *argp, nfsstat *result, struct svc_req *rqstp)
{
	int	err;
	struct vnode *dvp, *vp;
	struct creds cr;
	struct vlkop vlkops[2];
	struct vstat vstbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	dvp = NULL;
	err = nfs_getv(&argp->dir, &dvp, RWLK_READ);
	vp = NULL;
	if (!err)
		err = v_lookup(dvp, argp->name, &cr, RWLK_NONE, &vp);
	if (!err && v_cmp(dvp, vp) == 0)
		err = EBUSY;
	if (!err && v_cmp(vp, rootvp) == 0)
		err = EBUSY;
	if (!err) {
		vlkops[0].vlkop_vp = dvp;
		vlkops[0].vlkop_lkf = RWLK_WRITE;
		vlkops[1].vlkop_vp = vp;
		vlkops[1].vlkop_lkf = RWLK_WRITE;
		err = v_multilock(vlkops, 2);
	}
	if (!err) {
		err = VOP_GETATTR(vp, &vstbuf);
		if (!err && !VS_ISDIR(vstbuf.vst_mode))
			err = ENOTDIR;
		if (err) {
			VOP_UNLOCK(vp);
			VOP_UNLOCK(dvp);
		}
	}
	if (!err) {
		err = VOP_RMDIR(dvp, argp->name, vp, &cr);
		VOP_UNLOCK(dvp);
		if (!err)
			vp = NULL;
		else
			VOP_UNLOCK(vp);
	}
	if (vp != NULL)
		v_rele(vp);
	if (dvp != NULL)
		v_rele(dvp);
	*result = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

bool_t
nfsproc_readdir_2_svc(readdirargs *argp,
		      readdirres *result,
		      struct svc_req *rqstp)
{
	struct creds cr;
	struct vnode *dvp;
	u_int32_t ui32;
	z_off_t	off;
	int	err;
	size_t	buflen;
	char	*buf;
	int	eof;
	entry	*entries, *ep;
	struct directory_entry *dep;
	size_t	reclen;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	(void )memcpy(&ui32, argp->cookie, sizeof(ui32));
	off = (z_off_t )ui32;				/* beware sizeof here */
	if (argp->count > NFS_MAXDATA)
		argp->count = NFS_MAXDATA;
	buflen = argp->count;
	buf = m_alloc(2 * buflen);
	ep = entries = (entry *)(buf + buflen);
	err = buf == NULL ? ENOMEM : 0;
	dvp = NULL;
	if (!err)
		err = nfs_getv(&argp->dir, &dvp, RWLK_READ);
	eof = 0;
	if (!err)
		err = VOP_READDIR(dvp,
				  &off,
				  buf,
				  (u_int32_t *)&buflen,
				  &eof,
				  &cr);
	if (!err && buflen) {
		for (dep = (struct directory_entry *)buf;
		     ((reclen = directory_record_length(dep->de_namlen),
		       buflen >= reclen) &&
		      sizeof(entry) + dep->de_namlen + 1 < argp->count);
		     (buflen -= reclen,
		      argp->count -= sizeof(entry) + dep->de_namlen + 1,
		      dep =
		          (struct directory_entry *)((char *)dep + reclen))) {
			ep->fileid = dep->de_fileid;
			if (ep->fileid != dep->de_fileid) {
				err = EINVAL;
				break;
			}
			ep->name = dep->de_name;
			if (dep->de_namlen > NFS_MAXNAMLEN) {
				dep->de_namlen = NFS_MAXNAMLEN;
				ep->name[dep->de_namlen] = '\0';
			}
			(void )memset(ep->cookie, 0, sizeof(nfscookie));
			ui32 = (u_int32_t )dep->de_off;
			if (ui32 != dep->de_off) {
				err = EINVAL;
				break;
			}
			(void )memcpy(ep->cookie, &ui32, sizeof(ep->cookie));
			ep->nextentry = ep + 1;
			ep++;
		}
	}
	if (dvp != NULL)
		v_put(dvp);
	if (!err) {
		if (ep != entries) {
			result->readdirres_u.reply.entries = entries;
			ep--;
			ep->nextentry = NULL;
		} else
			result->readdirres_u.reply.entries = NULL;
		result->readdirres_u.reply.eof = 0;
		if (!buflen)
			result->readdirres_u.reply.eof = eof;
		assert(result->readdirres_u.reply.entries != NULL ||
		       result->readdirres_u.reply.eof);
	}
	result->status = nfs_errstat(err);
	NFS_SVCDBG_PROC_EXIT(rqstp, result);
	if (!svc_sendreply(rqstp->rq_xprt, xdr_readdirres, result))
		svcerr_systemerr(rqstp->rq_xprt);
	if (buf != NULL)
		m_free(buf);
	result->readdirres_u.reply.entries = NULL;
	return FALSE;
}

bool_t
nfsproc_statfs_2_svc(nfs_fh *argp, statfsres *result, struct svc_req *rqstp)
{
	struct creds cr;
	int	err;
	struct vnode *vp;
	struct vfs *vfsp;
	struct fsstats fsstatbuf;

	NFS_SVC_ENTER(rqstp, argp, &cr);
	(void )memset(result, 0, sizeof(*result));
	vfsp = NULL;
	err = nfs_getv(argp, &vp, RWLK_NONE);
	if (!err) {
		vfsp = vp->v_vfsp;
		err = vfs_get(vfsp, RWLK_READ, 0);
		v_rele(vp);
	}
	if (!err) {
		err = VFSOP_STATFS(vfsp, &fsstatbuf);
		vfs_put(vfsp);
	}
	if (!err) {
		result->statfsres_u.reply.tsize = fsstatbuf.fsstats_tsize;
		result->statfsres_u.reply.bsize = fsstatbuf.fsstats_bsize;
		result->statfsres_u.reply.blocks = fsstatbuf.fsstats_blocks;
		result->statfsres_u.reply.bfree = fsstatbuf.fsstats_bfree;
		result->statfsres_u.reply.bavail = fsstatbuf.fsstats_bavail;
	}
	result->status = nfs_errstat(err);

	NFS_SVC_RETURN(TRUE, rqstp, result);
}

int
nfs_program_2_freeresult(SVCXPRT *transp IS_UNUSED,
			 xdrproc_t xdr_result,
			 caddr_t result)
{
	(void) xdr_free(xdr_result, result);

	return 1;
}
