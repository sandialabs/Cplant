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
 *  linux/fs/nfs/dir.c
 *
 *  Copyright (C) 1992  Rick Sladkey
 *
 *  nfs directory handling functions
 *
 * 10 Apr 1996	Added silly rename for unlink	--okir
 * 28 Sep 1996	Improved directory cache --okir
 * 23 Aug 1997  Claus Heine claus@momo.math.rwth-aachen.de 
 *              Re-implemented silly rename for unlink, newly implemented
 *              silly rename for nfs_rename() following the suggestions
 *              of Olaf Kirch (okir) found in this file.
 *              Following Linus comments on my original hack, this version
 *              depends only on the dcache stuff and doesn't touch the inode
 *              layer (iput() and friends).
 *  6 Jun 1999  Cache readdir lookups in the page cache. -DaveM
 *  7 Oct 1999  Rewrite of Dave's readdir stuff for NFSv3 support, and in order
 *              to simplify cookie handling. -Trond
 */

#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/sunrpc/types.h>
#include <linux/nfs.h>
#include <linux/nfs2.h>
#include <linux/nfs3.h>
#include <linux/nfs_fs.h>
#include <linux/nfs_mount.h>
#include <linux/sunrpc/auth.h>
#include <linux/sunrpc/clnt.h>

#include <asm/segment.h>	/* for fs functions */

#define NFS_PARANOIA 1
/* #define NFS_DEBUG_VERBOSE 1 */

static int nfs_safe_remove(struct dentry *);

static ssize_t nfs_dir_read(struct file *, char *, size_t, loff_t *);
static int nfs_readdir(struct file *, void *, filldir_t);
static struct dentry *nfs_lookup(struct inode *, struct dentry *);
static int nfs_create(struct inode *, struct dentry *, int);
static int nfs_mkdir(struct inode *, struct dentry *, int);
static int nfs_rmdir(struct inode *, struct dentry *);
static int nfs_unlink(struct inode *, struct dentry *);
static int nfs_symlink(struct inode *, struct dentry *, const char *);
static int nfs_link(struct dentry *, struct inode *, struct dentry *);
static int nfs_mknod(struct inode *, struct dentry *, int, int);
static int nfs_rename(struct inode *, struct dentry *,
		      struct inode *, struct dentry *);

static struct file_operations nfs_dir_operations = {
	NULL,			/* lseek - default */
	nfs_dir_read,		/* read - bad */
	NULL,			/* write - bad */
	nfs_readdir,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* mmap */
	nfs_open,		/* open */
	NULL,			/* flush */
	nfs_release,		/* release */
	NULL			/* fsync */
};

struct inode_operations nfs_dir_inode_operations = {
	&nfs_dir_operations,	/* default directory file-ops */
	nfs_create,		/* create */
	nfs_lookup,		/* lookup */
	nfs_link,		/* link */
	nfs_unlink,		/* unlink */
	nfs_symlink,		/* symlink */
	nfs_mkdir,		/* mkdir */
	nfs_rmdir,		/* rmdir */
	nfs_mknod,		/* mknod */
	nfs_rename,		/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
 	NULL,			/* writepage */
 	NULL,			/* bmap */
	NULL,			/* truncate */
	nfs_permission,		/* permission */
	NULL,			/* smap */
	NULL,			/* updatepage */
	nfs_revalidate,		/* revalidate */
};

static ssize_t
nfs_dir_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return -EISDIR;
}

typedef u32 * (*decode_dirent_t)(u32 *, struct nfs_entry *, int);
typedef struct {
	struct file	*file;
	struct page	*page;
	unsigned long	page_index;
	unsigned	page_offset;
	u64		target;
	struct nfs_entry *entry;
	decode_dirent_t	decode;
	int		plus;
	int		error;
} nfs_readdir_descriptor_t;

/* Now we cache directories properly, by stuffing the dirent
 * data directly in the page cache.
 *
 * Inode invalidation due to refresh etc. takes care of
 * _everything_, no sloppy entry flushing logic, no extraneous
 * copying, network direct to page cache, the way it was meant
 * to be.
 *
 * NOTE: Dirent information verification is done always by the
 *	 page-in of the RPC reply, nowhere else, this simplies
 *	 things substantially.
 */
static
int nfs_readdir_filler(nfs_readdir_descriptor_t *desc, struct page *page)
{
	struct file	*file = desc->file;
	struct dentry	*dir = file->f_dentry;
	struct inode	*inode = dir->d_inode;
	struct nfs_fattr dir_attr;
	void		*buffer = (void *)page_address(page);
	int		plus = NFS_USE_READDIRPLUS(inode);
	int		error;

	dfprintk(VFS, "NFS: nfs_readdir_filler() reading cookie %Lu into page %lu.\n", (long long)desc->entry->cookie, page->offset);

 again:
	error = NFS_CALL(readdir, inode, (dir, &dir_attr,
					  nfs_file_cred(file),
					  desc->entry->cookie, buffer,
					  NFS_SERVER(inode)->dtsize, plus));
	nfs_refresh_inode(inode, &dir_attr);
	/* We requested READDIRPLUS, but the server doesn't grok it */
	if (desc->plus && error == -ENOTSUPP) {
		NFS_FLAGS(inode) &= ~NFS_INO_ADVISE_RDPLUS;
		plus = 0;
		goto again;
	}
	if (error < 0)
		goto error;
	flush_dcache_page(page_address(page)); /* Is this correct? */
	set_bit(PG_uptodate, &page->flags);

	/* Ensure consistent page alignment of the data.
	 * Note: assumes we have exclusive access to this inode either
	 *	 throught inode->i_sem or some other mechanism.
	 */
	if (page->offset == 0)
		invalidate_inode_pages(inode);
	nfs_unlock_page(page);
	return 0;
 error:
	set_bit(PG_error, &page->flags);
	nfs_unlock_page(page);
	desc->error = error;
	return -EIO;
}

