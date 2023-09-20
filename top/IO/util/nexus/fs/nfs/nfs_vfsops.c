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
 * NFS client side file system.
 *
 * $Id: nfs_vfsops.c,v 1.5 2001/07/18 18:57:31 rklundt Exp $
 */
#include <stdlib.h>
#include <string.h>

#include "nfsfs.h"
#include "nfsmnt.h"

IDENTIFY("$Id: nfs_vfsops.c,v 1.5 2001/07/18 18:57:31 rklundt Exp $");

/*
 * NFS file system operations.
 */

/*
 * Destroy an nfsfs record.
 */
static void
nfs_destroy_nfsfs(struct nfsfs *nfsfsp)
{

	if (nfsfsp->nfsfs_host != NULL)
		free((void *)nfsfsp->nfsfs_host);
	if (nfsfsp->nfsfs_path != NULL)
		free((void *)nfsfsp->nfsfs_path);
	if (nfsfsp->nfsfs_server != NULL)
		client_context_destroy(nfsfsp->nfsfs_server);
	free(nfsfsp);
}

/*
 * Make a filesystem ID using the file handle given.
 *
 * The idea is that a group of symmetric servers will all calculate the
 * same identifier. This will be used in the construction of the file handles
 * they will issue later. Since they are symmetric, handles are common.
 */
static z_dev_t
nfs_mkfsid(nfs_fh *fhp)
{
	unsigned i;
	z_dev_t	dev;
	unsigned char *srcp, *dstp;

	/*
	 * This implementation is very naive.
	 */
	dev = 0;
	srcp = (unsigned char *)fhp;
	dstp = (unsigned char *)&dev;
	for (i = 0; i < sizeof(nfs_fh); i++) {
		*dstp++ ^= *srcp++;
		if (srcp >= (unsigned char *)fhp + sizeof(*fhp))
			srcp = (unsigned char *)fhp;
		if (dstp >= (unsigned char *)&dev + sizeof(dev))
			dstp = (unsigned char *)&dev;
	}

	return dev;
}

/*
 * Mount the indicated remote file system.
 */
int
nfs_mount(const char *args,
	  size_t argslen,
	  struct creds *crp,
	  struct vfs **vfspp)
{
	nfsmnt	*nfsmntp;
	XDR	xdrs;
	struct nfsfs *nfsfsp;
	int	err;
	union {
		fhstatus fhstatus;
		attrstat attrstat;
	} result;
	struct timeval tv;
	extern struct vfsops nfs_vfsops;

	/*
	 * Decode the arguments.
	 */
	nfsmntp = m_alloc(sizeof(nfsmnt));
	if (nfsmntp == NULL)
		return ENOMEM;
	(void )memset(nfsmntp, 0, sizeof(nfsmnt));
	(void )memset(&xdrs, 0, sizeof(xdrs));
	xdrmem_create(&xdrs, args, argslen, XDR_DECODE);
	err = 0;
	if (!xdr_nfsmnt(&xdrs, nfsmntp)) {
		LOG(LOG_ERR, "nfs_mount: can't decode FS specific args");
		err = EINVAL;
	}

	nfsfsp = NULL;
	if (!err) {
		/*
		 * Allocate VFS private data.
		 */
		nfsfsp = m_alloc(sizeof(struct nfsfs));
		if (nfsfsp != NULL) {
			(void )memset(nfsfsp, 0, sizeof(struct nfsfs));
			nfsfsp->nfsfs_host = NULL;
			nfsfsp->nfsfs_path = NULL;
			nfsfsp->nfsfs_server = NULL;
			nfsfsp->nfsfs_attrtimeo = 0;
		} else
			err = ENOMEM;
	}

