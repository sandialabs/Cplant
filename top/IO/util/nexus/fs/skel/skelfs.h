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
#ifndef _SKELFS_H_
#define _SKELFS_H_

#include "cmn.h"
#include "vfs.h"
#include "vnode.h"
#include "direct.h"
#include "dtree.h"

/*
 * Skeletal file system types and definitions
 *
 * most of the skeletal file system is implemented in the underlying
 * dtree layer.
 *
 * $Id: skelfs.h,v 1.3 2000/05/23 09:34:41 smorgan Exp $
 *
 * $Log: skelfs.h,v $
 * Revision 1.3  2000/05/23 09:34:41  smorgan
 *
 *
 * modifications to support automatic skeletal file system directory creation
 * via a configuration file.
 *
 * Revision 1.2  2000/03/15 02:53:20  lward
 *
 *
 * Ugh! Remove all the RPC and NFS 'isms. Why were they there in the first place?
 * This isn't NFS.
 *
 * Then, fix a bad bug at restart. When generating the file handles, it was
 * done into an nfs_fh (?!?!?), which is 32 bytes. At restart, the upper
 * levels dutifully passed down that the handle was 32 bytes of data. The
 * routine that tore the handle apart decided this was too big and err'd.
 *
 * Now, build the right sized handle and tell the caller it's correct length.
 * Seems to work now.
 *
 * Revision 1.1  2000/02/16 00:49:40  smorgan
 *
 *
 * Addition of the skeletal file system.
 *
 *
 */

struct skelfs {
	mutex_t		sfs_instlk;		/* vnode instance lock */

	dtree		*sfs_root;		/* the directory structure */
	z_dev_t		sfs_dev;		/* device number */
	struct vfs	*sfs_vfs;		/* the VFS layer above skel */
};

#define V2SKELFS(v)	((struct skelfs *) (v)->v_vfsp->vfs_private)
#define V2SKEL(v)	((dtree_ino *) (v)->v_private)

#define VFS2SKELFS(vfs)	((struct skelfs *) (vfs)->vfs_private)

#define SKELFS_DEVMASK	0x00000000

#define SKELFS_FHSIZE	(sizeof(z_dev_t) + sizeof(z_ino_t))

#define SKELFS_DELIMSTR	"/"			/* directory path delimeter */
#define SKELFS_DELIM	'/'			/* directory path delimeter */

#define SKELFS_INTERNAL	_skelfs_intern		/* internal skeletal fs */

extern struct vfsops	skelfs_vfsops;
extern struct vops	skelfs_vops;
extern dtree_ino_ops	skelfs_ino_ops;
extern struct skelfs	*_skelfs_intern;

/* skelfs global helper function(s) */
extern int		skelfs_findi(struct skelfs *, dtree_ino *, rwty_t,
				     struct vnode **);
extern int		skelfs_register(struct skelfs *, const char *,
					struct vstat *);

#endif /* _SKELFS_H_ */

