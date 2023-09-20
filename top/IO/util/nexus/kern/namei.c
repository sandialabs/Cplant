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
#include <stdio.h>					/* for NULL */
#include <stdlib.h>

#include "cmn.h"
#include "vnode.h"

IDENTIFY("$Id: namei.c,v 1.2 2001/07/18 18:57:46 rklundt Exp $");

static int inamei(struct namei_data *, size_t);

/*
 * Follow a symlink for inamei().
 *
 * Note:
 *	The symlink vnode is expected to be ref'd and locked. That lock is
 *	dropped by this routine.
 */
static int
follow_symlink(struct namei_data *ndp, size_t depth)
{
	char	*pathbuf;
	size_t	len;
	int	err;

	len = Z_MAXPATHLEN;
	ndp->nd_path = pathbuf = m_alloc(len + 1);
	if (pathbuf == NULL) {
		VOP_UNLOCK(ndp->nd_vp);
		return ENOMEM;
	}

	err = 0;
	if (++depth > Z_MAXSYMLINKS)
		err = ELOOP;
	if (!err)
		err = VOP_READLINK(ndp->nd_vp, pathbuf, (u_int32_t *)&len);
	VOP_UNLOCK(ndp->nd_vp);
	ndp->nd_vp = NULL;
	if (!err) {
		pathbuf[len] = '\0';
		err = inamei(ndp, depth);
	}

	ndp->nd_path = NULL;
	free(pathbuf);
	return err;
}

/*
 * Translate path name to vnode ptr. The directory argument should be referenced
 * but not locked.
 *
 * At successful return the field nd_vp points at the desired vnode. It is
 * referenced and locked, if desired. If WANTPARENT was set, the field
 * nd_parent points at the parent directory of the vnode pointed at by nd_vp.
 * This vnode is referenced only.
 */
static int
inamei(struct namei_data *ndp, size_t depth)
{
	const char *path;
	struct vnode *dvp;
	struct vnode *vp;
	char	*component;
	int	err;
	struct vstat vstbuf;
	struct namei_data nd;
	char	*cp;
	unsigned indx;

	path = ndp->nd_path;
	vp = ndp->nd_parent;

	component = m_alloc(Z_MAXNAMLEN + 1);
	if (component == NULL)
		return ENOMEM;

	err = 0;
	if (*path == '/') {
		/*
		 * It's absolute. We don't need the passed directory vnode at
		 * all.
		 */
		err = v_get(rootvp, RWLK_NONE, 0);
		if (err) {
			LOG(LOG_ERR, "namei: can't reference root? (%d)", err);
			vp = NULL;
		} else
			vp = rootvp;
	}
	if (!err) {
		/*
		 * We need another reference so we don't drop the
		 * one the caller holds.
		 */
		v_ref(vp);
		err = VOP_LOCK(vp, RWLK_READ, 0);
	}

	/*
	 * Get each component from the pathname and look it up until
	 * no more or error.
	 */
	dvp = vp;
	while (!err && *path != '\0') {  /* dvp ref'd, vp locked & ref'd */
		/*
		 * Invariant:
		 *
		 * Directory vnode pointer is referenced but not locked.
		 * The component vnode pointer is referenced and locked.
		 */

		/*
		 * Acquire attributes.
		 */
		err = VOP_GETATTR(vp, &vstbuf);

		/*
		 * Deal with symbolic link.
		 */
		if (!err &&
		    !(ndp->nd_flags & ND_NOFOLLOW) &&
		    VS_ISLNK(vstbuf.vst_mode)) {
			nd.nd_flags = ND_WANTPARENT;
			nd.nd_parent = dvp;
			nd.nd_path = NULL;
			nd.nd_vp = vp;
			nd.nd_lkf = RWLK_READ;
			nd.nd_crp = ndp->nd_crp;
			err = follow_symlink(&nd, depth);
			v_rele(dvp);
			dvp = nd.nd_parent;
			/*
			 * The routine follow_symlink() dropped the lock. We
			 * need only release.
			 */
			v_rele(vp);
			vp = nd.nd_vp;
			continue;
		}

		/*
		 * Get the next component.
		 */
		while (*path == '/')
			path++;
		for (indx = 0, cp = component;
		     indx < Z_MAXNAMLEN && *path != '\0' && *path != '/'; 
		     indx++)
			*cp++ = *path++;
		*cp = '\0';
		component[Z_MAXNAMLEN] = '\0';

		if (!indx)
			break; /* dvp ref'd, vp ref'd and locked */

		if (err)
			break;	/* dvp ref'd and locked, vp NULL */

		if (indx >= Z_MAXNAMLEN) {
			err = ENAMETOOLONG;
			break;	/* dvp ref'd and locked, vp NULL */
		}
		if (indx && !VS_ISDIR(vstbuf.vst_mode)) {
			err = ENOTDIR;	/* dvp ref'd and locked, vp NULL */
			break;
		}

		/*
		 * Shift.
		 */
		v_rele(dvp);
		dvp = vp;
		vp = NULL;

		/*
		 * Short circuit lookup of `.'
		 */
		if (component[0] == '.' && component[1] == '\0') {
			vp = dvp;
			v_ref(vp);
			continue;
		}

		/*
		 * Now, look up the component.
		 *
		 * Note:
		 *  v_lookup drops the lock held on the directory
		 * vnode pointer before acquiring the read lock on the
		 * component vnode pointer.
		 */
		err = v_lookup(dvp, component, ndp->nd_crp, RWLK_READ, &vp);
	}

	if (dvp != NULL && !(ndp->nd_flags & ND_WANTPARENT)) {
		v_rele(dvp);
		dvp = NULL;
	}

	if (vp != NULL && (err || ndp->nd_lkf != RWLK_READ)) {
		VOP_UNLOCK(vp);
		if (!err && ndp->nd_lkf == RWLK_WRITE)
			err = VOP_LOCK(vp, ndp->nd_lkf, 0);
	}

	if (err) {
		if (dvp != NULL)
			v_rele(dvp);
		if (vp != NULL)
			v_rele(vp);
	}

	if (!err) {
		ndp->nd_parent = dvp;
		ndp->nd_vp = vp;
	}

	if (component != NULL) free(component);

	return err;
}

int
namei(struct namei_data *ndp)
{

	return inamei(ndp, 0);
}