	if (!err) {
		nfsfsp->nfsfs_host = m_alloc(strlen(nfsmntp->rhost) + 1);
		if (nfsfsp->nfsfs_host == NULL)
			err = ENOMEM;
		else
			(void )strcpy((char *)nfsfsp->nfsfs_host,
				      nfsmntp->rhost);
	}
	if (!err) {
		nfsfsp->nfsfs_path = m_alloc(strlen(nfsmntp->rpath) + 1);
		if (nfsfsp->nfsfs_path == NULL)
			err = ENOMEM;
		else
			(void )strcpy((char *)nfsfsp->nfsfs_path,
				      nfsmntp->rpath);
	}
	if (!err)
		nfsfsp->nfsfs_attrtimeo = (time_t )nfsmntp->attrtimeo;
	if (!err) {
		/*
		 * Set call timeout.
		 */
		nfsfsp->nfsfs_rpctimo.tv_sec = NFS_CALL_TIMEOUT;
		if ((time_t )nfsmntp->rpctimeo != 0)
			nfsfsp->nfsfs_rpctimo.tv_sec =
			  (time_t )nfsmntp->rpctimeo;
		nfsfsp->nfsfs_rpctimo.tv_usec = 0;
	}

	xdr_free(xdr_nfsmnt, nfsmntp);
	free(nfsmntp);

	if (err) {
		if (nfsfsp != NULL)
			nfs_destroy_nfsfs(nfsfsp);
		return err;
	}

	/*
	 * Issue the NFS mount call to the remote.
	 * We give it 30 seconds and then give up.
	 */
	tv.tv_sec = 180;
	tv.tv_usec = 0;
	(void )memset(&result.fhstatus, 0, sizeof(result.fhstatus));
	err =
	    nfs_simple_call(nfsfsp->nfsfs_host,
			    MOUNTPROG, MOUNTVERS, "udp",
			    (bool_t (*)(void *,
					void *,
					CLIENT *))mountproc_mnt_1,
			    (void **)&nfsfsp->nfsfs_path, &result,
			    &tv,
			    crp);
	if (!err)
		err = nfs_errxlate(result.fhstatus.fhs_status);

	(void )memcpy(&nfsfsp->nfsfs_rootfh,
		      &result.fhstatus.fhstatus_u.fhs_fhandle,
		      FHSIZE);
	xdr_free(xdr_fhstatus, &result.fhstatus);

	/*
	 * Create client context for this server.
	 */
	if (!err) {
		nfsfsp->nfsfs_server =
		    client_context_create(nfsfsp->nfsfs_host,
					  NFS_PROGRAM, NFS_VERSION,
					  "udp", 0);
		if (nfsfsp->nfsfs_server == NULL) {
			LOG(LOG_ERR,
			    "can't create client context for server %s",
			    nfsfsp->nfsfs_host);
			err = ENXIO;
		}
	}

	/*
	 * Create the VFS.
	 */
	if (!err) {
		nfsfsp->nfsfs_vfsp =
		    vfs_new(nfs_mkfsid(&nfsfsp->nfsfs_rootfh),
			    &nfs_vfsops,
			    nfsfsp);
		if (nfsfsp->nfsfs_vfsp == NULL)
			err = ENOMEM;
	}

	if (!err)
		*vfspp = nfsfsp->nfsfs_vfsp;
	else
		nfs_destroy_nfsfs(nfsfsp);

	return err;
}

static void
nfs_vfsop_unmount(struct vfs *vfsp)
{
	struct nfsfs *nfsfsp = VFS2NFS(vfsp);
	struct timeval tv;
	char	*path;
	int	err;

#ifdef DEBUG
	assert(!vfs_islocked(vfsp));
#endif

	/*
	 * Give the server 30 seconds to accomplish the unmount. Then,
	 * unmount anyway.
	 */
	tv.tv_sec = 30;
	tv.tv_usec = 0;

	path = (char *)nfsfsp->nfsfs_path;
	err =
	    nfs_simple_call(nfsfsp->nfsfs_host,
			    MOUNTPROG, MOUNTVERS, "udp",
			    (bool_t (*)(void *,
			    		void *,
					CLIENT *))mountproc_umnt_1,
			    &path, NULL,
			    &tv,
			    &suser_creds);
	if (err) {
		LOG(LOG_ERR,
		    "nfs_vfsop_unmount: can't get remote to unmount %s: %s",
		    nfsfsp->nfsfs_host,
		    nfsfsp->nfsfs_path);
		err = 0;
	}

	nfs_destroy_nfsfs(nfsfsp);
}

/*
 * Regenerate NFS inode from it's file handle.
 */
