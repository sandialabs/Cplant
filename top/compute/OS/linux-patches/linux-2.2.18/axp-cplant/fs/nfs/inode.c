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
 *  linux/fs/nfs/inode.c
 *
 *  Copyright (C) 1992  Rick Sladkey
 *
 *  nfs inode and superblock handling functions
 *
 *  Modularised by Alan Cox <Alan.Cox@linux.org>, while hacking some
 *  experimental NFS changes. Modularisation taken straight from SYS5 fs.
 *
 *  Change to nfs_read_super() to permit NFS mounts to multi-homed hosts.
 *  J.S.Peatfield@damtp.cam.ac.uk
 *
 */

#include <linux/config.h>
#include <linux/module.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <linux/unistd.h>
#include <linux/major.h>
#include <linux/sunrpc/clnt.h>
#include <linux/sunrpc/stats.h>
#include <linux/nfs.h>
#include <linux/nfs2.h>
#include <linux/nfs3.h>
#include <linux/nfs_fs.h>
#include <linux/nfs_mount.h>
#include <linux/nfs_flushd.h>
#include <linux/lockd/bind.h>

#include <asm/spinlock.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#define CONFIG_NFS_SNAPSHOT 1
#define NFSDBG_FACILITY		NFSDBG_VFS
#define NFS_PARANOIA 1

static struct inode * __nfs_fhget(struct super_block *, struct nfs_fattr *);

static void nfs_read_inode(struct inode *);
static void nfs_put_inode(struct inode *);
static void nfs_delete_inode(struct inode *);
static int  nfs_notify_change(struct dentry *, struct iattr *);
static void nfs_put_super(struct super_block *);
static int  nfs_statfs(struct super_block *, struct statfs *, int);
static void nfs_umount_begin(struct super_block *);
static struct nfs_file *nfs_file_alloc(void);
static void nfs_file_free(struct nfs_file *p);

static struct super_operations nfs_sops = { 
	nfs_read_inode,		/* read inode */
	NULL,			/* write inode */
	nfs_put_inode,		/* put inode */
	nfs_delete_inode,	/* delete inode */
	nfs_notify_change,	/* notify change */
	nfs_put_super,		/* put superblock */
	NULL,			/* write superblock */
	nfs_statfs,		/* stat filesystem */
	NULL,			/* remount */
	NULL,			/* clear inode */
	nfs_umount_begin	/* umount attempt begin */
};


/*
 * RPC crutft for NFS
 */
static struct rpc_stat		nfs_rpcstat = { &nfs_program };
static struct rpc_version *	nfs_version[] = {
	NULL,
	NULL,
	&nfs_version2,
#ifdef CONFIG_NFS_V3
	&nfs_version3,
#endif
};

struct rpc_program		nfs_program = {
	"nfs",
	NFS_PROGRAM,
	sizeof(nfs_version) / sizeof(nfs_version[0]),
	nfs_version,
	&nfs_rpcstat,
};

static inline unsigned long
nfs_fattr_to_ino_t(struct nfs_fattr *fattr)
{
	return nfs_fileid_to_ino_t(fattr->fileid);
}

/*
 * We don't keep the file handle in the inode anymore to avoid bloating
 * struct inode and use a pointer to external memory instead.
 */
#define NFS_SB_FHSIZE(sb)	((sb)->u.nfs_sb.s_fhsize)

/*
 * The "read_inode" function doesn't actually do anything:
 * the real data is filled in later in nfs_fhget. Here we
 * just mark the cache times invalid, and zero out i_mode
 * (the latter makes "nfs_refresh_inode" do the right thing
 * wrt pipe inodes)
 */
static void
nfs_read_inode(struct inode * inode)
{
	inode->i_blksize = inode->i_sb->s_blocksize;
	inode->i_mode = 0;
	inode->i_rdev = 0;
	inode->i_op = NULL;
	NFS_FILEID(inode) = 0;
	NFS_FSID(inode) = 0;
	INIT_LIST_HEAD(&inode->u.nfs_i.read);
	INIT_LIST_HEAD(&inode->u.nfs_i.dirty);
	INIT_LIST_HEAD(&inode->u.nfs_i.commit);
	INIT_LIST_HEAD(&inode->u.nfs_i.writeback);
	inode->u.nfs_i.nread = 0;
	inode->u.nfs_i.ndirty = 0;
	inode->u.nfs_i.ncommit = 0;
	inode->u.nfs_i.npages = 0;
	NFS_CACHEINV(inode);
	NFS_ATTRTIMEO(inode) = NFS_MINATTRTIMEO(inode);
	NFS_ATTRTIMEO_UPDATE(inode) = jiffies;
}

static void
nfs_put_inode(struct inode * inode)
{
	dprintk("NFS: put_inode(%x/%ld)\n", inode->i_dev, inode->i_ino);
	/*
	 * We want to get rid of unused inodes ...
	 */
	if (inode->i_count == 1)
		inode->i_nlink = 0;
}

static void
nfs_delete_inode(struct inode * inode)
{
	dprintk("NFS: delete_inode(%x/%ld)\n", inode->i_dev, inode->i_ino);
#ifdef NFS_DEBUG_VERBOSE
	/*
	 * Flush out any pending write requests ...
	 */
	if (nfs_have_writebacks(inode) || nfs_have_read(inode)) {
		printk(KERN_ERR "nfs_delete_inode: inode %ld has pending RPC requests\n", inode->i_ino);
	}
#endif
	clear_inode(inode);
}

