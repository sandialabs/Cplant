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

#include "nfsfs.h"

IDENTIFY("$Id: nfs_vops.c,v 1.4.8.1 2002/03/19 19:09:36 rklundt Exp $");

/*
 * NFS file system vnode operations.
 */

static int
nfs_vop_getattr(struct vnode *vp, struct vstat *vstp)
{
	struct nfsino *nfsip = V2NFS(vp);
	int	err;
	fattr	fa;

	err = nfs_igetattr(nfsip, &fa);
	if (err)
		return err;
	nfs_attrxlate(&fa, vp->v_vfsp->vfs_dev, vp->v_ino, vstp);
	return err;
}

static int
nfs_vop_setattr(struct vnode *vp, struct vstat *vstp, struct creds *crp)
{
	struct nfsino *nfsip = V2NFS(vp);
	int	err;
	sattrargs args;
	attrstat result;

	assert(VOP_LKTY(vp) == RWLK_WRITE);

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.file = nfsip->nfsi_fh;
	err = nfs_sattrload(vstp, &args.attributes);
	if (err)
		return err;

	err =
	    nfs_call(VFS2NFS(vp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_setattr_2,
		     &args,
		     &result,
		     crp);
	if (!err && result.status != NFS_OK)
		err = nfs_errxlate(result.status);
	if (err)
		INVALIDATE_ATTRCACHE(nfsip);
	else
		nfs_cacheattrs(nfsip, &result.attrstat_u.attributes);

	xdr_free(xdr_attrstat, &result);

	return err;
}

static int
nfs_vop_lookup(struct vnode *dvp,
	       const char *name,
	       struct creds *crp,
	       rwty_t lkf,
	       struct vnode **vpp)
{
	struct nfsino *nfsip, *nfsdip = V2NFS(dvp);
	struct nfsfs *nfsfsp;
	diropargs args;
	diropres result;
	int 	err;

	nfsfsp = VFS2NFS(nfsdip->nfsi_vp->v_vfsp);
	VOP_UNLOCK(dvp);				/* don't need this */

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.dir = nfsdip->nfsi_fh;
	if (strlen(name) >= NFS_MAXNAMLEN)
		return ENAMETOOLONG;
	args.name = (char *)name;

	err =
	    nfs_call(nfsfsp,
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_lookup_2,
		     &args,
		     &result,
		     crp);
	if (!err && result.status != NFS_OK)
		err = nfs_errxlate(result.status);
	if (err) {
		INVALIDATE_ATTRCACHE(nfsdip);		/* recheck the dir */
		xdr_free(xdr_diropres, &result);
		return err;
	}

	err =
	    nfs_findi(nfsfsp,
		      NFS_MKPSI(&result.diropres_u.diropres.file,
		      		result.diropres_u.diropres.attributes.fileid),
		      &result.diropres_u.diropres.file,
		      &result.diropres_u.diropres.attributes,
		      lkf,
		      &nfsip);
	if (!err)
		*vpp = nfsip->nfsi_vp;

	xdr_free(xdr_diropres, &result);

	return err;
}

static int
nfs_vop_readlink(struct vnode *vp, char *buf, u_int32_t *buflenp)
{
	struct nfsino *nfsip = V2NFS(vp);
	readlinkres result;
	int 	err;
	char	*scp, *dcp;
	u_int32_t cc;

	(void )memset(&result, 0, sizeof(result));

	err =
	    nfs_call(VFS2NFS(vp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_readlink_2,
		     &nfsip->nfsi_fh,
		     &result,
		     &suser_creds);
	cc = 0;
	if (!err && result.status != NFS_OK)
		err = nfs_errxlate(result.status);
	else {
		scp = result.readlinkres_u.data;
		dcp = buf;
		do {
			*dcp++ = *scp;
		} while (++cc < NFS_MAXPATHLEN &&
			 cc < *buflenp &&
			 *scp++ != '\0');
		if (cc >= NFS_MAXPATHLEN || cc >= *buflenp)
			err = ENAMETOOLONG;
	}
	if (err)
		INVALIDATE_ATTRCACHE(nfsip);
	else
		*buflenp = cc;
	xdr_free(xdr_readlinkres, &result);
	return err;
}

static int
nfs_vop_read(struct vnode *vp,
	     z_off_t offset,
	     char *buf,
	     u_int32_t *buflenp,
	     struct creds *crp)
{
	struct nfsino *nfsip = V2NFS(vp);
	int	err;
	readargs args;
	readres	result;
	u_int32_t remain;
	char	*bp;

#ifndef NDEBUG
	assert(!v_islocked(vp));
#endif

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	/*
	 * NFS V2 is only 32-bit addressable.
	 */
	if (offset >= 4294967296) {
		INVALIDATE_ATTRCACHE(nfsip);		/* old attrs? */
		*buflenp = 0;
		return 0;
	}
	if (4294967296 - (u_int32_t )offset < *buflenp)
		*buflenp = 4294967296 - (u_int32_t )offset;

	args.file = nfsip->nfsi_fh;
	remain = (u_int32_t )*buflenp;
	bp = buf;
	do {
		args.offset = (u_int )offset;
		args.count = remain;
		if (args.count > NFS_MAXDATA)
			args.count= NFS_MAXDATA;
		args.totalcount = args.count;

		result.readres_u.reply.data.data_val = bp;
	    	err =
		    nfs_call(VFS2NFS(nfsip->nfsi_vp->v_vfsp),
		    	     (bool_t (*)(void *,
					 void *,
					 CLIENT *))nfsproc_read_2,
			     &args,
			     &result,
			     crp);
		if (!err && result.status != NFS_OK)
			err = nfs_errxlate(result.status);
		if (err)
			break;
		nfs_cacheattrs(nfsip, &result.readres_u.reply.attributes);

		offset += result.readres_u.reply.data.data_len;
		remain -= result.readres_u.reply.data.data_len;
		bp += result.readres_u.reply.data.data_len;
	} while (remain && result.readres_u.reply.data.data_len == args.count);

	if (err)
		INVALIDATE_ATTRCACHE(nfsip);
	else
		*buflenp = bp - buf;

	/*
	 * No xdr_free() needed this time. We were very careful to use only
	 * local or derived space.
	 */
	return err;
}

static int
nfs_vop_write(struct vnode *vp,
	      z_off_t offset,
	      char *buf,
	      u_int32_t *buflenp,
	      struct creds *crp)
{
	struct nfsino *nfsip = V2NFS(vp);
	int	err;
	writeargs args;
	attrstat result;
	u_int32_t remain;
	char	*bp;

#ifndef NDEBUG
	assert(!v_islocked(vp));
#endif

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	/*
	 * NFS V2 is only 32-bit addressable.
	 */
	if (offset >= 4294967296) {
		INVALIDATE_ATTRCACHE(nfsip);		/* old attrs? */
		*buflenp = 0;
		return 0;
	}
	if (4294967296 - (u_int32_t )offset < *buflenp)
		*buflenp = 4294967296 - (u_int32_t )offset;

	args.file = nfsip->nfsi_fh;
	remain = (u_int32_t )*buflenp;
	bp = buf;
	do {
		args.offset = args.beginoffset = (u_int )offset;
		args.totalcount = remain;
		if (args.totalcount > NFS_MAXDATA)
			args.totalcount= NFS_MAXDATA;
		args.data.data_len = args.totalcount;
		args.data.data_val = bp;

	    	err =
		    nfs_call(VFS2NFS(nfsip->nfsi_vp->v_vfsp),
		    	     (bool_t (*)(void *,
					 void *,
					 CLIENT *))nfsproc_write_2,
			     &args,
			     &result,
			     crp);
		if (!err && result.status != NFS_OK)
			err = nfs_errxlate(result.status);
		if (err)
			break;
		nfs_cacheattrs(nfsip, &result.attrstat_u.attributes);

		offset += args.data.data_len;
		remain -= args.data.data_len;
		bp += args.data.data_len;
	} while (remain);

	if (err)
		INVALIDATE_ATTRCACHE(nfsip);
	else
		*buflenp = bp - buf;

	/*
	 * No xdr_free() needed this time. We were very careful to use only
	 * local or derived space.
	 */
	return err;
}

/*
 * Common directory/file creation routine.
 */
static int
do_create(struct vnode *dvp,
	  const char *name,
	  struct vstat *vstp,
	  struct creds *crp,
	  rwty_t lkf,
	  struct vnode **vpp,
	  bool_t (*f)(void *, void *, CLIENT *))
{
	struct nfsino *nfsdip = V2NFS(dvp);
	struct nfsfs *nfsfsp = VFS2NFS(dvp->v_vfsp);
	int	err;
	createargs args;
	diropres result;
	struct nfsino *nfsip;

	assert(VOP_LKTY(dvp) == RWLK_WRITE);

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.where.dir = nfsdip->nfsi_fh;
	if (strlen(name) >= NFS_MAXNAMLEN)
		return ENAMETOOLONG;
	args.where.name = (char *)name;
	err = nfs_sattrload(vstp, &args.attributes);
	if (err)
		return err;

	err =
	    nfs_call(nfsfsp,
	    	     f,
		     &args,
		     &result,
		     crp);
	if (!err && result.status != NFS_OK)
		err = nfs_errxlate(result.status);
	/*
	 * Always invalidate after a create. On success the directory
	 * has been updated. On failure, we want a refresh anyway.
	 */
	INVALIDATE_ATTRCACHE(nfsdip);
	if (err) {
		xdr_free(xdr_diropres, &result);
		return err;
	}

	err =
	    nfs_findi(nfsfsp,
		      NFS_MKPSI(&result.diropres_u.diropres.file,
		      		result.diropres_u.diropres.attributes.fileid),
		      &result.diropres_u.diropres.file,
		      &result.diropres_u.diropres.attributes,
		      lkf,
		      &nfsip);
	if (!err)
		*vpp = nfsip->nfsi_vp;
	xdr_free(xdr_diropres, &result);
	return err;
}

static int
nfs_vop_create(struct vnode *dvp,
	       const char *name,
	       struct vstat *vstp,
	       struct creds *crp,
	       rwty_t lkf,
	       struct vnode **vpp)
{

	return do_create(dvp,
			 name,
			 vstp,
			 crp,
			 lkf,
			 vpp, 
			 (bool_t (*)(void *,
				     void *,
				     CLIENT *))nfsproc_create_2);
}

/*
 * Common file/directory remove operation.
 */
static int
do_remove(struct vnode *dvp,
	  const char *name,
	  struct vnode *vp,
	  struct creds *crp,
	  bool_t (*f)(void *, void *, CLIENT *))
{
	struct nfsino *nfsdip = V2NFS(dvp);
	int	err;
	diropargs args;
	nfsstat	result;

	assert(VOP_LKTY(dvp) == RWLK_WRITE && VOP_LKTY(vp) == RWLK_WRITE);

	(void )memset(&args, 0, sizeof(args));

	args.dir = nfsdip->nfsi_fh;
	args.name = (char *)name;			/* trust the caller */

	err =
	    nfs_call(VFS2NFS(dvp->v_vfsp),
	    	     f,
		     &args,
		     &result,
		     crp);
	if (!err && result != NFS_OK)
		err = nfs_errxlate(result);
	xdr_free(xdr_nfsstat, &result);

	/*
	 * Always invalidate after a remove. On success the directory
	 * has been updated. On failure, we want a refresh anyway.
	 */
	INVALIDATE_ATTRCACHE(nfsdip);

	/*
	 * We always want this vnode eliminated. It's just safer that way, as
	 * we could have failed due to an idempotency problem. If they
	 * want to try again we can always recreate the handle.
	 *
	 * To v_gone this now can get us into a nasty situation
	 * where another reference causes us to wait. Since the
	 * parent directory is write locked, this can be far
	 * reaching. Instead, just put it and let it be reclaimed.
	 *
	 * It would be nice to give back the vnode now though...
	 */
	INVALIDATE_ATTRCACHE(V2NFS(vp));
	if (!err)
		v_gone(vp);

	return err;
}

static int
nfs_vop_remove(struct vnode *dvp,
	       const char *name,
	       struct vnode *vp,
	       struct creds *crp)
{

	return do_remove(dvp,
			 name,
			 vp,
			 crp,
			 (bool_t (*)(void *,
				     void *,
				     CLIENT *))nfsproc_remove_2);
}

static int
nfs_vop_rename(struct vnode *fdvp,
	       const char *fname,
	       struct vnode *vp,
	       struct vnode *tdvp,
	       const char *tname,
	       struct creds *crp)
{
	struct nfsino *nfsfdip = V2NFS(fdvp);
	struct nfsino *nfstdip = V2NFS(tdvp);
	int	err;
	renameargs args;
	nfsstat	result;

	assert(VOP_LKTY(fdvp) == RWLK_WRITE &&
	       VOP_LKTY(tdvp) == RWLK_WRITE &&
	       v_islocked(vp));

	if (fdvp->v_vfsp != tdvp->v_vfsp)
		return EXDEV;

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.from.dir = nfsfdip->nfsi_fh;
	args.from.name = (char *)fname;
	args.to.dir = nfstdip->nfsi_fh;
	args.to.name = (char *)tname;

	err =
	    nfs_call(VFS2NFS(vp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_rename_2,
		     &args,
		     &result,
		     crp);
	if (!err && result != NFS_OK)
		err = nfs_errxlate(result);
	if (err || v_cmp(fdvp, tdvp) == 0) {
		INVALIDATE_ATTRCACHE(nfsfdip);
		INVALIDATE_ATTRCACHE(nfstdip);
	}

	xdr_free(xdr_nfsstat, &result);
	return err;
}

static int
nfs_vop_link(struct vnode *vp,
	     struct vnode *tdvp,
	     const char *name,
	     struct creds *crp)
{
	struct nfsino *nfsip = V2NFS(vp);
	struct nfsino *nfstdip = V2NFS(tdvp);
	int	err;
	linkargs args;
	nfsstat	result;

	assert(VOP_LKTY(tdvp) == RWLK_WRITE);

	if (vp->v_vfsp != tdvp->v_vfsp)
		return EXDEV;

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.from = nfsip->nfsi_fh;
	args.to.dir = nfstdip->nfsi_fh;
	args.to.name = (char *)name;

	err =
	    nfs_call(VFS2NFS(vp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_link_2,
		     &args,
		     &result,
		     crp);
	if (!err && result != NFS_OK)
		err = nfs_errxlate(result);
	/*
	 * On success, both the parent and the inode have new attributes. The
	 * ones we're holding need a refresh.
	 *
	 * On error, we want a refresh, as usual. Invalidate them always then.
	 */
	INVALIDATE_ATTRCACHE(nfsip);
	INVALIDATE_ATTRCACHE(nfstdip);

	xdr_free(xdr_nfsstat, &result);
	return err;
}

static int
nfs_vop_symlink(struct vnode *dvp,
		const char *name,
		const char *path,
		struct vstat *vstp,
		struct creds *crp)
{
	struct nfsino *nfsdip = V2NFS(dvp);
	int	err;
	symlinkargs args;
	nfsstat	result;

	assert(VOP_LKTY(dvp) == RWLK_WRITE);

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.from.dir = nfsdip->nfsi_fh;
	args.from.name = (char *)name;
	if (strlen(path) > NFS_MAXPATHLEN)
		return ENAMETOOLONG;
	args.to = (char *)path;
	err = nfs_sattrload(vstp, &args.attributes);
	if (err)
		return err;

	err =
	    nfs_call(VFS2NFS(dvp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_symlink_2,
		     &args,
		     &result,
		     crp);
	if (!err && result != NFS_OK)
		err = nfs_errxlate(result);
	/*
	 * Always invalidate after a symlink. On success the directory
	 * has been updated. On failure, we want a refresh anyway.
	 */
	INVALIDATE_ATTRCACHE(nfsdip);

	/*
	 * The client will have to do a lookup. Let's not go to the
	 * trouble of generating the vnode now.
	 */

	xdr_free(xdr_nfsstat, &result);
	return err;
}

static int
nfs_vop_mkdir(struct vnode *dvp,
	      const char *name,
	      struct vstat *vstp,
	      struct creds *crp,
	      rwty_t lkf,
	      struct vnode **vpp)
{

	return do_create(dvp,
			 name,
			 vstp,
			 crp,
			 lkf,
			 vpp, 
			 (bool_t (*)(void *, void *, CLIENT *))nfsproc_mkdir_2);
}

static int
nfs_vop_rmdir(struct vnode *dvp,
	      const char *name,
	      struct vnode *vp,
	      struct creds *crp)
{

	return do_remove(dvp,
			 name,
			 vp,
			 crp,
			 (bool_t (*)(void *,
				     void *,
				     CLIENT *))nfsproc_rmdir_2);
}

static int
nfs_vop_readdir(struct vnode *dvp,
		z_off_t *offp,
		char *buf,
		u_int32_t *bufsiz,
		int	*eofp,
		struct creds *crp)
{
	struct nfsino *dip = V2NFS(dvp);
	int	err;
	readdirargs args;
	u_int32_t ui32;
	readdirres result;
	size_t	cc;
	struct directory_entry *dep;
	entry	*ep;
	size_t	len;
	size_t	reclen;

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.dir = dip->nfsi_fh;
	(void )memset(args.cookie, 0, sizeof(nfscookie));
	ui32 = (unsigned long )*offp;
	if (ui32 != *offp)
		return EINVAL;
	(void )memcpy(args.cookie, &ui32, sizeof(args.cookie));
	args.count = min(*bufsiz, NFS_MAXDATA);
	err =
	    nfs_call(VFS2NFS(dvp->v_vfsp),
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_readdir_2,
		     &args,
		     &result,
		     crp);
	if (!err && result.status != NFS_OK)
		err = nfs_errxlate(result.status);
	if (err) {
		INVALIDATE_ATTRCACHE(dip);
		xdr_free(xdr_readdirres, &result);
		return err;
	}
	cc = 0;
	dep = (struct directory_entry *)buf;
	for (ep = result.readdirres_u.reply.entries;
	     ep != NULL;
	     ep = ep->nextentry) {
		len = strlen(ep->name);
		if (len > NFS_MAXNAMLEN)
			len = NFS_MAXNAMLEN;
		reclen = directory_record_length(len);
		if (cc + reclen >= *bufsiz)
			break;
		dep->de_fileid = ep->fileid;
		if (dep->de_fileid != ep->fileid) {
			err = EINVAL;
			break;
		}
		(void )memcpy(&ui32, ep->cookie, sizeof(ui32));
		dep->de_off = (z_off_t )ui32;
		if (dep->de_off != ui32) {
			err = EINVAL;
			break;
		}
		dep->de_namlen = len;
		(void )strncpy(dep->de_name, ep->name, NFS_MAXNAMLEN);
		dep->de_name[len] = '\0';

		cc += reclen;
		dep = (struct directory_entry *)((char *)dep + reclen);
	}
	xdr_free(xdr_readdirres, &result);
	if (!err) {
		*eofp = FALSE;
		if (ep == NULL) {
			*eofp = result.readdirres_u.reply.eof;
		} else if (!cc)
			err = EINVAL;
		if (!err)
			*bufsiz = cc;
	}

	return err;
}

static int
nfs_vop_handle(struct vnode *vp, struct vnhndl *vnhndlp)
{
	struct nfsino *nfsip = V2NFS(vp);

	vnhndlp->vnhnd_len = NFS_FHSIZE;
#ifndef ENHANCE_NFS_HANDLE_COMPRESSION
	vnhndlp->vnhnd_data = nfsip->nfsi_fh.data;
#else
	vnhndlp->vnhnd_data = nfsip->nfsi_vfh.data;
#endif

	return 0;
}

#define nfs_vop_lock \
	v_generic_op_lock

#define nfs_vop_unlock \
	v_generic_op_unlock

#define nfs_vop_lkty \
	v_generic_op_lkty

static void
nfs_vop_reclaim(struct vnode *vp)
{
	struct nfsino *nfsip = V2NFS(vp);

	nfs_reclaimi(nfsip);
}

struct vops nfs_vops = {
	nfs_vop_getattr,
	nfs_vop_setattr,
	nfs_vop_lookup,
	nfs_vop_readlink,
	nfs_vop_read,
	nfs_vop_write,
	nfs_vop_create,
	nfs_vop_remove,
	nfs_vop_rename,
	nfs_vop_link,
	nfs_vop_symlink,
	nfs_vop_mkdir,
	nfs_vop_rmdir,
	nfs_vop_readdir,
	nfs_vop_handle,
	nfs_vop_lock,
	nfs_vop_unlock,
	nfs_vop_lkty,
	nfs_vop_reclaim
};
