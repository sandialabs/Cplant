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
 * $Id: nfs_prot.h,v 0.1 1999/08/17 05:42:21 lee Stab $
 */
#ifndef _NFS_PROT_
#define	_NFS_PROT_

#include "rpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	NFS_PORT 2049
#define	NFS_MAXDATA 8192
#define	NFS_MAXPATHLEN 1024
#define	NFS_MAXNAMLEN 255
#define	NFS_FHSIZE 32
#define	NFS_COOKIESIZE 4
#define	NFS_FIFO_DEV -1
#define	NFSMODE_FMT 0170000
#define	NFSMODE_DIR 0040000
#define	NFSMODE_CHR 0020000
#define	NFSMODE_BLK 0060000
#define	NFSMODE_REG 0100000
#define	NFSMODE_LNK 0120000
#define	NFSMODE_SOCK 0140000
#define	NFSMODE_FIFO 0010000

enum nfsstat {
	NFS_OK = 0,
	NFSERR_PERM = 1,
	NFSERR_NOENT = 2,
	NFSERR_IO = 5,
	NFSERR_NXIO = 6,
	NFSERR_ACCES = 13,
	NFSERR_EXIST = 17,
	NFSERR_NODEV = 19,
	NFSERR_NOTDIR = 20,
	NFSERR_ISDIR = 21,
	NFSERR_FBIG = 27,
	NFSERR_NOSPC = 28,
	NFSERR_ROFS = 30,
	NFSERR_NAMETOOLONG = 63,
	NFSERR_NOTEMPTY = 66,
	NFSERR_DQUOT = 69,
	NFSERR_STALE = 70,
	NFSERR_WFLUSH = 99
};
typedef enum nfsstat nfsstat;

enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5,
	NFSOCK = 6,
	NFBAD = 7,
	NFFIFO = 8
};
typedef enum ftype ftype;

struct nfs_fh {
	char data[NFS_FHSIZE];
};
typedef struct nfs_fh nfs_fh;

struct nfstime {
	u_int seconds;
	u_int useconds;
};
typedef struct nfstime nfstime;

struct fattr {
	ftype type;
	u_int mode;
	u_int nlink;
	u_int uid;
	u_int gid;
	u_int size;
	u_int blocksize;
	u_int rdev;
	u_int blocks;
	u_int fsid;
	u_int fileid;
	nfstime atime;
	nfstime mtime;
	nfstime ctime;
};
typedef struct fattr fattr;

struct sattr {
	u_int mode;
	u_int uid;
	u_int gid;
	u_int size;
	nfstime atime;
	nfstime mtime;
};
typedef struct sattr sattr;

typedef char *filename;

typedef char *nfspath;

struct attrstat {
	nfsstat status;
	union {
		fattr attributes;
	} attrstat_u;
};
typedef struct attrstat attrstat;

struct sattrargs {
	nfs_fh file;
	sattr attributes;
};
typedef struct sattrargs sattrargs;

struct diropargs {
	nfs_fh dir;
	filename name;
};
typedef struct diropargs diropargs;

struct diropokres {
	nfs_fh file;
	fattr attributes;
};
typedef struct diropokres diropokres;

struct diropres {
	nfsstat status;
	union {
		diropokres diropres;
	} diropres_u;
};
typedef struct diropres diropres;

struct readlinkres {
	nfsstat status;
	union {
		nfspath data;
	} readlinkres_u;
};
typedef struct readlinkres readlinkres;

struct readargs {
	nfs_fh file;
	u_int offset;
	u_int count;
	u_int totalcount;
};
typedef struct readargs readargs;