/*
 * Given a pointer to a buffer that has already been filled by a call
 * to readdir, find the next entry.
 *
 * If the end of the buffer has been reached, return -EAGAIN, if not,
 * return the offset within the buffer of the next entry to be
 * read.
 */
static inline
int find_dirent(nfs_readdir_descriptor_t *desc, struct page *page)
{
	struct nfs_entry *entry = desc->entry;
	char		*start = (char *)page_address(page),
			*p = start;
	int		loop_count = 0,
			status = 0;

	for(;;) {
		p = (char *)desc->decode((u32*)p, entry, desc->plus);
		if (IS_ERR(p)) {
			status = PTR_ERR(p);
			break;
		}
		desc->page_offset = p - start;
		dfprintk(VFS, "NFS: found cookie %Lu\n", (long long)entry->cookie);
		if (entry->prev_cookie == desc->target)
			break;
		if (loop_count++ > 200) {
			loop_count = 0;
			schedule();
		}
	}
	dfprintk(VFS, "NFS: find_dirent() returns %d\n", status);
	return status;
}

/*
 * Find the given page, and call find_dirent() in order to try to
 * return the next entry.
 */
static inline
int find_dirent_page(nfs_readdir_descriptor_t *desc)
{
	struct inode	*inode = desc->file->f_dentry->d_inode;
	struct page	*page;
	unsigned long	index = desc->page_index;
	int		status;

	dfprintk(VFS, "NFS: find_dirent_page() searching directory page %ld\n", desc->page_index);

	if (desc->page) {
		page_cache_release(desc->page);
		desc->page = NULL;
	}

	page = read_cache_page(inode, index,
			       (filler_t *)nfs_readdir_filler, desc);
	if (IS_ERR(page)) {
		status = PTR_ERR(page);
		goto out;
	}

	/* NOTE: Someone else may have changed the READDIRPLUS flag */
	desc->plus = NFS_USE_READDIRPLUS(inode);
	status = find_dirent(desc, page);
	if (status >= 0)
		desc->page = page;
	else
		page_cache_release(page);
 out:
	dfprintk(VFS, "NFS: find_dirent_page() returns %d\n", status);
	return status;
}

/*
 * Recurse through the page cache pages, and return a
 * filled nfs_entry structure of the next directory entry if possible.
 *
 * The target for the search is 'desc->target'.
 */
static inline
int readdir_search_pagecache(nfs_readdir_descriptor_t *desc)
{
	int		res = 0;
	int		loop_count = 0;

	dfprintk(VFS, "NFS: readdir_search_pagecache() searching for cookie %Lu\n", (long long)desc->target);
	for (;;) {
		res = find_dirent_page(desc);
		if (res != -EAGAIN)
			break;
		/* Align to beginning of next page */
		desc->page_offset = 0;
		desc->page_index += PAGE_CACHE_SIZE;
		if (loop_count++ > 200) {
			loop_count = 0;
			schedule();
		}
	}
	dfprintk(VFS, "NFS: readdir_search_pagecache() returned %d\n", res);
	return res;
}

/*
 * Once we've found the start of the dirent within a page: fill 'er up...
 */
static 
int nfs_do_filldir(nfs_readdir_descriptor_t *desc, void *dirent,
		   filldir_t filldir)
{
	struct file	*file = desc->file;
	struct nfs_entry *entry = desc->entry;
	char		*start = (char *)page_address(desc->page),
			*p = start + desc->page_offset;
	unsigned long	fileid;
	int		loop_count = 0,
			res = 0;

	dfprintk(VFS, "NFS: nfs_do_filldir() filling starting @ cookie %Lu\n", (long long)desc->target);

	for(;;) {
		/* Note: entry->prev_cookie contains the cookie for
		 *	 retrieving the current dirent on the server */
		fileid = nfs_fileid_to_ino_t(entry->ino);
		res = filldir(dirent, entry->name, entry->len, 
			      entry->prev_cookie, fileid);
		if (res < 0)
			break;
		file->f_pos = desc->target = entry->cookie;
		p = (char *)desc->decode((u32 *)p, entry, desc->plus);
		if (IS_ERR(p)) {
			if (PTR_ERR(p) == -EAGAIN) {
				desc->page_offset = 0;
				desc->page_index += PAGE_CACHE_SIZE;
			}
			break;
		}
		desc->page_offset = p - start;
		if (loop_count++ > 200) {
			loop_count = 0;
			schedule();
		}
	}
	page_cache_release(desc->page);
	desc->page = NULL;

	dfprintk(VFS, "NFS: nfs_do_filldir() filling ended @ cookie %Lu; returning = %d\n", (long long)desc->target, res);
	return res;
}

/*
 * If we cannot find a cookie in our cache, we suspect that this is
 * because it points to a deleted file, so we ask the server to return
 * whatever it thinks is the next entry. We then feed this to filldir.
 * If all goes well, we should then be able to find our way round the
 * cache on the next call to readdir_search_pagecache();
 *
 * NOTE: we cannot add the anonymous page to the pagecache because
 *	 the data it contains might not be page aligned. Besides,
 *	 we should already have a complete representation of the
 *	 directory in the page cache by the time we get here.
 */