static int
regenerate(struct vfs *vfsp, nfs_fh *fhp, struct vnode **vpp, rwty_t lkf)
{
	int	err;
	struct nfsfs *nfsfsp = VFS2NFS(vfsp);
	attrstat result;
	struct nfsino *nfsip;

	(void )memset(&result, 0, sizeof(result));
	err =
	    nfs_call(nfsfsp,
	   	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_getattr_2,
		     fhp,
		     &result,
		     &suser_creds);
	if (!err && result.status != RPC_SUCCESS)
		err = nfs_errxlate(result.status);
	if (!err)
		err =
		    nfs_findi(nfsfsp,
			      NFS_MKPSI(fhp,
					result.attrstat_u.attributes.fileid),
			      fhp,
			      &result.attrstat_u.attributes,
			      lkf,
			      &nfsip);
	if (!err)
		*vpp = nfsip->nfsi_vp;
	xdr_free(xdr_attrstat, &result);
	return err;
}

static int
nfs_vfsop_getroot(struct vfs *vfsp, rwty_t lkf, struct vnode **vpp)
{

	/*
	 * You know... It would be so much nicer if we didn't
	 * have to appeal to the authority every time. Maybe we could
	 * cache the root vnode? After all, it's always referenced for
	 * any path that reaches this routine.
	 */

	return regenerate(vfsp, &VFS2NFS(vfsp)->nfsfs_rootfh, vpp, lkf);
}

static int
nfs_vfsop_statfs(struct vfs *vfsp, struct fsstats *fsstatbuf)
{
	struct nfsfs *nfsfsp = VFS2NFS(vfsp);
	int	err;
	statfsres result;

	(void )memset(&result, 0, sizeof(statfsres));
	err =
	    nfs_call(nfsfsp,
	    	     (bool_t (*)(void *, void *, CLIENT *))nfsproc_statfs_2,
		     &nfsfsp->nfsfs_rootfh, &result,
		     &suser_creds);
	if (!err)
		err = nfs_errxlate(result.status);
	if (!err) {
		fsstatbuf->fsstats_tsize = result.statfsres_u.reply.tsize;
		fsstatbuf->fsstats_bsize = result.statfsres_u.reply.bsize;
		fsstatbuf->fsstats_blocks = result.statfsres_u.reply.blocks;
		fsstatbuf->fsstats_bfree = result.statfsres_u.reply.bfree;
		fsstatbuf->fsstats_bavail = result.statfsres_u.reply.bavail;
	}

	xdr_free(xdr_statfsres, &result);
	return err;
}

static int
nfs_vfsop_getv(struct vfs *vfsp, z_ino_t inum, struct vnode **vpp, unsigned lkf)
{
	struct nfsfs *nfsfsp = VFS2NFS(vfsp);
	int	err;
	struct nfsino *nfsip;

	err = nfs_findi(nfsfsp, inum, NULL, NULL, lkf, &nfsip);
	if (!err)
		*vpp = nfsip->nfsi_vp;

	return err;
}

static int
nfs_vfsop_getv_by_handle(struct vfs *vfsp,
			 struct vnhndl *vnhndlp,
			 struct vnode **vpp,
			 rwty_t lkf)
{
#ifdef ENHANCE_NFS_HANDLE_COMPRESSION
	nfs_fh	realfh;
#else
#define	realfh *(nfs_fh *)vhndlp->vhnd_data
#endif

	if (vnhndlp->vnhnd_len != NFS_FHSIZE)
		return EINVAL;

#ifdef ENHANCE_NFS_HANDLE_COMPRESSION
	nfs_vfhtofh(VFS2NFS(vfsp), (nfs_fh *)vnhndlp->vnhnd_data, &realfh);
#endif
	return regenerate(vfsp, &realfh, vpp, lkf);
}

/*
 * Virtual file system operations.
 */
static struct vfsops nfs_vfsops = {
	nfs_vfsop_unmount,
	nfs_vfsop_getroot,
	nfs_vfsop_statfs,
	nfs_vfsop_getv,
	nfs_vfsop_getv_by_handle,
	vfs_generic_op_lock,
	vfs_generic_op_unlock,
	vfs_generic_op_lkty
};
