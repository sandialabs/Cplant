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

IDENTIFY("$Id: nfs_rw.c,v 1.4 2000/11/25 00:33:39 lward Exp $");

/*
 * Transfer data from a file.
 */
int
nfs_vop_read(struct vnode *vp,
	     z_off_t offset,
	     char *buf,
	     u_int32_t *xferlen,
	     struct creds *crp)
{
	int	err;
	struct vstat vstbuf;
	u_int32_t remain;
	struct buffer *bp;
	u_int32_t off;
	size_t	len;
	size_t	rgnlen;

	err = VOP_GETATTR(vp, &vstbuf);
	if (err)
		return err;
	if (offset > 4294967295U) {
		*xferlen = 0;
		return 0;
	}
	remain = *xferlen;
	if (*xferlen > 4294967295U - offset)
		remain = 4294967295U - offset;
	*xferlen = 0;
	while (remain) {
		bp = v_findbuf(vp, offset / vstbuf.vst_blocksize);
		BUG_CHECK(bp != NULL);
		off = offset % vstbuf.vst_blocksize;
		BUG_CHECK(bp->b_size > off);
		len = bp->b_size - off;
		if (len > remain)
			len = remain;
		err = buf_read(vp, bp, off, len, crp);
		if (err) {
			v_invalbuf(bp);
			break;
		}
		BUG_CHECK(off >= bp->b_off);
		rgnlen =
		    bp->b_len > off - bp->b_off
		      ? bp->b_len - (off - bp->b_off)
		      : 0;
                if (rgnlen < len)
			len = rgnlen;
		if (len)
			(void )memcpy(buf, bp->b_data + off, len);
		buf_put(bp);
		if (!len)
			break;				/* at EOF */
		*xferlen += len;
		buf += len;
		offset += len;
		remain -= len;
	}
	if (*xferlen)
		err = 0;
	return err;
}

/*
 * Transfer data to a file.
 */
int
nfs_vop_write(struct vnode *vp,
	      z_off_t offset,
	      char *buf,
	      u_int32_t *xferlen,
	      struct creds *crp)
{
	int	err;
	struct vstat vstbuf;
	u_int32_t remain;
	struct buffer *bp;
	u_int32_t off;
	size_t	len;
	size_t	rgnlen;

	err = VOP_GETATTR(vp, &vstbuf);
	if (err)
		return err;
	if (offset > 4294967295U) {
		*xferlen = 0;
		return EFBIG;
	}
	remain = *xferlen;
	if (*xferlen > 4294967295U - offset)
		remain = 4294967295U - offset;
	*xferlen = 0;
	while (remain) {
		bp = v_findbuf(vp, offset / vstbuf.vst_blocksize);
		BUG_CHECK(bp != NULL);
		/*
		 * Have to do a buf_iowait() before we copy the data into
		 * the buffer. Otherwise, the mutex could be dropped in
		 * buf_doio() prior to the operation being posted but with
		 * the data we altered in place. We want readers to see
		 * only data that was successfully transferred.
		 */
		if (buf_iowait(bp) != 0) {
			buf_put(bp);
			continue;
		}
		off = offset % vstbuf.vst_blocksize;
		len = bp->b_size - off;
		if (len > remain)
			len = remain;
		(void )memcpy(bp->b_data + off, buf, len);
		err = buf_write(vp, bp, off, len, crp);
		if (err) {
			v_invalbuf(bp);
			break;
		}
		BUG_CHECK(off >= bp->b_off);
		rgnlen =
		    bp->b_len > off - bp->b_off
		      ? bp->b_len - (off - bp->b_off)
		      : 0;
		if (!rgnlen) {
			LOG(LOG_WARNING,
			    ("nfs_vop_write: ["
			     FMT_Z_DEV_T ":" FMT_Z_INO_T
			     "] zero length write?"),
			    vp->v_vfsp->vfs_dev,
			    vp->v_ino);
		}
                if (rgnlen < len)
			len = rgnlen;
		buf_put(bp);
		BUG_CHECK(len);
		if (!len)
			break;
		*xferlen += len;
		buf += len;
		offset += len;
		remain -= len;
	}
	if (*xferlen)
		err = 0;
	return err;
}

static int
nfs_perform_read(struct nfsino *nfsip,
		 u_int32_t offset,
		 char	*buf,
		 size_t *buflenp,
		 struct creds *crp)
{
	readargs args;
	readres	result;
	int	err;

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.file = nfsip->nfsi_fh;
	args.offset = (u_int )offset;
	args.totalcount = args.count = *buflenp;

	result.readres_u.reply.data.data_val = buf;
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
	if (err) {
		*buflenp = 0;
		return err;
	}
	nfs_cacheattrs(nfsip, &result.readres_u.reply.attributes, 1);

	*buflenp = result.readres_u.reply.data.data_len;

	/*
	 * No xdr_free() needed this time. We were very careful to use only
	 * local or derived space.
	 */
	return 0;
}

static int
nfs_perform_write(struct nfsino *nfsip,
		  u_int32_t offset,
		  const char *buf,
		  size_t *buflenp,
		  struct creds *crp)
{
	writeargs args;
	attrstat result;
	int	err;

	(void )memset(&args, 0, sizeof(args));
	(void )memset(&result, 0, sizeof(result));

	args.file = nfsip->nfsi_fh;
	args.offset = args.beginoffset = (u_int )offset;
	args.data.data_len = args.totalcount = *buflenp;
	args.data.data_val = (char *)buf;

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
	if (err) {
		*buflenp = 0;
		return err;
	}
	/*
	 * We just changed the mtime of the file with this write.
	 * Avoid invalidating all the buffers then.
	 */
	nfs_cacheattrs(nfsip, &result.attrstat_u.attributes, 0);

	/*
	 * No xdr_free() needed this time. We were very careful to use only
	 * local or derived space.
	 */
	return 0;
}

/*
 * Perform IO.
 *
 * We can't do asynchronous NFS IO. I wish we could but the RPC library
 * doesn't allow that. Just perform the operation.
 */
int
nfs_vop_strategy(struct vnode *vp, struct buffer *bp)
{
	struct nfsino *nfsip = V2NFS(vp);
	fattr	fa;
	int	err;

	err = nfs_igetattr(nfsip, &fa, 0);
	if (!err) {
		BUG_CHECK(bp->b_off < fa.blocksize &&
			  fa.blocksize - bp->b_off >= bp->b_len);
		/*
		 * Range check.
		 */
		if (bp->b_blkno > 4294967295U / fa.blocksize)
			err = EFBIG;
	}

	if (!err) {
		u_int32_t off;

		/*
		 * Make the RPC.
		 */
		off = bp->b_blkno * fa.blocksize + bp->b_off;
		if (bp->b_flags & B_READ)
			err =
			    nfs_perform_read(nfsip,
					     off,
					     bp->b_data + bp->b_off,
					     &bp->b_len,
					     bp->b_crp);
		else
			err =
			    nfs_perform_write(nfsip,
					      off,
					      bp->b_data + bp->b_off,
					      &bp->b_len,
					      bp->b_crp);
	}

	if (err) {
		/*
		 * On error set valid data length to zero.
		 *
		 */
		bp->b_len = 0;
	}
	buf_iodone(bp, err);
	return 0;
}