static inline
int uncached_readdir(nfs_readdir_descriptor_t *desc, void *dirent,
		     filldir_t filldir)
{
	struct file	*file = desc->file;
	struct dentry	*dir = file->f_dentry;
	struct inode	*inode = dir->d_inode;
	struct nfs_fattr dir_attr;
	struct page	*page = NULL;
	unsigned long	cache_page;
	u32		*p;
	int		status = -EIO;

	dfprintk(VFS, "NFS: uncached_readdir() searching for cookie %Lu\n", (long long)desc->target);
	if (desc->page) {
		page_cache_release(desc->page);
		desc->page = NULL;
	}

	cache_page = page_cache_alloc();
	if (!cache_page) {
		status = -ENOMEM;
		goto out;
	}
	page = page_cache_entry(cache_page);
	p = (u32 *)page_address(page);
	status = NFS_CALL(readdir, inode, (dir, &dir_attr,
					   nfs_file_cred(file),
					   desc->target, p,
					   NFS_SERVER(inode)->dtsize, 0));
	nfs_refresh_inode(inode, &dir_attr);
	if (status >= 0) {
		p = desc->decode(p, desc->entry, 0);
		if (IS_ERR(p))
			status = PTR_ERR(p);
		else
			desc->entry->prev_cookie = desc->target;
	}
	if (status < 0)
		goto out_release;

	desc->page_index = 0;
	desc->page_offset = 0;
	desc->page = page;
	status = nfs_do_filldir(desc, dirent, filldir);

	/* Reset read descriptor so it searches the page cache from
	 * the start upon the next call to readdir_search_pagecache() */
	desc->page_index = 0;
	desc->page_offset = 0;
	memset(desc->entry, 0, sizeof(*desc->entry));
 out:
	dfprintk(VFS, "NFS: uncached_readdir() returns %d\n", status);
	return status;
 out_release:
	page_cache_release(page);
	goto out;
}

/* The file offset position is now represented as a true offset into the
 * page cache as is the case in most of the other filesystems.
 */
static int nfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	struct dentry	*dentry = filp->f_dentry;
	struct inode	*inode = dentry->d_inode;
	nfs_readdir_descriptor_t my_desc,
			*desc = &my_desc;
	struct nfs_entry my_entry;
	long		res;

	res = nfs_revalidate(dentry);
	if (res < 0)
		return res;

	/*
	 * filp->f_pos points to the file offset in the page cache.
	 * but if the cache has meanwhile been zapped, we need to
	 * read from the last dirent to revalidate f_pos
	 * itself.
	 */
	memset(desc, 0, sizeof(*desc));
	memset(&my_entry, 0, sizeof(my_entry));

	desc->file = filp;
	desc->target = filp->f_pos;
	desc->entry = &my_entry;
	desc->decode = NFS_PROTO(inode)->decode_dirent;

	while(!desc->entry->eof) {
		res = readdir_search_pagecache(desc);
		if (res == -EBADCOOKIE) {
			/* This means either end of directory */
			if (desc->entry->cookie == desc->target) {
				res = 0;
				break;
			}
			/* Or that the server has 'lost' a cookie */
			res = uncached_readdir(desc, dirent, filldir);
			if (res >= 0)
				continue;
		}
		if (res < 0)
			break;

		res = nfs_do_filldir(desc, dirent, filldir);
		if (res < 0) {
			res = 0;
			break;
		}
	}
	if (desc->page)
		page_cache_release(desc->page);
	if (desc->error < 0)
		return desc->error;
	if (res < 0)
		return res;
	return 0;
}


/*
 * Whenever an NFS operation succeeds, we know that the dentry
 * is valid, so we update the revalidation timestamp.
 */
static inline void
nfs_renew_times(struct dentry * dentry)
{
		dentry->d_time = jiffies;
}

static inline int nfs_dentry_force_reval(struct dentry *dentry, int flags)
{
	struct inode *inode = dentry->d_inode;
	unsigned long timeout = NFS_ATTRTIMEO(inode);

	/*
	 * If it's the last lookup in a series, we use a stricter
	 * cache consistency check by looking at the parent mtime.
	 *
	 * If it's been modified in the last hour, be really strict.
	 * (This still means that we can avoid doing unnecessary
	 * work on directories like /usr/share/bin etc which basically
	 * never change).
	 */
	if (!(flags & LOOKUP_CONTINUE)) {
		long diff = CURRENT_TIME - dentry->d_parent->d_inode->i_mtime;

		if (diff < 15*60)
			timeout = 0;
	}

	return time_after(jiffies,dentry->d_time + timeout);
}

/*
 * We judge how long we want to trust negative
 * dentries by looking at the parent inode mtime.
 *
 * If mtime is close to present time, we revalidate
 * more often.
 */
#define NFS_REVALIDATE_NEGATIVE (1 * HZ)
static inline int nfs_neg_need_reval(struct dentry *dentry)
{
	struct inode *dir = dentry->d_parent->d_inode;
	unsigned long timeout = NFS_ATTRTIMEO(dir);
	long diff = CURRENT_TIME - dir->i_mtime;

	if (diff < 5*60 && timeout > NFS_REVALIDATE_NEGATIVE)
		timeout = NFS_REVALIDATE_NEGATIVE;

	return time_after(jiffies, dentry->d_time + timeout);
}

/*
 * This is called every time the dcache has a lookup hit,
 * and we should check whether we can really trust that
 * lookup.
 *
 * NOTE! The hit can be a negative hit too, don't assume
 * we have an inode!
 *
 * If the dentry is older than the revalidation interval, 
 * we do a new lookup and verify that the dentry is still
 * correct.
 */