void
nfs_put_super(struct super_block *sb)
{
	struct nfs_server *server = &sb->u.nfs_sb.s_server;
	struct rpc_clnt	*rpc;

	/*
	 * First get rid of the request flushing daemon.
	 * Relies on rpc_shutdown_client() waiting on all
	 * client tasks to finish.
	 */
	nfs_reqlist_exit(server);

	if ((rpc = server->client) != NULL)
		rpc_shutdown_client(rpc);

	nfs_reqlist_free(server);

	if (!(server->flags & NFS_MOUNT_NONLM))
		lockd_down();	/* release rpc.lockd */

	rpciod_down();		/* release rpciod */

	kfree(server->hostname);

	MOD_DEC_USE_COUNT;
}

void
nfs_umount_begin(struct super_block *sb)
{
	struct nfs_server *server = &sb->u.nfs_sb.s_server;
	struct rpc_clnt	*rpc;

	/* -EIO all pending I/O */
	if ((rpc = server->client) != NULL)
		rpc_killall_tasks(rpc);
}


static inline unsigned long
nfs_block_bits(unsigned long bsize, unsigned char *nrbitsp)
{
	/* make sure blocksize is a power of two */
	if ((bsize & (bsize - 1)) || nrbitsp) {
		unsigned int	nrbits;

		for (nrbits = 31; nrbits && !(bsize & (1 << nrbits)); nrbits--)
			;
		bsize = 1 << nrbits;
		if (nrbitsp)
			*nrbitsp = nrbits;
	}

	return bsize;
}

/*
 * Calculate the number of 512byte blocks used.
 */
static inline unsigned long
nfs_calc_block_size(u64 tsize)
{
	off_t used = nfs_size_to_off_t(tsize);
	return (used + 511) >> 9;
}

/*
 * Compute and set NFS server blocksize
 */
static inline unsigned long
nfs_block_size(unsigned long bsize, unsigned char *nrbitsp)
{
	if (bsize < 1024)
		bsize = NFS_DEF_FILE_IO_BUFFER_SIZE;
	else if (bsize >= NFS_MAX_FILE_IO_BUFFER_SIZE)
		bsize = NFS_MAX_FILE_IO_BUFFER_SIZE;

	return nfs_block_bits(bsize, nrbitsp);
}

/*
 * Obtain the root inode of the file system.
 */
static struct inode *
nfs_get_root(struct super_block *sb, struct nfs_fh *rootfh)
{
	struct nfs_server	*server = &sb->u.nfs_sb.s_server;
	struct nfs_fattr	fattr;
	struct inode		*inode;
	int			error;

	if ((error = server->rpc_ops->getroot(server, rootfh, &fattr)) < 0) {
		printk(KERN_NOTICE "nfs_get_root: getattr error = %d\n", -error);
		return NULL;
	}

	inode = __nfs_fhget(sb, &fattr);
	return inode;
}

/*
 * The way this works is that the mount process passes a structure
 * in the data argument which contains the server's IP address
 * and the root file handle obtained from the server's mount
 * daemon. We stash these away in the private superblock fields.
 */
