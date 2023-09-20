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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "nfsfs.h"

/*
 * NFS client support.
 */

IDENTIFY("$Id: nfs_subr.c,v 1.6 2001/07/18 18:57:31 rklundt Exp $");

static unsigned fty_map[] = {
	0,						/* NFNON */
	VS_IFREG,					/* NFREG */
	VS_IFDIR,					/* NFDIR */
	0,						/* NFBLK */
	0,						/* NFCHR */
	VS_IFLNK,					/* NFLNK */
	0,						/* NFSOCK */
	0,						/* NFBAD */
	0,						/* NFFIFO */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
	0,						/* unused */
};

/*
 * Make an RPC call.
 */
int
nfs_docall(struct server *srvr,
	   bool_t (*f)(void *, void *, CLIENT *),
	   void *argp, void *result,
	   const struct timeval *timotvp,
	   struct creds *crp)
{
	CLIENT	*clnt;
	int	err;
	struct timeval retry_tv, start, tv;

#ifndef NDEBUG
		assert(timotvp->tv_sec >= 0 && timotvp->tv_usec >= 0);
#endif

	/*
	 * Initially, the retry timeout is set at 1/2 second.
	 */
	retry_tv.tv_sec = 0;
	retry_tv.tv_usec = 1000000 / 2;

	clnt = NULL;
	assert(gettimeofday(&start, NULL) == 0);
	while (1) {
		if (clnt == NULL) {
			clnt = client_activate(srvr, &tv, crp);
			if (clnt == NULL)
				return EIO;
		}
		clnt_control(clnt, CLSET_TIMEOUT, timotvp);
		clnt_control(clnt, CLSET_RETRY_TIMEOUT, &retry_tv);
		err = (*f)(argp, result, clnt);
		if (err != RPC_SUCCESS) {
			if (err == RPC_TIMEDOUT) {
				LOG(LOG_ERR,
				    "%s: NFS server didn't responding.",
				    srvr->host);
			} else
				LOG(LOG_ERR, clnt_sperror(clnt, srvr->host));
		}
		if (!(err == RPC_SUCCESS || err == RPC_TIMEDOUT)) {
			/*
			 * These errors can be persistent. The only cure
			 * seems to be to acquire a fresh client handle. Kill
			 * this one so it is not reused.
			 */
			client_destroy(srvr, clnt);
			clnt = NULL;
		}
		if (err == RPC_SUCCESS)
			break;

		/*
		 * Timed out yet?
		 */
		assert(gettimeofday(&tv, NULL) == 0);
		if (tv.tv_usec < start.tv_usec) {
			tv.tv_usec += 1000000;
			tv.tv_sec -= 1;
		}
		tv.tv_sec -= start.tv_sec;
		tv.tv_usec -= start.tv_usec;
		if (timotvp->tv_sec < tv.tv_sec ||
		    (timotvp->tv_sec == tv.tv_sec &&
		     timotvp->tv_usec <= tv.tv_usec)) {
			err = RPC_TIMEDOUT;
			break;
		}
	}
	if (clnt != NULL)
		client_idle(srvr, clnt);

	return err == RPC_SUCCESS ? 0 : EIO;
}

/*
 * Make a simple, one-time RPC.
 */
int
nfs_simple_call(const char *host,
		u_long prog, u_long vers, const char *proto,
		bool_t (*f)(void *, void *, CLIENT *),
		void *argp, void *result,
		struct timeval *timotvp,
		struct creds *crp)
{
	struct server *srvr;
	bool_t	ret;

	srvr = client_context_create(host, prog, vers, proto, 0);
	if (srvr == NULL)
		return EIO;
	ret = nfs_docall(srvr, f, argp, result, timotvp, crp);
	client_context_destroy(srvr);
	return ret;
}

/*
 * Translate NFS status to errno equivalent.
 */
int
nfs_errxlate(nfsstat status)
{
	int	err;
	static int map[] = {
		0,		EPERM,		ENOENT,		-1,
		-1,		EIO,		ENXIO,		-1,
		-1,		-1,		-1,		-1,
		-1,		EACCES,		-1,		-1,
		-1,		EEXIST,		-1,		ENODEV,
		ENOTDIR,	EISDIR,		-1,		-1,
		-1,		-1,		-1,		EFBIG,
		ENOSPC,		-1,		EROFS,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		ENAMETOOLONG,
		-1,		-1,		ENOTEMPTY,	-1,
		-1,		EDQUOT,		ESTALE,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		-1,
		-1,		-1,		-1,		EIO,
	};

	err = status >= sizeof(map) / sizeof(int) ? -1 : map[status];
	if (err < 0)
		LOG(LOG_ERR, "nfs_errxlate: unmapped error code (%d)", status);
	return err;
}

/*
 * Update (refresh) an inode's cached attributes.
 */
void
nfs_cacheattrs(struct nfsino *nfsip, fattr *fap)
{
	if (rw_lock(&nfsip->nfsi_alock, RWLK_WRITE, 0) != 0)
		panic("nfs_cacheattrs: can't lock attrs for writing");
	nfsip->nfsi_fattr = *fap;
	if (time(&nfsip->nfsi_atstamp) == (time_t )-1) {
		LOG(LOG_ERR,
		    "nfs_cacheattrs: can't get attributes stamp");
		nfsip->nfsi_atstamp = NFSATSTAMP_NONE;
	}
	rw_unlock(&nfsip->nfsi_alock);
}

/*
 * Return current file attributes for the NFS inode passed.
 */