static int nfs_lookup_revalidate(struct dentry * dentry, int flags)
{
	struct dentry		*dir = dentry->d_parent;
	struct inode		*inode = dentry->d_inode,
				*dir_i = dir->d_inode;
	struct nfs_fh		fhandle;
	struct nfs_fattr	fattr, dir_attr;
	int error;

	/*
	 * If we don't have an inode, let's look at the parent
	 * directory mtime to get a hint about how often we
	 * should validate things..
	 */
	if (!inode) {
		if (nfs_neg_need_reval(dentry))
			goto out_bad;
		goto out_valid;
	}

	if (is_bad_inode(inode)) {
		dfprintk(VFS, "nfs_lookup_validate: %s/%s has dud inode\n",
			dir->d_name.name, dentry->d_name.name);
		goto out_bad;
	}

	if (!nfs_dentry_force_reval(dentry, flags))
		goto out_valid;

	if (IS_ROOT(dentry)) {
		__nfs_revalidate_inode(dentry);
		goto out_valid_renew;
	}

	if (NFS_FLAGS(inode) & NFS_INO_STALE)
		goto out_bad;

	/*
	 * Do a new lookup and check the dentry attributes.
	 */
	error = NFS_CALL(lookup, dir_i, (dir, &dir_attr,
				  &dentry->d_name, &fhandle, &fattr));
	if (error < 0)
		goto out_bad;

	/* Inode number matches? */
	if (!(fattr.valid & NFS_ATTR_FATTR) ||
	    NFS_FSID(inode) != fattr.fsid ||
	    NFS_FILEID(inode) != fattr.fileid)
		goto out_bad;

	/* Filehandle matches? */
	if (NFS_FH(dentry)->size == 0)
		goto out_bad;

	if (NFS_FH(dentry)->size != fhandle.size ||
	    memcmp(NFS_FH(dentry)->data, fhandle.data, fhandle.size))
		goto out_bad;

	/* Ok, remeber that we successfully checked it.. */
	nfs_refresh_inode(inode, &fattr);
	nfs_refresh_inode(dir_i, &dir_attr);

 out_valid_renew:
	nfs_renew_times(dentry);
 out_valid:
	return 1;
 out_bad:
	if (!list_empty(&dentry->d_subdirs))
		shrink_dcache_parent(dentry);
	/* If we have submounts, don't unhash ! */
	if (have_submounts(dentry))
		goto out_valid;
	d_drop(dentry);
	if (dentry->d_parent->d_inode)
		NFS_CACHEINV(dentry->d_parent->d_inode);
	if (inode && S_ISDIR(inode->i_mode))
		NFS_CACHEINV(inode);
	return 0;
}

/*
 * This is called from dput() when d_count is going to 0.
 */
static void nfs_dentry_delete(struct dentry *dentry)
{
	if (dentry->d_flags & DCACHE_NFSFS_RENAMED) {
		/* Unhash it, so that ->d_iput() would be called */
		d_drop(dentry);
	}
}

__inline__ struct nfs_fh *nfs_fh_alloc(void)
{
	struct nfs_fh *p;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;
	memset(p, 0, sizeof(*p));
	return p;
}

__inline__ void nfs_fh_free(struct nfs_fh *p)
{
	kfree(p);
}

/*
 * Called when the dentry is being freed to release private memory.
 */
static void nfs_dentry_release(struct dentry *dentry)
{
	if (dentry->d_fsdata) {
		nfs_fh_free(dentry->d_fsdata);
		dentry->d_fsdata = NULL;
	}
}

/*
 * Called when the dentry loses inode.
 * We use it to clean up silly-renamed files.
 */
static void nfs_dentry_iput(struct dentry *dentry, struct inode *inode)
{
	if (dentry->d_flags & DCACHE_NFSFS_RENAMED)
		nfs_complete_unlink(dentry);
	iput(inode);
}

struct dentry_operations nfs_dentry_operations = {
	nfs_lookup_revalidate,	/* d_revalidate(struct dentry *, int) */
	NULL,			/* d_hash */
	NULL,			/* d_compare */
	nfs_dentry_delete,	/* d_delete(struct dentry *) */
	nfs_dentry_release,	/* d_release(struct dentry *) */
	nfs_dentry_iput		/* d_iput */
};

static struct dentry *nfs_lookup(struct inode *dir_i, struct dentry * dentry)
{
	struct dentry *dir = dentry->d_parent;
	struct inode *inode;
	int error;
	struct nfs_fh fhandle;
	struct nfs_fattr fattr, dir_attr;

	dfprintk(VFS, "NFS: lookup(%s/%s)\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);

	error = -ENAMETOOLONG;
	if (dentry->d_name.len > NFS_SERVER(dir_i)->namelen)
		goto out;

	dentry->d_op = &nfs_dentry_operations;

	if (!dentry->d_fsdata) {
		dentry->d_fsdata = nfs_fh_alloc();
		if (!dentry->d_fsdata) {
			error = -ENOMEM;
			goto out;
		}
	}

#if NFS_FIXME
	inode = nfs_dircache_lookup(dir_i, dentry);
	if (inode)
		goto no_entry;
#endif

	error = NFS_CALL(lookup, dir_i, (dir, &dir_attr,
				 &dentry->d_name, &fhandle, &fattr));
	nfs_refresh_inode(dir_i, &dir_attr);
	inode = NULL;
	if (error == -ENOENT)
		goto no_entry;

	if (!error) {
		error = -EACCES;
if (dentry->d_inode) while (1) printk("wedgy: i = %p, di = %p\n", inode, dentry->d_inode);
		inode = nfs_fhget(dentry, &fhandle, &fattr);
		if (inode) {
	    no_entry:
			d_add(dentry, inode);
			nfs_renew_times(dentry);
			error = 0;
		}
	}
out:
	return ERR_PTR(error);
}

/*
 * Code common to create, mkdir, and mknod.
 */
static int nfs_instantiate(struct dentry *dentry, struct nfs_fh *fhandle,
				struct nfs_fattr *fattr)
{
	struct inode *inode;
	int error = -EACCES;