struct super_block *
nfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	struct nfs_mount_data	*data = (struct nfs_mount_data *) raw_data;
	struct nfs_server	*server;
	struct rpc_xprt		*xprt = 0;
	struct rpc_clnt		*clnt = 0;
	struct nfs_fh		*root_fh = NULL,
				*root = &data->root,
				fh;
	struct inode		*root_inode = NULL;
	unsigned int		authflavor;
	struct sockaddr_in	srvaddr;
	struct rpc_timeout	timeparms;
	struct nfs_fsinfo       fsinfo;
	int                     tcp, version, maxlen;

	MOD_INC_USE_COUNT;
	memset(&sb->u.nfs_sb, 0, sizeof(sb->u.nfs_sb));
	if (!data) {
		printk(KERN_NOTICE "nfs_read_super: missing data argument\n");
		goto failure;
	}

	memset(&fh, 0, sizeof(fh));
	if (data->version != NFS_MOUNT_VERSION) {
		printk(KERN_WARNING "nfs warning: mount version %s than kernel\n",
			data->version < NFS_MOUNT_VERSION ? "older" : "newer");
		if (data->version < 2)
			data->namlen = 0;
		if (data->version < 3)
			data->bsize  = 0;
		if (data->version < 4) {
			data->flags &= ~NFS_MOUNT_VER3;
			root = &fh;
			root->size = NFS2_FHSIZE;
			memcpy(root->data, data->old_root.data, NFS2_FHSIZE);
		}
	}

	/* We now require that the mount process passes the remote address */
	memcpy(&srvaddr, &data->addr, sizeof(srvaddr));
	if (srvaddr.sin_addr.s_addr == INADDR_ANY) {
		printk(KERN_WARNING "NFS: mount program didn't pass remote address!\n");
		goto failure;
	}

	lock_super(sb);

	sb->s_flags |= MS_ODD_RENAME; /* This should go away */

	sb->s_magic      = NFS_SUPER_MAGIC;
	sb->s_op         = &nfs_sops;

	sb->s_blocksize_bits = 0;
	sb->s_blocksize = nfs_block_bits(data->bsize, &sb->s_blocksize_bits);

	server           = &sb->u.nfs_sb.s_server;
	memset(server, 0, sizeof(*server));

	server->rsize    = nfs_block_size(data->rsize, NULL);
	server->wsize    = nfs_block_size(data->wsize, NULL);
	server->flags    = data->flags & NFS_MOUNT_FLAGMASK;

	if (data->flags & NFS_MOUNT_NOAC) {
		data->acregmin = data->acregmax = 0;
		data->acdirmin = data->acdirmax = 0;
	}
	server->acregmin = data->acregmin*HZ;
	server->acregmax = data->acregmax*HZ;
	server->acdirmin = data->acdirmin*HZ;
	server->acdirmax = data->acdirmax*HZ;

	server->namelen  = data->namlen;
	server->hostname = kmalloc(strlen(data->hostname) + 1, GFP_KERNEL);
	if (!server->hostname)
		goto failure_unlock;
	strcpy(server->hostname, data->hostname);

 nfsv3_try_again:
	/* Check NFS protocol revision and initialize RPC op vector
	 * and file handle pool. */
	if (data->flags & NFS_MOUNT_VER3) {
#ifdef CONFIG_NFS_V3
		server->rpc_ops = &nfs_v3_clientops;
		NFS_SB_FHSIZE(sb) = sizeof(unsigned short) + NFS3_FHSIZE;
		version = 3;
		if (data->version < 4) {
			printk(KERN_NOTICE "NFS: NFSv3 not supported by mount program.\n");
			goto failure_unlock;
		}
#else
		printk(KERN_NOTICE "NFS: NFSv3 not supported.\n");
		goto failure_unlock;
#endif
	} else {
		server->rpc_ops = &nfs_v2_clientops;
		NFS_SB_FHSIZE(sb) = sizeof(unsigned short) + NFS2_FHSIZE;
		version = 2;
	}

	/* Which protocol do we use? */
	tcp   = (data->flags & NFS_MOUNT_TCP);

	/* Initialize timeout values */
	timeparms.to_initval = data->timeo * HZ / 10;
	timeparms.to_retries = data->retrans;
	timeparms.to_maxval  = tcp? RPC_MAX_TCP_TIMEOUT : RPC_MAX_UDP_TIMEOUT;
	timeparms.to_exponential = 1;

	if (!timeparms.to_initval)
		timeparms.to_initval = (tcp ? 600 : 11) * HZ / 10;
	if (!timeparms.to_retries)
		timeparms.to_retries = 5;

	/* Now create transport and client */
	xprt = xprt_create_proto(tcp? IPPROTO_TCP : IPPROTO_UDP,
						&srvaddr, &timeparms);
	if (xprt == NULL) {
		printk(KERN_NOTICE "NFS: cannot create RPC transport. \n");
		goto failure_unlock;
	}

	/* Choose authentication flavor */
	authflavor = RPC_AUTH_UNIX;
	if (data->flags & NFS_MOUNT_SECURE)
		authflavor = RPC_AUTH_DES;
	else if (data->flags & NFS_MOUNT_KERBEROS)
		authflavor = RPC_AUTH_KRB;

	clnt = rpc_create_client(xprt, server->hostname, &nfs_program,
						version, authflavor);
	if (clnt == NULL) {
		printk(KERN_NOTICE "NFS: cannot create RPC client \n");
		goto failure_unlock;
	}

	clnt->cl_intr     = (data->flags & NFS_MOUNT_INTR)? 1 : 0;
	clnt->cl_softrtry = (data->flags & NFS_MOUNT_SOFT)? 1 : 0;
	clnt->cl_droppriv = (data->flags & NFS_MOUNT_BROKEN_SUID) ? 1 : 0;
	clnt->cl_chatty   = 1;
	server->client    = clnt;

	/* Fire up rpciod if not yet running */
	if (rpciod_up() != 0) {
		printk(KERN_NOTICE "NFS: cannot start rpciod!\n");
		goto failure_unlock;
	}

	/*
	 * Keep the super block locked while we try to get 
	 * the root fh attributes.
	 */
	root_fh = nfs_fh_alloc();
	if (!root_fh)
		goto out_no_fh;
	memcpy((u8*)root_fh, (u8*)root, sizeof(*root_fh));

	/* Did getting the root inode fail? */
	if ((root->size > NFS_SB_FHSIZE(sb)
	     || ! (root_inode = nfs_get_root(sb, root)))
	    && (data->flags & NFS_MOUNT_VER3)) {
		data->flags &= ~NFS_MOUNT_VER3;
		nfs_fh_free(root_fh);
		rpciod_down();
		rpc_shutdown_client(server->client);
		goto nfsv3_try_again;
	}
	if (!root_inode)
		goto failure_put_root;

	if (! (sb->s_root = d_alloc_root(root_inode, NULL)))
		goto failure_put_root;

	sb->s_root->d_op = &nfs_dentry_operations;
	sb->s_root->d_fsdata = root_fh;
	sb->u.nfs_sb.s_root = root_fh;

	/* Get some general file system info */
	if (server->rpc_ops->statfs(server, root, &fsinfo) >= 0) {
		if (server->namelen == 0)
			server->namelen = fsinfo.namelen;
	} else {
		printk(KERN_NOTICE "NFS: cannot retrieve file system info.\n");
		goto failure_put_root;
	}

	/* Fire up the writeback cache */
	if (nfs_reqlist_alloc(server) < 0) {
		printk(KERN_NOTICE "NFS: cannot initialize writeback cache.\n");
                goto failure_kill_reqlist;
	}

	if (data->rsize == 0 && tcp)
		server->rsize = nfs_block_size(fsinfo.rtpref, NULL);
	if (data->wsize == 0 && tcp)
		server->wsize = nfs_block_size(fsinfo.wtpref, NULL);

	/* NFSv3: we don't have bsize, but rather rtmult and wtmult... */
	if (!fsinfo.bsize)
		fsinfo.bsize = (fsinfo.rtmult>fsinfo.wtmult) ? fsinfo.rtmult : fsinfo.wtmult;
	/* Also make sure we don't go below rsize/wsize since
	 * RPC calls are expensive */
	if (fsinfo.bsize < server->rsize)
		fsinfo.bsize = server->rsize;
	if (fsinfo.bsize < server->wsize)
		fsinfo.bsize = server->wsize;

	if (data->bsize == 0)
		sb->s_blocksize = nfs_block_bits(fsinfo.bsize, &sb->s_blocksize_bits);
	if (server->rsize > fsinfo.rtmax)
		server->rsize = fsinfo.rtmax;
	server->rpages = (server->rsize + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	if (server->rpages > NFS_READ_MAXIOV) {
		server->rpages = NFS_READ_MAXIOV;
		server->rsize = server->rpages << PAGE_CACHE_SHIFT;
	}

	if (server->wsize > fsinfo.wtmax)
		server->wsize = fsinfo.wtmax;
	server->wpages = (server->wsize + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	if (server->wpages > NFS_WRITE_MAXIOV) {
		server->wpages = NFS_WRITE_MAXIOV;
		server->wsize = server->wpages << PAGE_CACHE_SHIFT;
	}

	server->dtsize = nfs_block_size(fsinfo.dtpref, NULL);
	if (server->dtsize > PAGE_CACHE_SIZE)
		server->dtsize = PAGE_CACHE_SIZE;
	if (server->dtsize > server->rsize)
		server->dtsize = server->rsize;

	maxlen = (version == 2) ? NFS2_MAXNAMLEN : NFS3_MAXNAMLEN;

	if (server->namelen == 0 || server->namelen > maxlen)
		server->namelen = maxlen;

	/* We're airborne */
	unlock_super(sb);

	/* Check whether to start the lockd process */
	if (!(server->flags & NFS_MOUNT_NONLM))
		lockd_up();

	return sb;

	/* Yargs. It didn't work out. */
 failure_kill_reqlist:
	nfs_reqlist_exit(server);
 failure_put_root:
	if (root_inode)
		iput(root_inode);
	if (root_fh)
		nfs_fh_free(root_fh);
 out_no_fh:
	rpciod_down();

 failure_unlock:
	/* Yargs. It didn't work out. */
	if (clnt)
		rpc_shutdown_client(server->client);
	else if (xprt)
		xprt_destroy(xprt);
	unlock_super(sb);
	nfs_reqlist_free(server);
	if (server->hostname)
		kfree(server->hostname);
	printk(KERN_NOTICE "NFS: cannot create RPC transport.\n");

failure:
	sb->s_dev = 0;
	MOD_DEC_USE_COUNT;
	return NULL;
}

static int
nfs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz)
{
	struct nfs_sb_info	*si = &sb->u.nfs_sb;
	struct nfs_server	*server = &si->s_server;
	unsigned char		blockbits;
	unsigned long		blockres;
	int			error;
	struct nfs_fsinfo	res;
	struct statfs		tmp;

	error = server->rpc_ops->statfs(server, NFS_FH(sb->s_root), &res);
	if (error) {
		printk(KERN_NOTICE "nfs_statfs: statfs error = %d\n", -error);
		memset(&res, 0, sizeof(res));
	}
	tmp.f_type = NFS_SUPER_MAGIC;
	if (res.bsize == 0)
		res.bsize = sb->s_blocksize;
	if (res.namelen == 0)
		res.namelen = server->namelen;
	tmp.f_bsize   = nfs_block_bits(res.bsize, &blockbits);
	blockres = (1 << blockbits) - 1;
	tmp.f_blocks  = (res.tbytes + blockres) >> blockbits;
	tmp.f_bfree   = (res.fbytes + blockres) >> blockbits;
	tmp.f_bavail  = (res.abytes + blockres) >> blockbits;
	tmp.f_files   = res.tfiles;
	tmp.f_ffree   = res.ffiles;
	tmp.f_namelen = res.namelen;
	return copy_to_user(buf, &tmp, bufsiz) ? -EFAULT : 0;
}