struct readokres {
	fattr attributes;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct readokres readokres;

struct readres {
	nfsstat status;
	union {
		readokres reply;
	} readres_u;
};
typedef struct readres readres;

struct writeargs {
	nfs_fh file;
	u_int beginoffset;
	u_int offset;
	u_int totalcount;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct writeargs writeargs;

struct createargs {
	diropargs where;
	sattr attributes;
};
typedef struct createargs createargs;

struct renameargs {
	diropargs from;
	diropargs to;
};
typedef struct renameargs renameargs;

struct linkargs {
	nfs_fh from;
	diropargs to;
};
typedef struct linkargs linkargs;

struct symlinkargs {
	diropargs from;
	nfspath to;
	sattr attributes;
};
typedef struct symlinkargs symlinkargs;

typedef char nfscookie[NFS_COOKIESIZE];

struct readdirargs {
	nfs_fh dir;
	nfscookie cookie;
	u_int count;
};
typedef struct readdirargs readdirargs;

struct entry {
	u_int fileid;
	filename name;
	nfscookie cookie;
	struct entry *nextentry;
};
typedef struct entry entry;

struct dirlist {
	entry *entries;
	bool_t eof;
};
typedef struct dirlist dirlist;

struct readdirres {
	nfsstat status;
	union {
		dirlist reply;
	} readdirres_u;
};
typedef struct readdirres readdirres;

struct statfsokres {
	u_int tsize;
	u_int bsize;
	u_int blocks;
	u_int bfree;
	u_int bavail;
};
typedef struct statfsokres statfsokres;

struct statfsres {
	nfsstat status;
	union {
		statfsokres reply;
	} statfsres_u;
};
typedef struct statfsres statfsres;

#define	NFS_PROGRAM ((unsigned long)(100003))
#define	NFS_VERSION ((unsigned long)(2))

#if defined(__STDC__) || defined(__cplusplus)
#define	NFSPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat nfsproc_null_2(void *, void *, CLIENT *);
extern  bool_t nfsproc_null_2_svc(void *, void *, struct svc_req *);
#define	NFSPROC_GETATTR ((unsigned long)(1))
extern  enum clnt_stat nfsproc_getattr_2(nfs_fh *, attrstat *, CLIENT *);
extern  bool_t nfsproc_getattr_2_svc(nfs_fh *, attrstat *, struct svc_req *);
#define	NFSPROC_SETATTR ((unsigned long)(2))
extern  enum clnt_stat nfsproc_setattr_2(sattrargs *, attrstat *, CLIENT *);
extern  bool_t nfsproc_setattr_2_svc(sattrargs *, attrstat *, struct svc_req *);
#define	NFSPROC_ROOT ((unsigned long)(3))
extern  enum clnt_stat nfsproc_root_2(void *, void *, CLIENT *);
extern  bool_t nfsproc_root_2_svc(void *, void *, struct svc_req *);
#define	NFSPROC_LOOKUP ((unsigned long)(4))
extern  enum clnt_stat nfsproc_lookup_2(diropargs *, diropres *, CLIENT *);
extern  bool_t nfsproc_lookup_2_svc(diropargs *, diropres *, struct svc_req *);
#define	NFSPROC_READLINK ((unsigned long)(5))
extern  enum clnt_stat nfsproc_readlink_2(nfs_fh *, readlinkres *, CLIENT *);
extern  bool_t nfsproc_readlink_2_svc(nfs_fh *, readlinkres *, struct svc_req *);
#define	NFSPROC_READ ((unsigned long)(6))
extern  enum clnt_stat nfsproc_read_2(readargs *, readres *, CLIENT *);
extern  bool_t nfsproc_read_2_svc(readargs *, readres *, struct svc_req *);
#define	NFSPROC_WRITECACHE ((unsigned long)(7))
extern  enum clnt_stat nfsproc_writecache_2(void *, void *, CLIENT *);
extern  bool_t nfsproc_writecache_2_svc(void *, void *, struct svc_req *);
#define	NFSPROC_WRITE ((unsigned long)(8))
extern  enum clnt_stat nfsproc_write_2(writeargs *, attrstat *, CLIENT *);
extern  bool_t nfsproc_write_2_svc(writeargs *, attrstat *, struct svc_req *);
#define	NFSPROC_CREATE ((unsigned long)(9))
extern  enum clnt_stat nfsproc_create_2(createargs *, diropres *, CLIENT *);
extern  bool_t nfsproc_create_2_svc(createargs *, diropres *, struct svc_req *);
#define	NFSPROC_REMOVE ((unsigned long)(10))
extern  enum clnt_stat nfsproc_remove_2(diropargs *, nfsstat *, CLIENT *);
extern  bool_t nfsproc_remove_2_svc(diropargs *, nfsstat *, struct svc_req *);
#define	NFSPROC_RENAME ((unsigned long)(11))
extern  enum clnt_stat nfsproc_rename_2(renameargs *, nfsstat *, CLIENT *);
extern  bool_t nfsproc_rename_2_svc(renameargs *, nfsstat *, struct svc_req *);
#define	NFSPROC_LINK ((unsigned long)(12))
extern  enum clnt_stat nfsproc_link_2(linkargs *, nfsstat *, CLIENT *);
extern  bool_t nfsproc_link_2_svc(linkargs *, nfsstat *, struct svc_req *);
#define	NFSPROC_SYMLINK ((unsigned long)(13))
extern  enum clnt_stat nfsproc_symlink_2(symlinkargs *, nfsstat *, CLIENT *);
extern  bool_t nfsproc_symlink_2_svc(symlinkargs *, nfsstat *, struct svc_req *);
#define	NFSPROC_MKDIR ((unsigned long)(14))
extern  enum clnt_stat nfsproc_mkdir_2(createargs *, diropres *, CLIENT *);
extern  bool_t nfsproc_mkdir_2_svc(createargs *, diropres *, struct svc_req *);
#define	NFSPROC_RMDIR ((unsigned long)(15))
extern  enum clnt_stat nfsproc_rmdir_2(diropargs *, nfsstat *, CLIENT *);
extern  bool_t nfsproc_rmdir_2_svc(diropargs *, nfsstat *, struct svc_req *);
#define	NFSPROC_READDIR ((unsigned long)(16))
extern  enum clnt_stat nfsproc_readdir_2(readdirargs *, readdirres *, CLIENT *);
extern  bool_t nfsproc_readdir_2_svc(readdirargs *, readdirres *, struct svc_req *);
#define	NFSPROC_STATFS ((unsigned long)(17))
extern  enum clnt_stat nfsproc_statfs_2(nfs_fh *, statfsres *, CLIENT *);
extern  bool_t nfsproc_statfs_2_svc(nfs_fh *, statfsres *, struct svc_req *);
extern int nfs_program_2_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	NFSPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat nfsproc_null_2();
extern  bool_t nfsproc_null_2_svc();
#define	NFSPROC_GETATTR ((unsigned long)(1))
extern  enum clnt_stat nfsproc_getattr_2();
extern  bool_t nfsproc_getattr_2_svc();
#define	NFSPROC_SETATTR ((unsigned long)(2))
extern  enum clnt_stat nfsproc_setattr_2();
extern  bool_t nfsproc_setattr_2_svc();
#define	NFSPROC_ROOT ((unsigned long)(3))
extern  enum clnt_stat nfsproc_root_2();
extern  bool_t nfsproc_root_2_svc();
#define	NFSPROC_LOOKUP ((unsigned long)(4))
extern  enum clnt_stat nfsproc_lookup_2();
extern  bool_t nfsproc_lookup_2_svc();
#define	NFSPROC_READLINK ((unsigned long)(5))
extern  enum clnt_stat nfsproc_readlink_2();
extern  bool_t nfsproc_readlink_2_svc();
#define	NFSPROC_READ ((unsigned long)(6))
extern  enum clnt_stat nfsproc_read_2();
extern  bool_t nfsproc_read_2_svc();
#define	NFSPROC_WRITECACHE ((unsigned long)(7))
extern  enum clnt_stat nfsproc_writecache_2();
extern  bool_t nfsproc_writecache_2_svc();
#define	NFSPROC_WRITE ((unsigned long)(8))
extern  enum clnt_stat nfsproc_write_2();
extern  bool_t nfsproc_write_2_svc();
#define	NFSPROC_CREATE ((unsigned long)(9))
extern  enum clnt_stat nfsproc_create_2();
extern  bool_t nfsproc_create_2_svc();
#define	NFSPROC_REMOVE ((unsigned long)(10))
extern  enum clnt_stat nfsproc_remove_2();
extern  bool_t nfsproc_remove_2_svc();
#define	NFSPROC_RENAME ((unsigned long)(11))
extern  enum clnt_stat nfsproc_rename_2();
extern  bool_t nfsproc_rename_2_svc();
#define	NFSPROC_LINK ((unsigned long)(12))
extern  enum clnt_stat nfsproc_link_2();
extern  bool_t nfsproc_link_2_svc();
#define	NFSPROC_SYMLINK ((unsigned long)(13))
extern  enum clnt_stat nfsproc_symlink_2();
extern  bool_t nfsproc_symlink_2_svc();
#define	NFSPROC_MKDIR ((unsigned long)(14))
extern  enum clnt_stat nfsproc_mkdir_2();
extern  bool_t nfsproc_mkdir_2_svc();
#define	NFSPROC_RMDIR ((unsigned long)(15))
extern  enum clnt_stat nfsproc_rmdir_2();
extern  bool_t nfsproc_rmdir_2_svc();
#define	NFSPROC_READDIR ((unsigned long)(16))
extern  enum clnt_stat nfsproc_readdir_2();
extern  bool_t nfsproc_readdir_2_svc();
#define	NFSPROC_STATFS ((unsigned long)(17))
extern  enum clnt_stat nfsproc_statfs_2();
extern  bool_t nfsproc_statfs_2_svc();
extern int nfs_program_2_freeresult();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_nfsstat(XDR *, nfsstat*);
extern  bool_t xdr_ftype(XDR *, ftype*);
extern  bool_t xdr_nfs_fh(XDR *, nfs_fh*);
extern  bool_t xdr_nfstime(XDR *, nfstime*);
extern  bool_t xdr_fattr(XDR *, fattr*);
extern  bool_t xdr_sattr(XDR *, sattr*);
extern  bool_t xdr_filename(XDR *, filename*);
extern  bool_t xdr_nfspath(XDR *, nfspath*);
extern  bool_t xdr_attrstat(XDR *, attrstat*);
extern  bool_t xdr_sattrargs(XDR *, sattrargs*);
extern  bool_t xdr_diropargs(XDR *, diropargs*);
extern  bool_t xdr_diropokres(XDR *, diropokres*);
extern  bool_t xdr_diropres(XDR *, diropres*);
extern  bool_t xdr_readlinkres(XDR *, readlinkres*);
extern  bool_t xdr_readargs(XDR *, readargs*);
extern  bool_t xdr_readokres(XDR *, readokres*);
extern  bool_t xdr_readres(XDR *, readres*);
extern  bool_t xdr_writeargs(XDR *, writeargs*);
extern  bool_t xdr_createargs(XDR *, createargs*);
extern  bool_t xdr_renameargs(XDR *, renameargs*);
extern  bool_t xdr_linkargs(XDR *, linkargs*);
extern  bool_t xdr_symlinkargs(XDR *, symlinkargs*);
extern  bool_t xdr_nfscookie(XDR *, nfscookie);
extern  bool_t xdr_readdirargs(XDR *, readdirargs*);
extern  bool_t xdr_entry(XDR *, entry*);
extern  bool_t xdr_dirlist(XDR *, dirlist*);
extern  bool_t xdr_readdirres(XDR *, readdirres*);
extern  bool_t xdr_statfsokres(XDR *, statfsokres*);
extern  bool_t xdr_statfsres(XDR *, statfsres*);

#else /* K&R C */
extern bool_t xdr_nfsstat();
extern bool_t xdr_ftype();
extern bool_t xdr_nfs_fh();
extern bool_t xdr_nfstime();
extern bool_t xdr_fattr();
extern bool_t xdr_sattr();
extern bool_t xdr_filename();
extern bool_t xdr_nfspath();
extern bool_t xdr_attrstat();
extern bool_t xdr_sattrargs();
extern bool_t xdr_diropargs();
extern bool_t xdr_diropokres();
extern bool_t xdr_diropres();
extern bool_t xdr_readlinkres();
extern bool_t xdr_readargs();
extern bool_t xdr_readokres();
extern bool_t xdr_readres();
extern bool_t xdr_writeargs();
extern bool_t xdr_createargs();
extern bool_t xdr_renameargs();
extern bool_t xdr_linkargs();
extern bool_t xdr_symlinkargs();
extern bool_t xdr_nfscookie();
extern bool_t xdr_readdirargs();
extern bool_t xdr_entry();
extern bool_t xdr_dirlist();
extern bool_t xdr_readdirres();
extern bool_t xdr_statfsokres();
extern bool_t xdr_statfsres();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_NFS_PROT_ */