	if (dentry->d_inode)
		return 0;
	inode = nfs_fhget(dentry, fhandle, fattr);
	if (inode) {
		d_instantiate(dentry, inode);
		nfs_renew_times(dentry);
		error = 0;
	}
	return error;
}

/*
 * Following a failed create operation, we drop the dentry rather
 * than retain a negative dentry. This avoids a problem in the event
 * that the operation succeeded on the server, but an error in the
 * reply path made it appear to have failed.
 */
static int nfs_create(struct inode *dir_i, struct dentry *dentry, int mode)
{
	struct dentry	*dir = dentry->d_parent;
	struct iattr	 attr;
	struct nfs_fattr fattr, dir_attr;
	struct nfs_fh	 fhandle;
	int		 error;

	dfprintk(VFS, "NFS: create(%x/%ld, %s\n",
		dir_i->i_dev, dir_i->i_ino, dentry->d_name.name);

#ifdef NFSD_BROKEN_UID
	/* We set uid/gid in the request because IBM's broken nfsd
	 * uses the root uid/gid otherwise. Argh!
	 * (Hopefully the server will override the gid when the directory
	 * has the sticky bit set. Irix may have a problem here...)
	 */
	attr.ia_mode = mode;
	attr.ia_valid = ATTR_MODE | ATTR_UID | ATTR_GID;
	attr.ia_uid = current->fsuid;
	attr.ia_gid = current->fsgid;
#else
	attr.ia_mode = mode;
	attr.ia_valid = ATTR_MODE;
#endif

	/*
	 * Invalidate the dir cache before the operation to avoid a race.
	 * The 0 argument passed into the create function should one day
	 * contain the O_EXCL flag if requested. This allows NFSv3 to
	 * select the appropriate create strategy. Currently open_namei
	 * does not pass the create flags.
	 */
	nfs_zap_caches(dir_i);
	error = NFS_CALL(create, dir_i, (dir, &dir_attr, &dentry->d_name,
			&attr, 0, &fhandle, &fattr));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (!error && fhandle.size != 0)
		error = nfs_instantiate(dentry, &fhandle, &fattr);
	if (error || fhandle.size == 0)
		d_drop(dentry);
	return error;
}

/*
 * See comments for nfs_proc_create regarding failed operations.
 */
static int nfs_mknod(struct inode *dir_i, struct dentry *dentry, int mode, int rdev)
{
	struct dentry	*dir = dentry->d_parent;
	struct iattr	 attr;
	struct nfs_fattr fattr, dir_attr;
	struct nfs_fh	 fhandle;
	int		 error;

	dfprintk(VFS, "NFS: mknod(%x/%ld, %s\n",
		dir_i->i_dev, dir_i->i_ino, dentry->d_name.name);

#ifdef NFSD_BROKEN_UID
	attr.ia_valid = ATTR_MODE | ATTR_UID | ATTR_GID;
	attr.ia_mode = mode;
	attr.ia_uid = current->fsuid;
	attr.ia_gid = current->fsgid;
#else
	attr.ia_valid = ATTR_MODE;
	attr.ia_mode = mode;
#endif


	nfs_zap_caches(dir_i);
	error = NFS_CALL(mknod, dir_i, (dir, &dir_attr, &dentry->d_name,
				&attr, rdev, &fhandle, &fattr));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (!error && fhandle.size != 0)
		error = nfs_instantiate(dentry, &fhandle, &fattr);
	if (error || fhandle.size == 0)
		d_drop(dentry);
	return error;
}

/*
 * See comments for nfs_proc_create regarding failed operations.
 */
static int nfs_mkdir(struct inode *dir_i, struct dentry *dentry, int mode)
{
	struct dentry   *dir = dentry->d_parent;
	struct iattr	 attr;
	struct nfs_fattr fattr, dir_attr;
	struct nfs_fh	 fhandle;
	int		 error;

	dfprintk(VFS, "NFS: mkdir(%x/%ld, %s)\n",
		dir_i->i_dev, dir_i->i_ino, dentry->d_name.name);

#ifdef NFSD_BROKEN_UID
	attr.ia_valid = ATTR_MODE | ATTR_UID | ATTR_GID;
	attr.ia_mode = mode | S_IFDIR;
	attr.ia_uid = current->fsuid;
	attr.ia_gid = current->fsgid;
#else
	attr.ia_valid = ATTR_MODE;
	attr.ia_mode = mode | S_IFDIR;
#endif

	nfs_zap_caches(dir_i);
	error = NFS_CALL(mkdir, dir_i, (dir, &dir_attr,
				&dentry->d_name, &attr, &fhandle, &fattr));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (!error && fhandle.size != 0)
		error = nfs_instantiate(dentry, &fhandle, &fattr);

	if (error || fhandle.size == 0)
		d_drop(dentry);
	return error;
}

static int nfs_rmdir(struct inode *dir_i, struct dentry *dentry)
{
	struct dentry	*dir = dentry->d_parent;
	struct nfs_fattr dir_attr;
	int		 error;

	dfprintk(VFS, "NFS: rmdir(%x/%ld, %s\n",
		dir_i->i_dev, dir_i->i_ino, dentry->d_name.name);

	nfs_zap_caches(dir_i);
	error = NFS_CALL(rmdir, dir_i, (dir, &dir_attr, &dentry->d_name));
	nfs_refresh_inode(dir_i, &dir_attr);

	/* Free the inode */
	if (!error)
		d_delete(dentry);

	return error;
}


/*  Note: we copy the code from lookup_dentry() here, only: we have to
 *  omit the directory lock. We are already the owner of the lock when
 *  we reach here. And "down(&dir->i_sem)" would make us sleep forever
 *  ('cause WE have the lock)
 * 
 *  VERY IMPORTANT: calculate the hash for this dentry!!!!!!!!
 *  Otherwise the cached lookup DEFINITELY WILL fail. And a new dentry
 *  is created. Without the DCACHE_NFSFS_RENAMED flag. And with d_count
 *  == 1. And trouble.
 *
 *  Concerning my choice of the temp name: it is just nice to have
 *  i_ino part of the temp name, as this offers another check whether
 *  somebody attempts to remove the "silly renamed" dentry itself.
 *  Which is something that I consider evil. Your opinion may vary.
 *  BUT:
 *  Now that I compute the hash value right, it should be possible to simply
 *  check for the DCACHE_NFSFS_RENAMED flag in dentry->d_flag instead of
 *  doing the string compare.
 *  WHICH MEANS:
 *  This offers the opportunity to shorten the temp name. Currently, I use
 *  the hex representation of i_ino + an event counter. This sums up to
 *  as much as 36 characters for a 64 bit machine, and needs 20 chars on 
 *  a 32 bit machine.
 *  QUINTESSENCE
 *  The use of i_ino is simply cosmetic. All we need is a unique temp
 *  file name for the .nfs files. The event counter seemed to be adequate.
 *  And as we retry in case such a file already exists, we are guaranteed
 *  to succeed.
 */

static
struct dentry *nfs_silly_lookup(struct dentry *parent, char *silly, int slen)
{
	struct qstr    sqstr;
	struct dentry *sdentry;
	struct dentry *res;