#if 0
int nfs_remountfs(struct super_block *sb, int *flags, char *data)
{
	struct nfs_server *server = &sb->u.nfs_sb.s_server;

	if (*flags & ~(NFS_MOUNT_NONLM|MS_RDONLY))
		return -EINVAL;

	if (*flags & ~NFS_MOUNT_NONLM)
		return 0;

	if ((*flags & NFS_MOUNT_NONLM) == (server->flags & NFS_MOUNT_NONLM))
		return 0;
}
#endif

/*
 * Free all unused dentries in an inode's alias list.
 *
 * Subtle note: we have to be very careful not to cause
 * any IO operations with the stale dentries, as this
 * could cause file corruption. But since the dentry
 * count is 0 and all pending IO for a dentry has been
 * flushed when the count went to 0, we're safe here.
 * Also returns the number of unhashed dentries
 */
static int
nfs_free_dentries(struct inode *inode)
{
	struct list_head *tmp, *head = &inode->i_dentry;
	int unhashed;

restart:
	tmp = head->next;
	unhashed = 0;
	while (tmp != head) {
		struct dentry *dentry = list_entry(tmp, struct dentry, d_alias);
		dget(dentry);
		if (!list_empty(&dentry->d_subdirs))
			shrink_dcache_parent(dentry);
		dprintk("nfs_free_dentries: found %s/%s, d_count=%d, hashed=%d\n",
			dentry->d_parent->d_name.name, dentry->d_name.name,
			dentry->d_count, !list_empty(&dentry->d_hash));
		if (dentry->d_count == 1) {
			d_drop(dentry);
			dput(dentry);
			goto restart;
		}
		if (list_empty(&dentry->d_hash))
			unhashed++;
		tmp = tmp->next;
		dput(dentry);
	}
	return unhashed;
}

/*
 * Zap the caches.
 */
void nfs_zap_caches(struct inode *inode)
{
	NFS_ATTRTIMEO(inode) = NFS_MINATTRTIMEO(inode);
	NFS_ATTRTIMEO_UPDATE(inode) = jiffies;

	invalidate_inode_pages(inode);

	memset(NFS_COOKIEVERF(inode), 0, sizeof(NFS_COOKIEVERF(inode)));
	NFS_CACHEINV(inode);
}

static void
nfs_invalidate_inode(struct inode *inode)
{
	umode_t save_mode = inode->i_mode;

	make_bad_inode(inode);
	inode->i_mode = save_mode;
	nfs_zap_caches(inode);
}