int
nfs_igetattr(struct nfsino *nfsip, fattr *fap)
{
	struct nfsfs *nfsfsp;
	int	err;
	attrstat result;

	v_islocked(nfsip->nfsi_vp);

	/*
	 * Try cached attributes first.
	 */
	nfsfsp = VFS2NFS(nfsip->nfsi_vp->v_vfsp);
	if (rw_lock(&nfsip->nfsi_alock, RWLK_READ, 0) != 0)
		panic("nfs_igetattr: can't lock attrs for reading");
	if (nfsip->nfsi_atstamp != NFSATSTAMP_NONE &&
	    nfsip->nfsi_atstamp + nfsfsp->nfsfs_attrtimeo > time(NULL)) {
		*fap = nfsip->nfsi_fattr;
		rw_unlock(&nfsip->nfsi_alock);
		return 0;
	}
	rw_unlock(&nfsip->nfsi_alock);

	/*
	 * Nope. They need a refresh.
	 */
	err =
	    nfs_call(nfsfsp,
		     (bool_t (*)(void *, void *, CLIENT *))nfsproc_getattr_2,
		     &nfsip->nfsi_fh, &result,
                     &suser_creds);
	if (!err)
		nfs_errxlate(result.status);
	if (!err) {
		*fap = result.attrstat_u.attributes;

		/*
		 * Cache the attributes just returned as well.
		 */
		nfs_cacheattrs(nfsip, &result.attrstat_u.attributes);
	}
	xdr_free(xdr_attrstat, &result);

	return err;
}

/*
 * Translate NFS attributes to file system independent version.
 *
 * Beware when porting: It is assumed that the values in vstbuf are
 * 	large enough to hold the values in fattr.
 */
void
nfs_attrxlate(fattr *fap, z_dev_t dev, z_ino_t ino, struct vstat *vstp)
{

	vstp->vst_mode = fty_map[fap->type & 0xf];
	vstp->vst_mode |= fap->mode & 07777;
	vstp->vst_nlink = fap->nlink;
	vstp->vst_uid = fap->uid;
	vstp->vst_gid = fap->gid;
	vstp->vst_size = fap->size;
	vstp->vst_blocksize = fap->blocksize;
	vstp->vst_rdev = fap->rdev;
	vstp->vst_blocks = fap->blocks;
	vstp->vst_fsid = dev;
	vstp->vst_fileid = ino;
	vstp->vst_atime.tv_sec = fap->atime.seconds;
	vstp->vst_atime.tv_nsec = fap->atime.useconds;
	vstp->vst_atime.tv_nsec *= 1000;
	vstp->vst_mtime.tv_sec = fap->mtime.seconds;
	vstp->vst_mtime.tv_nsec = fap->mtime.useconds;
	vstp->vst_mtime.tv_nsec *= 1000;
	vstp->vst_ctime.tv_sec = fap->ctime.seconds;
	vstp->vst_ctime.tv_nsec = fap->ctime.useconds;
	vstp->vst_ctime.tv_nsec *= 1000;
}

/*
 * Load NFS sattr arg from vstat.
 */
int
nfs_sattrload(struct vstat *vstp, sattr *sap)
{

	if (!VST_VALID(vstp))
		return EINVAL;

	if (VST_DOESSET_MODE(vstp)) {
		/*
		 * NFS perm bits match those used in vnode.h
		 */
		sap->mode =
		    vstp->vst_mode & (VS_IRWXU | VS_IRWXU >> 3 | VS_IRWXU >> 6);
	} else
		sap->mode = (u_int )-1;
	sap->uid = (u_int )(VST_DOESSET_UID(vstp) ? vstp->vst_uid : -1);
	sap->gid = (u_int )(VST_DOESSET_GID(vstp) ? vstp->vst_gid : -1);
	sap->size = (u_int )(VST_DOESSET_SIZE(vstp) ? vstp->vst_size : -1);
	sap->atime.seconds =
	    (u_int )(VST_DOESSET_ATIME_SEC(vstp) ? vstp->vst_atime.tv_sec : -1);
	sap->atime.useconds =
	    (u_int )(VST_DOESSET_ATIME_NSEC(vstp)
		       ? vstp->vst_atime.tv_nsec / 1000 :
		       -1);
	sap->mtime.seconds =
	    (u_int )(VST_DOESSET_MTIME_SEC(vstp) ? vstp->vst_mtime.tv_sec : -1);
	sap->mtime.useconds =
	    (u_int )(VST_DOESSET_MTIME_NSEC(vstp)
		       ? vstp->vst_mtime.tv_nsec / 1000
		       : -1);

	return 0;
}

#ifdef ENHANCE_NFS_HANDLE_COMPRESSION
/*
 * To make NFS file handles more likely to compress the common information
 * is removed. This is done by XORing the root and inode file handles together.
 * The theory is that they arefixed in format and that, at least, the device
 * portion will be zero'd.
 *
 * Then, it's a permutation really.
 */
void
nfs_permutefh(nfs_fh *fh1p, nfs_fh *fh2p, nfs_fh *resultfhp)
{
	size_t	indx;
	char	*c1p, *c2p, *destp;

	c1p = fh1p->data;
	c2p = fh2p->data;
	destp = resultfhp->data;
	for (indx = 0; indx < NFS_FHSIZE; indx++)
		*destp++ = *c1p++ ^ *c2p++;
}
#endif

#ifdef Z_INO_T_IS_64
/*
 * Create a psuedo-unique 64-bit inode number from a 32-bit inode number as
 * delivered by NFS V2 and the file handle.
 */
z_ino_t
nfs_mki64(nfs_fh *fhp, u_int32_t i)
{
	u_int32_t acc;
	unsigned char *ucp;
	size_t	n;

	acc = 0;
	ucp = fhp->data;
	for (n = 0; n < sizeof(fhp->data); n++) {
		acc <<= 1;
		acc += *ucp++;
	}

	return ((z_ino_t )acc << 32) | (i & 0xffffffff);
}
#endif