	sqstr.name = silly;
	sqstr.len  = slen;
	sqstr.hash = full_name_hash(silly, slen);
	sdentry = d_lookup(parent, &sqstr);
	if (!sdentry) {
		sdentry = d_alloc(parent, &sqstr);
		if (sdentry == NULL)
			return ERR_PTR(-ENOMEM);
		res = nfs_lookup(parent->d_inode, sdentry);
		if (res) {
			dput(sdentry);
			return res;
		}
	}
	return sdentry;
}

static int nfs_sillyrename(struct inode *dir_i, struct dentry *dentry)
{
	struct dentry	*dir = dentry->d_parent;
	static unsigned int sillycounter = 0;
	struct nfs_fattr dir_attr;
	const int        i_inosize  = sizeof(dir_i->i_ino)*2;
	const int        countersize = sizeof(sillycounter)*2;
	const int        slen       = strlen(".nfs") + i_inosize + countersize;
	struct qstr      qsilly;
	char             silly[slen+1];
	struct dentry *  sdentry;
	int              error = -EIO;

	dfprintk(VFS, "NFS: silly-rename(%s/%s, ct=%d)\n",
		dentry->d_parent->d_name.name, dentry->d_name.name, 
		dentry->d_count);

	/*
	 * Note that a silly-renamed file can be deleted once it's
	 * no longer in use -- it's just an ordinary file now.
	 */
	if (dentry->d_count == 1)
		goto out;  /* No need to silly rename. */

#ifdef NFS_PARANOIA
	if (!dentry->d_inode)
		printk(KERN_ERR "NFS: silly-renaming %s/%s, negative dentry??\n",
		       dentry->d_parent->d_name.name, dentry->d_name.name);
#endif
	/*
	 * We don't allow a dentry to be silly-renamed twice.
	 */
	error = -EBUSY;
	if (dentry->d_flags & DCACHE_NFSFS_RENAMED)
		goto out;

	sprintf(silly, ".nfs%*.*lx",
		i_inosize, i_inosize, dentry->d_inode->i_ino);

	sdentry = NULL;
	do {
		char *suffix = silly + slen - countersize;

		if (sdentry)
			dput(sdentry);
		sillycounter++;
		sprintf(suffix, "%*.*x", countersize, countersize, sillycounter);

		dfprintk(VFS, "trying to rename %s to %s\n",
			 dentry->d_name.name, silly);
		
		sdentry = nfs_silly_lookup(dentry->d_parent, silly, slen);
		/*
		 * N.B. Better to return EBUSY here ... it could be
		 * dangerous to delete the file while it's in use.
		 */
		if (IS_ERR(sdentry))
			goto out;
	} while(sdentry->d_inode != NULL); /* need negative lookup */

	nfs_zap_caches(dir_i);
	qsilly.name = silly;
	qsilly.len  = strlen(silly);
	error = NFS_CALL(rename, dir_i, (dir, &dir_attr, &dentry->d_name,
				  dir, &dir_attr, &qsilly));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (!error) {
		nfs_renew_times(dentry);
		d_move(dentry, sdentry);
		error = nfs_async_unlink(dentry);
 		/* If we return 0 we don't unlink */
	}
	dput(sdentry);
out:
	return error;
}

/*
 * Remove a file after making sure there are no pending writes,
 * and after checking that the file has only one user. 
 *
 * We update inode->i_nlink and free the inode prior to the operation
 * to avoid possible races if the server reuses the inode.
 */
static int nfs_safe_remove(struct dentry *dentry)
{
	struct nfs_fattr dir_attr;
	struct dentry	*dir = dentry->d_parent;
	struct inode	*dir_i = dir->d_inode,   
			*inode = dentry->d_inode;
	int		 error = 0, rehash = 0;
		
	/*
	 * Unhash the dentry while we remove the file ...
	 */
	if (!list_empty(&dentry->d_hash)) {
		d_drop(dentry);
		rehash = 1;
	}

	/* If the dentry was sillyrenamed, we simply call d_delete() */
	if (dentry->d_flags & DCACHE_NFSFS_RENAMED)
		goto out_delete;

	dfprintk(VFS, "NFS: safe_remove(%s/%s)\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);

	/* N.B. not needed now that d_delete is done in advance? */
	error = -EBUSY;
	if (!inode) {
#ifdef NFS_PARANOIA
		printk(KERN_ERR "nfs_safe_remove: %s/%s already negative??\n",
		       dentry->d_parent->d_name.name, dentry->d_name.name);
#endif
	}

	if (dentry->d_count > 1) {
#ifdef NFS_PARANOIA
		printk(KERN_INFO "nfs_safe_remove: %s/%s busy, d_count=%d\n",
		       dentry->d_parent->d_name.name, dentry->d_name.name, dentry->d_count);
#endif
		goto out;
	}

	nfs_zap_caches(dir_i);
	if (inode)
		NFS_CACHEINV(inode);
	error = NFS_CALL(remove, dir_i, (dir, &dir_attr, &dentry->d_name));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (error < 0)
		goto out;

 out_delete:
	/*
	 * Free the inode
	 */
	d_delete(dentry);
out:
	/*
	 * Rehash the negative dentry if the operation succeeded.
	 */
	if (rehash)
		d_rehash(dentry);
	return error;
}

/*  We do silly rename. In case sillyrename() returns -EBUSY, the inode
 *  belongs to an active ".nfs..." file and we return -EBUSY.
 *
 *  If sillyrename() returns 0, we do nothing, otherwise we unlink.
 */
static int nfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int error;

	dfprintk(VFS, "NFS: unlink(%x/%ld, %s)\n",
		dir->i_dev, dir->i_ino, dentry->d_name.name);

	if (dentry->d_inode)
		nfs_wb_all(dentry->d_inode);
	error = nfs_sillyrename(dir, dentry);
	if (error && error != -EBUSY) {
		error = nfs_safe_remove(dentry);
		if (!error) {
			nfs_renew_times(dentry);
		}
	}
	return error;
}

static int
nfs_symlink(struct inode *dir_i, struct dentry *dentry, const char *symname)
{
	struct dentry	*dir = dentry->d_parent;
	struct nfs_fattr dir_attr, sym_attr;
	struct nfs_fh    sym_fh;
	struct iattr     attr;
	struct qstr      qsymname;
	int              error, mode, maxlen;

	dfprintk(VFS, "NFS: symlink(%x/%ld, %s, %s)\n",
		dir_i->i_dev, dir_i->i_ino, dentry->d_name.name, symname);

	error = -ENAMETOOLONG;
	maxlen = (NFS_PROTO(dir_i)->version==2) ? NFS2_MAXPATHLEN : NFS3_MAXPATHLEN;
	if (strlen(symname) > maxlen)
		goto out;

#ifdef NFS_PARANOIA
	if (dentry->d_inode)
		printk(KERN_WARNING "nfs_proc_symlink: %s/%s not negative!\n",
		       dentry->d_parent->d_name.name, dentry->d_name.name);
#endif
	/*
	 * Fill in the sattr for the call.

 	 * Note: SunOS 4.1.2 crashes if the mode isn't initialized!
	 */
#ifdef NFSD_BROKEN_UID
	attr.ia_valid = ATTR_MODE|ATTR_UID|ATTR_GID;
	attr.ia_mode = mode = S_IFLNK | S_IRWXUGO;
	attr.ia_uid = current->fsuid;
	attr.ia_gid = current->fsgid;
#else
	attr.ia_valid = ATTR_MODE;
	attr.ia_mode = mode = S_IFLNK | S_IRWXUGO;
#endif


	qsymname.name = symname;
	qsymname.len  = strlen(symname);

	nfs_zap_caches(dir_i);
	error = NFS_CALL(symlink, dir_i, (dir, &dir_attr,
				&dentry->d_name, &qsymname, &attr,
				&sym_fh, &sym_attr));
	nfs_refresh_inode(dir_i, &dir_attr);
	if (!error && sym_fh.size != 0 && (sym_attr.valid & NFS_ATTR_FATTR)) {
		error = nfs_instantiate(dentry, &sym_fh, &sym_attr);
	} else {
		if (error == -EEXIST)
			printk(KERN_INFO "nfs_proc_symlink: %s/%s already exists??\n",
			       dentry->d_parent->d_name.name,
			       dentry->d_name.name);
		d_drop(dentry);
	}

out:
	return error;
}

static int 
nfs_link(struct dentry *old_dentry, struct inode *dir_i, struct dentry *dentry)
{
	struct dentry	*dir = dentry->d_parent;
	struct inode	*inode = old_dentry->d_inode;
	struct nfs_fattr old_attr, dir_attr;
	int		 error;

	dfprintk(VFS, "NFS: link(%s/%s -> %s/%s)\n",
		old_dentry->d_parent->d_name.name, old_dentry->d_name.name,
		dentry->d_parent->d_name.name, dentry->d_name.name);

	/*
	 * Drop the dentry in advance to force a new lookup.
	 * Since nfs_proc_link doesn't return a file handle,
	 * we can't use the existing dentry.
	 */
	d_drop(dentry);
	nfs_zap_caches(dir_i);
	NFS_CACHEINV(inode);
	error = NFS_CALL(link, inode, (old_dentry, &old_attr,
				       dir, &dir_attr, &dentry->d_name));
	nfs_refresh_inode(inode, &old_attr);
	nfs_refresh_inode(dir_i, &dir_attr);
	return error;
}

/*
 * RENAME
 * FIXME: Some nfsds, like the Linux user space nfsd, may generate a
 * different file handle for the same inode after a rename (e.g. when
 * moving to a different directory). A fail-safe method to do so would
 * be to look up old_dir/old_name, create a link to new_dir/new_name and
 * rename the old file using the sillyrename stuff. This way, the original
 * file in old_dir will go away when the last process iput()s the inode.
 *
 * FIXED.
 * 
 * It actually works quite well. One needs to have the possibility for
 * at least one ".nfs..." file in each directory the file ever gets
 * moved or linked to which happens automagically with the new
 * implementation that only depends on the dcache stuff instead of
 * using the inode layer
 *
 * Unfortunately, things are a little more complicated than indicated
 * above. For a cross-directory move, we want to make sure we can get
 * rid of the old inode after the operation.  This means there must be
 * no pending writes (if it's a file), and the use count must be 1.
 * If these conditions are met, we can drop the dentries before doing
 * the rename.
 *
 * FIXME: Sun seems to take this even one step further. The connectathon
 * test suite has a file that renames open file A to open file B,
 * and expects a silly rename to happen for B.
 */
static int nfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		      struct inode *new_dir, struct dentry *new_dentry)
{
	struct nfs_fattr old_attr, new_attr;
	struct inode *   old_inode = old_dentry->d_inode;
	struct inode *   new_inode = new_dentry->d_inode;
	struct dentry *  dentry = NULL, *rehash = NULL;
	int              error;

	/*
	 * To prevent any new references to the target during the rename,
	 * we unhash the dentry and free the inode in advance.
	 */
	if (!list_empty(&new_dentry->d_hash)) {
		d_drop(new_dentry);
		rehash = new_dentry;
	}

	dfprintk(VFS, "NFS: rename(%s/%s -> %s/%s, ct=%d)\n",
		old_dentry->d_parent->d_name.name, old_dentry->d_name.name,
		new_dentry->d_parent->d_name.name, new_dentry->d_name.name,
		new_dentry->d_count);

	/*
	 * First check whether the target is busy ... we can't
	 * safely do _any_ rename if the target is in use.
	 *
	 * For files, make a copy of the dentry and then do a 
	 * silly-rename. If the silly-rename succeeds, the
	 * copied dentry is hashed and becomes the new target.
	 *
	 * With directories check is done in VFS.
	 */
	error = -EBUSY;
	if (new_inode)
		nfs_wb_all(new_inode);
	if (new_dentry->d_count > 1 && new_inode) {
		int err;
		/* copy the target dentry's name */
		dentry = d_alloc(new_dentry->d_parent,
				 &new_dentry->d_name);
		if (!dentry)
			goto out;

		/* silly-rename the existing target ... */
		err = nfs_sillyrename(new_dir, new_dentry);
		if (!err) {
			new_dentry = rehash = dentry;
			new_inode = NULL;
			/* hash the replacement target */
			d_instantiate(new_dentry, NULL);
		}

		/* dentry still busy? */
		if (new_dentry->d_count > 1) {
#ifdef NFS_PARANOIA
		printk(KERN_INFO "nfs_rename: target %s/%s busy, d_count=%d\n",
			new_dentry->d_parent->d_name.name,
			new_dentry->d_name.name,new_dentry->d_count);
#endif
			goto out;
		}
	}

	/*
	 * ... prune child dentries and writebacks if needed.
	 */
	if (old_dentry->d_count > 1) {
		nfs_wb_all(old_inode);
		shrink_dcache_parent(old_dentry);
	}

	if (new_dentry->d_count > 1 && new_inode) {
#ifdef NFS_PARANOIA
		printk(KERN_INFO "nfs_rename: new dentry %s/%s busy, d_count=%d\n",
			new_dentry->d_parent->d_name.name,new_dentry->d_name.name,new_dentry->d_count);
#endif
		goto out;
	}

	if (new_inode)
		d_delete(new_dentry);

	nfs_zap_caches(new_dir);
	nfs_zap_caches(old_dir);
	error = NFS_CALL(rename, old_dir,
			 (old_dentry->d_parent, &old_attr, &old_dentry->d_name,
			  new_dentry->d_parent, &new_attr, &new_dentry->d_name));
	nfs_refresh_inode(old_dir, &old_attr);
	nfs_refresh_inode(new_dir, &new_attr);

out:
	/* Update the dcache if needed */
	if (rehash)
		d_rehash(rehash);
	if (!error && !S_ISDIR(old_inode->i_mode))
		d_move(old_dentry, new_dentry);
	/* new dentry created? */
	if (dentry)
		dput(dentry);
	return error;
}