/*
 * Fill in inode information from the fattr.
 */
static void
nfs_fill_inode(struct inode *inode, struct nfs_fattr *fattr)
{
	/*
	 * Check whether the mode has been set, as we only want to
	 * do this once. (We don't allow inodes to change types.)
	 */
	if (inode->i_mode == 0) {
		NFS_FILEID(inode) = fattr->fileid;
		NFS_FSID(inode) = fattr->fsid;
		inode->i_mode = fattr->mode;
		if (S_ISREG(inode->i_mode))
			inode->i_op = &nfs_file_inode_operations;
		else if (S_ISDIR(inode->i_mode))
			inode->i_op = &nfs_dir_inode_operations;
		else if (S_ISLNK(inode->i_mode))
			inode->i_op = &nfs_symlink_inode_operations;
		else if (S_ISCHR(inode->i_mode)) {
			inode->i_op = &chrdev_inode_operations;
			inode->i_rdev = to_kdev_t(fattr->rdev);
		} else if (S_ISBLK(inode->i_mode)) {
			inode->i_op = &blkdev_inode_operations;
			inode->i_rdev = to_kdev_t(fattr->rdev);
		} else if (S_ISFIFO(inode->i_mode))
			init_fifo(inode);
		else
			inode->i_op = NULL;
		/*
		 * Preset the size and mtime, as there's no need
		 * to invalidate the caches.
		 */
		inode->i_size  = nfs_size_to_off_t(fattr->size);
		inode->i_mtime = nfs_time_to_secs(fattr->mtime);
		inode->i_atime = nfs_time_to_secs(fattr->atime);
		inode->i_ctime = nfs_time_to_secs(fattr->ctime);
		NFS_CACHE_CTIME(inode) = fattr->ctime;
		NFS_CACHE_MTIME(inode) = fattr->mtime;
		NFS_CACHE_ATIME(inode) = fattr->atime;
		NFS_CACHE_ISIZE(inode) = fattr->size;
		NFS_ATTRTIMEO(inode) = NFS_MINATTRTIMEO(inode);
		NFS_ATTRTIMEO_UPDATE(inode) = jiffies;
	}
	nfs_refresh_inode(inode, fattr);
}

static struct inode *
nfs_make_new_inode(struct super_block *sb, struct nfs_fattr *fattr)
{
	struct inode *inode = get_empty_inode();

	if (!inode)
		return NULL;	
	inode->i_sb = sb;
	inode->i_dev = sb->s_dev;
	inode->i_flags = 0;
	inode->i_ino = nfs_fattr_to_ino_t(fattr);
	nfs_read_inode(inode);
	nfs_fill_inode(inode, fattr);
	return inode;
}

/*
 * In NFSv3 we can have 64bit inode numbers. In order to support
 * this, and re-exported directories (also seen in NFSv2)
 * we are forced to allow 2 different inodes to have the same
 * i_ino.
 */
static int
nfs_find_actor(struct inode *inode, unsigned long ino, void *opaque)
{
	struct nfs_fattr *fattr = (struct nfs_fattr *)opaque;
	if (NFS_FSID(inode) != fattr->fsid)
		return 0;
	if (NFS_FILEID(inode) != fattr->fileid)
		return 0;
	if (inode->i_mode &&
	    (fattr->mode & S_IFMT) != (inode->i_mode & S_IFMT))
		return 0;
	if (is_bad_inode(inode))
		return 0;
	if (NFS_FLAGS(inode) & NFS_INO_STALE)
		return 0;
	return 1;
}

static int
nfs_inode_is_stale(struct inode *inode, struct nfs_fattr *fattr)
{
	int unhashed;
	int is_stale = 0;

	if (inode->i_mode &&
	    (fattr->mode & S_IFMT) != (inode->i_mode & S_IFMT))
		is_stale = 1;

	if (is_bad_inode(inode))
		is_stale = 1;

	/*
	 * If the inode seems stale, free up cached dentries.
	 */
	unhashed = nfs_free_dentries(inode);

	/* Assume we're holding an i_count
	 *
	 * NB: sockets sometimes have volatile file handles
	 *     don't invalidate their inodes even if all dentries are
	 *     unhashed.
	 */
	if (unhashed && inode->i_count == unhashed + 1
	    && !S_ISSOCK(inode->i_mode) && !S_ISFIFO(inode->i_mode))
		is_stale = 1;

	return is_stale;
}

/*
 * This is our own version of iget that looks up inodes by file handle
 * instead of inode number.  We use this technique instead of using
 * the vfs read_inode function because there is no way to pass the
 * file handle or current attributes into the read_inode function.
 *
 * We provide a special check for NetApp .snapshot directories to avoid
 * inode aliasing problems. All snapshot inodes are anonymous (unhashed).
 */
struct inode *
nfs_fhget(struct dentry *dentry, struct nfs_fh *fhandle,
				 struct nfs_fattr *fattr)
{
	struct super_block *sb = dentry->d_sb;

	dprintk("NFS: nfs_fhget(%s/%s fileid=%Lu)\n",
		dentry->d_parent->d_name.name, dentry->d_name.name,
		(long long) fattr->fileid);

	/* Install the file handle in the dentry */
	memcpy(NFS_FH(dentry), (u8*)fhandle, sizeof(*fhandle));

#ifdef CONFIG_NFS_SNAPSHOT
	/*
	 * Check for NetApp snapshot dentries, and get an 
	 * unhashed inode to avoid aliasing problems.
	 */
	if ((dentry->d_parent->d_inode->u.nfs_i.flags & NFS_IS_SNAPSHOT) ||
	    (dentry->d_name.len == 9 &&
	     memcmp(dentry->d_name.name, ".snapshot", 9) == 0)) {
		struct inode *inode = nfs_make_new_inode(sb, fattr);
		if (!inode)
			goto out;
		inode->u.nfs_i.flags |= NFS_IS_SNAPSHOT;
		dprintk("NFS: nfs_fhget(snapshot ino=%ld)\n", inode->i_ino);
	out:
		return inode;
	}
#endif
	return __nfs_fhget(sb, fattr);
}

