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
 * $Id: enfs_prot.h,v 1.1 2000/02/15 23:34:41 lward Exp $
 */

#ifndef _STDIN_
#define	_STDIN_

#include "rpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "cmn.h"
#include "rpcsvc/nfs_prot.h"
#define	ENFS_PORT 19049
#define	ENFS_MAXDATA 65536

struct ereadargs {
	nfs_fh file;
	u_int offset;
	u_int count;
	u_int totalcount;
};
typedef struct ereadargs ereadargs;

struct ereadokres {
	fattr attributes;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct ereadokres ereadokres;

struct ereadres {
	nfsstat status;
	union {
		ereadokres reply;
	} ereadres_u;
};
typedef struct ereadres ereadres;

struct ewriteargs {
	nfs_fh file;
	u_int beginoffset;
	u_int offset;
	u_int totalcount;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct ewriteargs ewriteargs;

#define	ENFS_PROGRAM ((unsigned long)(100003))
#define	ENFS_VERSION ((unsigned long)(2))

#if defined(__STDC__) || defined(__cplusplus)
#define	ENFSPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat enfsproc_null_2(void *, void *, CLIENT *)
 ALIAS_TO("nfsproc_null_2");
extern  bool_t enfsproc_null_2_svc(void *, void *, struct svc_req *)
 ALIAS_TO("nfsproc_null_2_svc");
#define	ENFSPROC_GETATTR ((unsigned long)(1))
extern  enum clnt_stat enfsproc_getattr_2(nfs_fh *, attrstat *, CLIENT *)
 ALIAS_TO("nfsproc_getattr_2");
extern  bool_t enfsproc_getattr_2_svc(nfs_fh *, attrstat *, struct svc_req *)
 ALIAS_TO("nfsproc_getattr_2_svc");
#define	ENFSPROC_SETATTR ((unsigned long)(2))
extern  enum clnt_stat enfsproc_setattr_2(sattrargs *, attrstat *, CLIENT *)
 ALIAS_TO("nfsproc_setattr_2");
extern  bool_t enfsproc_setattr_2_svc(sattrargs *, attrstat *, struct svc_req *)
 ALIAS_TO("nfsproc_setattr_2_svc");
#define	ENFSPROC_ROOT ((unsigned long)(3))
extern  enum clnt_stat enfsproc_root_2(void *, void *, CLIENT *)
 ALIAS_TO("nfsproc_root_2");
extern  bool_t enfsproc_root_2_svc(void *, void *, struct svc_req *)
 ALIAS_TO("nfsproc_root_2_svc");
#define	ENFSPROC_LOOKUP ((unsigned long)(4))
extern  enum clnt_stat enfsproc_lookup_2(diropargs *, diropres *, CLIENT *)
 ALIAS_TO("nfsproc_lookup_2");
extern  bool_t enfsproc_lookup_2_svc(diropargs *, diropres *, struct svc_req *)
 ALIAS_TO("nfsproc_lookup_2_svc");
#define	ENFSPROC_READLINK ((unsigned long)(5))
extern  enum clnt_stat enfsproc_readlink_2(nfs_fh *, readlinkres *, CLIENT *)
 ALIAS_TO("nfsproc_readlink_2");
extern  bool_t enfsproc_readlink_2_svc(nfs_fh *, readlinkres *, struct svc_req *)
 ALIAS_TO("nfsproc_readlink_2_svc");
#define	ENFSPROC_READ ((unsigned long)(6))
extern  enum clnt_stat enfsproc_read_2(ereadargs *, ereadres *, CLIENT *)
 ALIAS_TO("nfsproc_read_2");
extern  bool_t enfsproc_read_2_svc(ereadargs *, ereadres *, struct svc_req *)
 ALIAS_TO("nfsproc_read_2_svc");
#define	ENFSPROC_WRITECACHE ((unsigned long)(7))
extern  enum clnt_stat enfsproc_writecache_2(void *, void *, CLIENT *)
 ALIAS_TO("nfsproc_writecache_2");
extern  bool_t enfsproc_writecache_2_svc(void *, void *, struct svc_req *)
 ALIAS_TO("nfsproc_writecache_2_svc");
#define	ENFSPROC_WRITE ((unsigned long)(8))
extern  enum clnt_stat enfsproc_write_2(ewriteargs *, attrstat *, CLIENT *)
 ALIAS_TO("nfsproc_write_2");
extern  bool_t enfsproc_write_2_svc(ewriteargs *, attrstat *, struct svc_req *)
 ALIAS_TO("nfsproc_write_2_svc");
#define	ENFSPROC_CREATE ((unsigned long)(9))
extern  enum clnt_stat enfsproc_create_2(createargs *, diropres *, CLIENT *)
 ALIAS_TO("nfsproc_create_2");
extern  bool_t enfsproc_create_2_svc(createargs *, diropres *, struct svc_req *)
 ALIAS_TO("nfsproc_create_2_svc");
#define	ENFSPROC_REMOVE ((unsigned long)(10))
extern  enum clnt_stat enfsproc_remove_2(diropargs *, nfsstat *, CLIENT *)
 ALIAS_TO("nfsproc_remove_2");
extern  bool_t enfsproc_remove_2_svc(diropargs *, nfsstat *, struct svc_req *)
 ALIAS_TO("nfsproc_remove_2_svc");
#define	ENFSPROC_RENAME ((unsigned long)(11))
extern  enum clnt_stat enfsproc_rename_2(renameargs *, nfsstat *, CLIENT *)
 ALIAS_TO("nfsproc_rename_2");
extern  bool_t enfsproc_rename_2_svc(renameargs *, nfsstat *, struct svc_req *)
 ALIAS_TO("nfsproc_rename_2_svc");
#define	ENFSPROC_LINK ((unsigned long)(12))
extern  enum clnt_stat enfsproc_link_2(linkargs *, nfsstat *, CLIENT *)
 ALIAS_TO("nfsproc_link_2");
extern  bool_t enfsproc_link_2_svc(linkargs *, nfsstat *, struct svc_req *)
 ALIAS_TO("nfsproc_link_2_svc");
#define	ENFSPROC_SYMLINK ((unsigned long)(13))
extern  enum clnt_stat enfsproc_symlink_2(symlinkargs *, nfsstat *, CLIENT *)
 ALIAS_TO("nfsproc_symlink_2");
extern  bool_t enfsproc_symlink_2_svc(symlinkargs *, nfsstat *, struct svc_req *)
 ALIAS_TO("nfsproc_symlink_2_svc");
#define	ENFSPROC_MKDIR ((unsigned long)(14))
extern  enum clnt_stat enfsproc_mkdir_2(createargs *, diropres *, CLIENT *)
 ALIAS_TO("nfsproc_mkdir_2");
extern  bool_t enfsproc_mkdir_2_svc(createargs *, diropres *, struct svc_req *)
 ALIAS_TO("nfsproc_mkdir_2_svc");
#define	ENFSPROC_RMDIR ((unsigned long)(15))
extern  enum clnt_stat enfsproc_rmdir_2(diropargs *, nfsstat *, CLIENT *)
 ALIAS_TO("nfsproc_rmdir_2");
extern  bool_t enfsproc_rmdir_2_svc(diropargs *, nfsstat *, struct svc_req *)
 ALIAS_TO("nfsproc_rmdir_2_svc");
#define	ENFSPROC_READDIR ((unsigned long)(16))
extern  enum clnt_stat enfsproc_readdir_2(readdirargs *, readdirres *, CLIENT *)
 ALIAS_TO("nfsproc_readdir_2");
extern  bool_t enfsproc_readdir_2_svc(readdirargs *, readdirres *, struct svc_req *)
 ALIAS_TO("nfsproc_readdir_2_svc");
#define	ENFSPROC_STATFS ((unsigned long)(17))
extern  enum clnt_stat enfsproc_statfs_2(nfs_fh *, statfsres *, CLIENT *)
 ALIAS_TO("nfsproc_statfs_2");
extern  bool_t enfsproc_statfs_2_svc(nfs_fh *, statfsres *, struct svc_req *)
 ALIAS_TO("nfsproc_statfs_2_svc");
#define	ENFSPROC_SYNCHRONIZE ((unsigned long)(18))
extern  enum clnt_stat enfsproc_synchronize_2(nfs_fh *, nfsstat *, CLIENT *);
extern  bool_t enfsproc_synchronize_2_svc(nfs_fh *, nfsstat *, struct svc_req *);
extern int enfs_program_2_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	ENFSPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat enfsproc_null_2()
 ALIAS_TO("nfsproc_null_2");
extern  bool_t enfsproc_null_2_svc()
 ALIAS_TO("nfsproc_null_2_svc");
#define	ENFSPROC_GETATTR ((unsigned long)(1))
extern  enum clnt_stat enfsproc_getattr_2()
 ALIAS_TO("nfsproc_getattr_2");
extern  bool_t enfsproc_getattr_2_svc()
 ALIAS_TO("nfsproc_getattr_2_svc");
#define	ENFSPROC_SETATTR ((unsigned long)(2))
extern  enum clnt_stat enfsproc_setattr_2()
 ALIAS_TO("nfsproc_setattr_2");
extern  bool_t enfsproc_setattr_2_svc()
 ALIAS_TO("nfsproc_setattr_2_svc");
#define	ENFSPROC_ROOT ((unsigned long)(3))
extern  enum clnt_stat enfsproc_root_2()
 ALIAS_TO("nfsproc_root_2");
extern  bool_t enfsproc_root_2_svc()
 ALIAS_TO("nfsproc_root_2_svc");
#define	ENFSPROC_LOOKUP ((unsigned long)(4))
extern  enum clnt_stat enfsproc_lookup_2()
 ALIAS_TO("nfsproc_lookup_2");
extern  bool_t enfsproc_lookup_2_svc()
 ALIAS_TO("nfsproc_lookup_2_svc");
#define	ENFSPROC_READLINK ((unsigned long)(5))
extern  enum clnt_stat enfsproc_readlink_2()
 ALIAS_TO("nfsproc_readlink_2");
extern  bool_t enfsproc_readlink_2_svc()
 ALIAS_TO("nfsproc_readlink_2_svc");
#define	ENFSPROC_READ ((unsigned long)(6))
extern  enum clnt_stat enfsproc_read_2()
 ALIAS_TO("nfsproc_read_2");
extern  bool_t enfsproc_read_2_svc()
 ALIAS_TO("nfsproc_read_2_svc");
#define	ENFSPROC_WRITECACHE ((unsigned long)(7))
extern  enum clnt_stat enfsproc_writecache_2()
 ALIAS_TO("nfsproc_writecache_2");
extern  bool_t enfsproc_writecache_2_svc()
 ALIAS_TO("nfsproc_writecache_2_svc");
#define	ENFSPROC_WRITE ((unsigned long)(8))
extern  enum clnt_stat enfsproc_write_2()
 ALIAS_TO("nfsproc_write_2");
extern  bool_t enfsproc_write_2_svc()
 ALIAS_TO("nfsproc_write_2_svc");
#define	ENFSPROC_CREATE ((unsigned long)(9))
extern  enum clnt_stat enfsproc_create_2()
 ALIAS_TO("nfsproc_create_2");
extern  bool_t enfsproc_create_2_svc()
 ALIAS_TO("nfsproc_create_2_svc");
#define	ENFSPROC_REMOVE ((unsigned long)(10))
extern  enum clnt_stat enfsproc_remove_2()
 ALIAS_TO("nfsproc_remove_2");
extern  bool_t enfsproc_remove_2_svc()
 ALIAS_TO("nfsproc_remove_2_svc");
#define	ENFSPROC_RENAME ((unsigned long)(11))
extern  enum clnt_stat enfsproc_rename_2()
 ALIAS_TO("nfsproc_rename_2");
extern  bool_t enfsproc_rename_2_svc()
 ALIAS_TO("nfsproc_rename_2_svc");
#define	ENFSPROC_LINK ((unsigned long)(12))
extern  enum clnt_stat enfsproc_link_2()
 ALIAS_TO("nfsproc_link_2");
extern  bool_t enfsproc_link_2_svc()
 ALIAS_TO("nfsproc_link_2_svc");
#define	ENFSPROC_SYMLINK ((unsigned long)(13))
extern  enum clnt_stat enfsproc_symlink_2()
 ALIAS_TO("nfsproc_symlink_2");
extern  bool_t enfsproc_symlink_2_svc()
 ALIAS_TO("nfsproc_symlink_2_svc");
#define	ENFSPROC_MKDIR ((unsigned long)(14))
extern  enum clnt_stat enfsproc_mkdir_2()
 ALIAS_TO("nfsproc_mkdir_2");
extern  bool_t enfsproc_mkdir_2_svc()
 ALIAS_TO("nfsproc_mkdir_2_svc");
#define	ENFSPROC_RMDIR ((unsigned long)(15))
extern  enum clnt_stat enfsproc_rmdir_2()
 ALIAS_TO("nfsproc_rmdir_2");
extern  bool_t enfsproc_rmdir_2_svc()
 ALIAS_TO("nfsproc_rmdir_2_svc");
#define	ENFSPROC_READDIR ((unsigned long)(16))
extern  enum clnt_stat enfsproc_readdir_2()
 ALIAS_TO("nfsproc_readdir_2");
extern  bool_t enfsproc_readdir_2_svc()
 ALIAS_TO("nfsproc_readdir_2_svc");
#define	ENFSPROC_STATFS ((unsigned long)(17))
extern  enum clnt_stat enfsproc_statfs_2()
 ALIAS_TO("nfsproc_statfs_2");
extern  bool_t enfsproc_statfs_2_svc()
 ALIAS_TO("nfsproc_statfs_2_svc");
#define	ENFSPROC_SYNCHRONIZE ((unsigned long)(18))
extern  enum clnt_stat enfsproc_synchronize_2();
extern  bool_t enfsproc_synchronize_2_svc();
extern int enfs_program_2_freeresult();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_ereadargs(XDR *, ereadargs*);
extern  bool_t xdr_ereadokres(XDR *, ereadokres*);
extern  bool_t xdr_ereadres(XDR *, ereadres*);
extern  bool_t xdr_ewriteargs(XDR *, ewriteargs*);

#else /* K&R C */
extern bool_t xdr_ereadargs();
extern bool_t xdr_ereadokres();
extern bool_t xdr_ereadres();
extern bool_t xdr_ewriteargs();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_STDIN_ */