int
nfs_permission(struct inode *i, int msk)
{
	struct nfs_fattr	fattr;
	struct dentry		*de = NULL;
	int			err = vfs_permission(i, msk);
	struct list_head	*start, *tmp;

	if (!NFS_PROTO(i)->access)
		goto out;
	/*
	 * Trust UNIX mode bits except:
	 *
	 * 1) When override capabilities may have been invoked
	 * 2) When root squashing may be involved
	 * 3) When ACLs may overturn a negative answer */
	if (!capable(CAP_DAC_OVERRIDE) && !capable(CAP_DAC_READ_SEARCH)
	    && (current->fsuid != 0) && (current->fsgid != 0)
	    && err != -EACCES)
		goto out;

	tmp = start = &i->i_dentry;
	while ((tmp = tmp->next) != start) {
		de = list_entry(tmp, struct dentry, d_alias);
		if (de->d_inode == i)
			break;
	}
	if (!de || de->d_inode != i)
		return 0;

	err = NFS_CALL(access, i, (de, msk, &fattr, 0));

	if (err == -EACCES && NFS_CLIENT(i)->cl_droppriv &&
	    current->uid != 0 && current->gid != 0 &&
	    (current->fsuid != current->uid || current->fsgid != current->gid))
		err = NFS_CALL(access, i, (de, msk, &fattr, 1));

	nfs_refresh_inode(i, &fattr);
 out:
	return err;
}

/*
 * Local variables:
 *  version-control: t
 *  kept-new-versions: 5
 * End:
 */