/*
 * Look up the inode by super block and fattr->fileid.
 *
 * Note carefully the special handling of busy inodes (i_count > 1).
 * With the kernel 2.1.xx dcache all inodes except hard links must
 * have i_count == 1 after iget(). Otherwise, it indicates that the
 * server has reused a fileid (i_ino) and we have a stale inode.
 */
static struct inode *
__nfs_fhget(struct super_block *sb, struct nfs_fattr *fattr)
{
	struct inode	*inode = NULL;
	unsigned long	ino;

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		goto out_no_inode;

	ino = nfs_fattr_to_ino_t(fattr);

	while((inode = iget4(sb, ino, nfs_find_actor, fattr)) != NULL) {

		/*
		 * Check for busy inodes, and attempt to get rid of any
		 * unused local references. If successful, we release the
		 * inode and try again.
		 *
		 * Note that the busy test uses the values in the fattr,
		 * as the inode may have become a different object.
		 * (We can probably handle modes changes here, too.)
		 */
		if (!nfs_inode_is_stale(inode,fattr))
			break;

		dprintk("__nfs_fhget: inode %ld still busy, i_count=%d\n",
		       inode->i_ino, inode->i_count);
		/* Mark the inode as being stale */
		NFS_FLAGS(inode) |= NFS_INO_STALE;
		nfs_zap_caches(inode);
		iput(inode);
	}

	if (!inode)
		goto out_no_inode;

	nfs_fill_inode(inode, fattr);
	dprintk("NFS: __nfs_fhget(%x/%ld ct=%d)\n",
		inode->i_dev, inode->i_ino, inode->i_count);

out:
	return inode;

out_no_inode:
	printk(KERN_NOTICE "__nfs_fhget: iget failed\n");
	goto out;
}

int
nfs_notify_change(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = dentry->d_inode;
	struct nfs_fattr fattr;
	int              error;

	/*
	 * Make sure the inode is up-to-date.
	 */
	error = nfs_revalidate(dentry);
	if (error) {
#ifdef NFS_PARANOIA
		printk(KERN_DEBUG "nfs_notify_change: revalidate failed, error=%d\n", error);
#endif
		goto out;
	}

	if (!S_ISREG(inode->i_mode))
		attr->ia_valid &= ~ATTR_SIZE;

	error = nfs_wb_all(inode);
	if (error < 0)
		goto out;

	/* Now perform the setattr call */
	error = NFS_CALL(setattr, inode, (dentry, &fattr, attr));
	if (error || !(fattr.valid & NFS_ATTR_FATTR)) {
		nfs_zap_caches(inode);
		goto out;
	}
	/*
	 * If we changed the size or mtime, update the inode
	 * now to avoid invalidating the page cache.
	 */
	if (!(fattr.valid & NFS_ATTR_WCC)) {
		fattr.pre_size = NFS_CACHE_ISIZE(inode);
		fattr.pre_mtime = NFS_CACHE_MTIME(inode);
		fattr.pre_ctime = NFS_CACHE_CTIME(inode);
		fattr.valid |= NFS_ATTR_WCC;
	}
	error = nfs_refresh_inode(inode, &fattr);
out:
	return error;
}

int
nfs_update_atime(struct dentry *dentry)
{
	struct iattr attr;
	struct inode *inode = dentry->d_inode;

	nfs_revalidate(dentry);
	if (!inode || time_before(inode->i_atime,nfs_time_to_secs(NFS_CACHE_ATIME(inode))))
		return 0;

	attr.ia_valid = ATTR_ATIME|ATTR_ATIME_SET;
	attr.ia_atime = inode->i_atime;
	return nfs_notify_change(dentry, &attr);
}

/*
 * Wait for the inode to get unlocked.
 * (Used for NFS_INO_LOCKED and NFS_INO_REVALIDATING).
 */
int
nfs_wait_on_inode(struct inode *inode, int flag)
{
	struct rpc_clnt		*clnt = NFS_CLIENT(inode);
	int error;
	if (!(NFS_FLAGS(inode) & flag))
		return 0;
	inode->i_count++;
	error = nfs_wait_event(clnt, inode->i_wait, !(NFS_FLAGS(inode) & flag));
	iput(inode);
	return error;
}

/*
 * Externally visible revalidation function
 */
int
nfs_revalidate(struct dentry *dentry)
{
	return nfs_revalidate_inode(dentry);
}

static __inline__ struct nfs_file *nfs_file_alloc(void)
{
	struct nfs_file	*p;
	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (p) {
		memset(p, 0, sizeof(*p));
		p->magic = NFS_FILE_MAGIC;
	}
	return p;
}

static __inline__ void nfs_file_free(struct nfs_file *p)
{
	if (p->magic == NFS_FILE_MAGIC) {
		p->magic = 0;
		kfree(p);
	} else
		printk(KERN_ERR "NFS: extra file info corrupted!\n");
}

int nfs_open(struct inode *inode, struct file *filp)
{
	struct rpc_auth	*auth = NFS_CLIENT(inode)->cl_auth;
	struct nfs_file	*data;

	data = nfs_file_alloc();
	if (!data)
		return -ENOMEM;
	data->cred = rpcauth_lookupcred(auth, 0);
	filp->private_data = data;
	return 0;
}

int nfs_release(struct inode *inode, struct file *filp)
{
	struct nfs_file	*data = NFS_FILE(filp);
	struct rpc_auth	*auth = NFS_CLIENT(inode)->cl_auth;
	struct rpc_cred	*cred;

	cred = nfs_file_cred(filp);
	if (cred)
		rpcauth_releasecred(auth, cred);
	nfs_file_free(data);
	return 0;
}

