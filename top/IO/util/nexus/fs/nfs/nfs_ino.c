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
#include <stdlib.h>
#include <string.h>

#include "nfsfs.h"

/*
 * NFS inode manipulation.
 */

IDENTIFY("$Id: nfs_ino.c,v 1.4 2001/07/18 18:57:31 rklundt Exp $");

/*
 * Find a vnode, given fs and i-number.
 */
int
nfs_findi(struct nfsfs *nfsfsp,
	  z_ino_t ino,
	  nfs_fh *fhp,
	  fattr	*fattrp,
	  rwty_t lkf,
	  struct nfsino **nfsipp)
{
	int	ismylock;
	int	err;
	struct vnode *vp;
	struct nfsino *nfsip;

	/*
	 * We'll need at least a read lock to examine the vnode found.
	 * If the caller isn't requesting a lock, we get one anyway and
	 * drop it before the return.
	 */
	ismylock = 0;
	if (lkf == RWLK_NONE) {
		ismylock = 1;
		lkf = RWLK_READ;
	}

	vp = NULL;
	nfsip = NULL;
	do {
		err = vfs_vfind(nfsfsp->nfsfs_vfsp->vfs_dev, ino, lkf, 0, &vp);
		if (err && (err != ENOENT || fhp == NULL))
			continue;

		if (!(err || fhp == NULL)) {
			/*
			 * Make sure we're still referencing the correct handle.
			 */
			nfsip = V2NFS(vp);
			if (vp->v_vfsp == nfsfsp->nfsfs_vfsp &&
			    vp->v_ino == ino &&
			    memcmp(&nfsip->nfsi_fh, fhp, sizeof(nfs_fh)) == 0) {
				if (ismylock)
					VOP_UNLOCK(nfsip->nfsi_vp);
				*nfsipp = nfsip;
				continue;
			}
			v_put(vp);
			vp = NULL;
		}

		/*
		 * Ok, we didn't find it but do have enough information
		 * to create a new vnode instance. Do that then.
		 */
		err = 0;
		nfsip = m_alloc(sizeof(struct nfsino));
		if (nfsip == NULL) {
			LOG(LOG_ERR, "nfs_findi: can't alloc new nfsino");
			err = ENOMEM;
			continue;
		}
		(void )memset(nfsip, 0, sizeof(struct nfsino));
		NFSI_INIT(nfsip, nfsfsp, ino, fhp);
		vp = v_new(nfsfsp->nfsfs_vfsp, ino, &nfs_vops, nfsip);
		if (vp == NULL) {
			/*
			 * Oops! Already present.
			 */
			free(nfsip);
			continue;
		}
		nfsip->nfsi_vp = vp;
		if (ismylock || lkf != RWLK_WRITE) {
			VOP_UNLOCK(vp);
			/*
			 * This is *so* stupid. Why don't we just have
			 * a downgrade?
			 */
			if (!ismylock) {
				err = VOP_LOCK(vp, lkf, 0);
				if (err) {
					LOG(LOG_ERR,
					    ("nfs_findi: lock err "
					     FMT_Z_DEV_T "." FMT_Z_INO_T
					     " err (%d)"),
					    vp->v_vfsp->vfs_dev,
					    vp->v_ino,
					    err);
					v_gone(vp);
					break;		/* no way */
				}
			}
		}
	} while (!err && vp == NULL);

	if (!err) {
		*nfsipp = nfsip;
		if (fattrp != NULL)
			nfs_cacheattrs(nfsip, fattrp);
	}

	return err;
}

/*
 * Reclaim resources held by the NFS ino passed.
 */
void
nfs_reclaimi(struct nfsino *nfsip)
{
	int	err;

	err = rw_destroy(&nfsip->nfsi_alock);
	if (err)
		panic("nfs_reclaim: can't destroy attrs lock (%d)", err);

	free(nfsip);
}