/*
 * This function is called whenever some part of NFS notices that
 * the cached attributes have to be refreshed.
 */
int
__nfs_revalidate_inode(struct dentry *dentry)
{
	struct inode	*inode = dentry->d_inode;
	struct nfs_fattr fattr;
	int		 status = 0;

	dfprintk(PAGECACHE, "NFS: revalidating %s/%s, ino=%ld\n",
		dentry->d_parent->d_name.name, dentry->d_name.name,
		inode->i_ino);

	if (!inode || is_bad_inode(inode))
		return -ESTALE;

	while (NFS_REVALIDATING(inode)) {
		status = nfs_wait_on_inode(inode, NFS_INO_REVALIDATING);
		if (status < 0)
			return status;
		if (time_before(jiffies,NFS_READTIME(inode)+NFS_ATTRTIMEO(inode)))
			return 0;
	}
	NFS_FLAGS(inode) |= NFS_INO_REVALIDATING;

	status = NFS_CALL(getattr, inode, (dentry, &fattr));
	if (status) {
		int error;
		u32 *fh;
		struct dentry *dir = dentry->d_parent;
		struct nfs_fh fhandle;
		struct nfs_fattr dir_attr;

		dfprintk(PAGECACHE, "nfs_revalidate_inode: %s/%s getattr failed, ino=%ld, error=%d\n",
		       dentry->d_parent->d_name.name, dentry->d_name.name,
		       inode->i_ino, status);
		nfs_zap_caches(inode);

		if (status != -ESTALE)
			goto out;

		/*
		 * A "stale filehandle" error ... show the current fh
		 * and find out what the filehandle should be.
		 */
		fh = (u32 *) NFS_FH(dentry);
		dfprintk(PAGECACHE, "NFS: bad fh %08x%08x%08x%08x%08x%08x%08x%08x\n",
			fh[0],fh[1],fh[2],fh[3],fh[4],fh[5],fh[6],fh[7]);
		error = NFS_CALL(lookup, dir->d_inode, (dir, &dir_attr, 
					&dentry->d_name, &fhandle, &fattr));
		nfs_refresh_inode(dir->d_inode, &dir_attr);
		if (error) {
			dfprintk(PAGECACHE, "NFS: lookup failed, error=%d\n", error);
			goto out;
		}
		fh = (u32 *) &fhandle;
		dfprintk(PAGECACHE, "            %08x%08x%08x%08x%08x%08x%08x%08x\n",
			fh[0],fh[1],fh[2],fh[3],fh[4],fh[5],fh[6],fh[7]);
		if (!IS_ROOT(dentry) && !have_submounts(dentry))
			d_drop(dentry);
		goto out;
	}

	status = nfs_refresh_inode(inode, &fattr);
	if (status) {
		dfprintk(PAGECACHE, "nfs_revalidate_inode: %s/%s refresh failed, ino=%ld, error=%d\n",
			 dentry->d_parent->d_name.name, dentry->d_name.name,
			 inode->i_ino, status);
		goto out;
	}

	dfprintk(PAGECACHE, "NFS: %s/%s revalidation complete\n",
		dentry->d_parent->d_name.name, dentry->d_name.name);
out:
	NFS_FLAGS(inode) &= ~NFS_INO_REVALIDATING;
	wake_up(&inode->i_wait);
	return status;
}

/*
 * Many nfs protocol calls return the new file attributes after
 * an operation.  Here we update the inode to reflect the state
 * of the server's inode.
 *
 * If we have reason to believe that any data we cached has become
 * invalid, we schedule it to be flushed on the next occasion
 * (i.e. when nfs_revalidate_inode is called).
 *
 * The reason we don't do it here is because nfs_refresh_inode can
 * be called outside of the process context, e.g. from nfs_readpage_result,
 * which is invoked by rpciod.
 */
int
nfs_refresh_inode(struct inode *inode, struct nfs_fattr *fattr)
{
	off_t		new_size, new_isize;
	__u64		new_mtime;
	int		invalid = 0;
	int		error = -EIO;

	if (!inode || !fattr) {
		printk(KERN_ERR "nfs_refresh_inode: inode or fattr is NULL\n");
		goto out;
	}
	if (inode->i_mode == 0) {
		printk(KERN_ERR "nfs_refresh_inode: empty inode\n");
		goto out;
	}

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		goto out;

	if (is_bad_inode(inode))
		goto out;

	dfprintk(VFS, "NFS: refresh_inode(%x/%ld ct=%d info=0x%x)\n",
			inode->i_dev, inode->i_ino, inode->i_count,
			fattr->valid);


	if (NFS_FSID(inode) != fattr->fsid ||
	    NFS_FILEID(inode) != fattr->fileid) {
		printk(KERN_ERR "nfs_refresh_inode: inode number mismatch\n"
		       "expected (0x%lx%08lx/0x%lx%08lx), got (0x%lx%08lx/0x%lx%08lx)\n",
		       (unsigned long) (NFS_FSID(inode)>>32),
		       (unsigned long) (NFS_FSID(inode) & 0xFFFFFFFFUL),
		       (unsigned long) (NFS_FILEID(inode)>>32),
		       (unsigned long) (NFS_FILEID(inode) & 0xFFFFFFFFUL),
		       (unsigned long) (fattr->fsid >> 32),
		       (unsigned long) (fattr->fsid & 0xFFFFFFFFUL),
		       (unsigned long) (fattr->fileid >> 32),
		       (unsigned long) (fattr->fileid & 0xFFFFFFFFUL));
		goto out;
	}

	/*
	 * Make sure the inode's type hasn't changed.
	 */
	if ((inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT))
		goto out_changed;

 	new_mtime = fattr->mtime;
	new_size = fattr->size;
 	new_isize = nfs_size_to_off_t(fattr->size);

	error = 0;

	/*
	 * Update the read time so we don't revalidate too often.
	 */
	NFS_READTIME(inode) = jiffies;

	/*
	 * Note: NFS_CACHE_ISIZE(inode) reflects the state of the cache.
	 *       NOT inode->i_size!!!
	 */
	if (NFS_CACHE_ISIZE(inode) != new_size) {
#ifdef NFS_DEBUG_VERBOSE
		printk(KERN_DEBUG "NFS: isize change on %x/%ld\n", inode->i_dev, inode->i_ino);
#endif
		invalid = 1;
	}

	/*
	 * Note: we don't check inode->i_mtime since pipes etc.
	 *       can change this value in VFS without requiring a
	 *	 cache revalidation.
	 */
	if (NFS_CACHE_MTIME(inode) != new_mtime) {
#ifdef NFS_DEBUG_VERBOSE
		printk(KERN_DEBUG "NFS: mtime change on %x/%ld\n", inode->i_dev, inode->i_ino);
#endif
		invalid = 1;
	}

	/* Check Weak Cache Consistency data.
	 * If size and mtime match the pre-operation values, we can
	 * assume that any attribute changes were caused by our NFS
         * operation, so there's no need to invalidate the caches.
         */
        if ((fattr->valid & NFS_ATTR_WCC)
	    && NFS_CACHE_ISIZE(inode) == fattr->pre_size
	    && NFS_CACHE_MTIME(inode) == fattr->pre_mtime) {
		invalid = 0;
	}

	/*
	 * If we have pending writebacks, things can get
	 * messy.
	 */
	if (nfs_have_writebacks(inode) && new_isize < inode->i_size)
		new_isize = inode->i_size;

	NFS_CACHE_CTIME(inode) = fattr->ctime;
	inode->i_ctime = nfs_time_to_secs(fattr->ctime);
	/* If we've been messing around with atime, don't
	 * update it. Save the server value in NFS_CACHE_ATIME.
	 */
	NFS_CACHE_ATIME(inode) = fattr->atime;
	if (time_before(inode->i_atime, nfs_time_to_secs(fattr->atime)))
		inode->i_atime = nfs_time_to_secs(fattr->atime);

	NFS_CACHE_MTIME(inode) = new_mtime;
	inode->i_mtime = nfs_time_to_secs(new_mtime);

	NFS_CACHE_ISIZE(inode) = new_size;
	inode->i_size = new_isize;

	inode->i_mode = fattr->mode;
	inode->i_nlink = fattr->nlink;
	inode->i_uid = fattr->uid;
	inode->i_gid = fattr->gid;

	if (fattr->valid & NFS_ATTR_FATTR_V3) {
		/*
		 * report the blocks in 512byte units
		 */
		inode->i_blocks = nfs_calc_block_size(fattr->du.nfs3.used);
		inode->i_blksize = inode->i_sb->s_blocksize;
 	} else {
 		inode->i_blocks = fattr->du.nfs2.blocks;
		/*
		 * Imitate the Linux 2.2.13 behavior. The redhat
		 * linux 6.2 distribution contains a bad readdir/getdents
		 * in the glibc that cause programs to core dump when using
		 * our SGI server.
		 *
		 * --Lee; Wed Feb 21 16:58:49 MST 2001
		 *
		 * Dump this ASAP. Fix glibc, redhat, or something but
		 * this lie is not a wonderful solution.
		 */
		inode->i_blksize = inode->i_sb->s_blocksize;
 	}
 	inode->i_rdev = 0;
 	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
 		inode->i_rdev = to_kdev_t(fattr->rdev);
 
	/* Update attrtimeo value */
	if (!invalid && time_after(jiffies, NFS_ATTRTIMEO_UPDATE(inode)+NFS_ATTRTIMEO(inode))) {
		if ((NFS_ATTRTIMEO(inode) <<= 1) > NFS_MAXATTRTIMEO(inode))
			NFS_ATTRTIMEO(inode) = NFS_MAXATTRTIMEO(inode);
		NFS_ATTRTIMEO_UPDATE(inode) = jiffies;
	}

	if (invalid)
		nfs_zap_caches(inode);

out:
	return error;

out_changed:
	/*
	 * Big trouble! The inode has become a different object.
	 */
#ifdef NFS_PARANOIA
	printk(KERN_DEBUG "nfs_refresh_inode: inode %ld mode changed, %07o to %07o\n",
	       inode->i_ino, inode->i_mode, fattr->mode);
#endif
	/*
	 * No need to worry about unhashing the dentry, as the
	 * lookup validation will know that the inode is bad.
	 * (But we fall through to invalidate the caches.)
	 */
	nfs_invalidate_inode(inode);
	goto out;
}

/*
 * File system information
 */
static struct file_system_type nfs_fs_type = {
	"nfs",
	0 /* FS_NO_DCACHE - this doesn't work right now*/,
	nfs_read_super,
	NULL
};

/*
 * Initialize NFS
 */
int
init_nfs_fs(void)
{
#ifdef CONFIG_PROC_FS
	rpc_proc_register(&nfs_rpcstat);
#endif
        return register_filesystem(&nfs_fs_type);
}

/*
 * Every kernel module contains stuff like this.
 */
#ifdef MODULE

EXPORT_NO_SYMBOLS;
/* Not quite true; I just maintain it */
MODULE_AUTHOR("Olaf Kirch <okir@monad.swb.de>");

int
init_module(void)
{
	return init_nfs_fs();
}

void
cleanup_module(void)
{
#ifdef CONFIG_PROC_FS
	rpc_proc_unregister("nfs");
#endif
	unregister_filesystem(&nfs_fs_type);
}
#endif
