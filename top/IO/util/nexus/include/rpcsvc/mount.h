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
 * $Id: mount.h,v 0.1 1999/08/17 05:42:21 lee Stab $
 */
#ifndef _MOUNT_
#define	_MOUNT_

#include "rpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	MNTPATHLEN 1024
#define	MNTNAMLEN 255
#define	FHSIZE 32

typedef char fhandle[FHSIZE];

struct fhstatus {
	u_int fhs_status;
	union {
		fhandle fhs_fhandle;
	} fhstatus_u;
};
typedef struct fhstatus fhstatus;

typedef char *dirpath;

typedef char *name;

typedef struct mountbody *mountlist;

struct mountbody {
	name ml_hostname;
	dirpath ml_directory;
	mountlist ml_next;
};
typedef struct mountbody mountbody;

typedef struct groupnode *groups;

struct groupnode {
	name gr_name;
	groups gr_next;
};
typedef struct groupnode groupnode;

typedef struct exportnode *exports;

struct exportnode {
	dirpath ex_dir;
	groups ex_groups;
	exports ex_next;
};
typedef struct exportnode exportnode;

#define	MOUNTPROG ((unsigned long)(100005))
#define	MOUNTVERS ((unsigned long)(1))

#if defined(__STDC__) || defined(__cplusplus)
#define	MOUNTPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat mountproc_null_1(void *, void *, CLIENT *);
extern  bool_t mountproc_null_1_svc(void *, void *, struct svc_req *);
#define	MOUNTPROC_MNT ((unsigned long)(1))
extern  enum clnt_stat mountproc_mnt_1(dirpath *, fhstatus *, CLIENT *);
extern  bool_t mountproc_mnt_1_svc(dirpath *, fhstatus *, struct svc_req *);
#define	MOUNTPROC_DUMP ((unsigned long)(2))
extern  enum clnt_stat mountproc_dump_1(void *, mountlist *, CLIENT *);
extern  bool_t mountproc_dump_1_svc(void *, mountlist *, struct svc_req *);
#define	MOUNTPROC_UMNT ((unsigned long)(3))
extern  enum clnt_stat mountproc_umnt_1(dirpath *, void *, CLIENT *);
extern  bool_t mountproc_umnt_1_svc(dirpath *, void *, struct svc_req *);
#define	MOUNTPROC_UMNTALL ((unsigned long)(4))
extern  enum clnt_stat mountproc_umntall_1(void *, void *, CLIENT *);
extern  bool_t mountproc_umntall_1_svc(void *, void *, struct svc_req *);
#define	MOUNTPROC_EXPORT ((unsigned long)(5))
extern  enum clnt_stat mountproc_export_1(void *, exports *, CLIENT *);
extern  bool_t mountproc_export_1_svc(void *, exports *, struct svc_req *);
#define	MOUNTPROC_EXPORTALL ((unsigned long)(6))
extern  enum clnt_stat mountproc_exportall_1(void *, exports *, CLIENT *);
extern  bool_t mountproc_exportall_1_svc(void *, exports *, struct svc_req *);
extern int mountprog_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	MOUNTPROC_NULL ((unsigned long)(0))
extern  enum clnt_stat mountproc_null_1();
extern  bool_t mountproc_null_1_svc();
#define	MOUNTPROC_MNT ((unsigned long)(1))
extern  enum clnt_stat mountproc_mnt_1();
extern  bool_t mountproc_mnt_1_svc();
#define	MOUNTPROC_DUMP ((unsigned long)(2))
extern  enum clnt_stat mountproc_dump_1();
extern  bool_t mountproc_dump_1_svc();
#define	MOUNTPROC_UMNT ((unsigned long)(3))
extern  enum clnt_stat mountproc_umnt_1();
extern  bool_t mountproc_umnt_1_svc();
#define	MOUNTPROC_UMNTALL ((unsigned long)(4))
extern  enum clnt_stat mountproc_umntall_1();
extern  bool_t mountproc_umntall_1_svc();
#define	MOUNTPROC_EXPORT ((unsigned long)(5))
extern  enum clnt_stat mountproc_export_1();
extern  bool_t mountproc_export_1_svc();
#define	MOUNTPROC_EXPORTALL ((unsigned long)(6))
extern  enum clnt_stat mountproc_exportall_1();
extern  bool_t mountproc_exportall_1_svc();
extern int mountprog_1_freeresult();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_fhandle(XDR *, fhandle);
extern  bool_t xdr_fhstatus(XDR *, fhstatus*);
extern  bool_t xdr_dirpath(XDR *, dirpath*);
extern  bool_t xdr_name(XDR *, name*);
extern  bool_t xdr_mountlist(XDR *, mountlist*);
extern  bool_t xdr_mountbody(XDR *, mountbody*);
extern  bool_t xdr_groups(XDR *, groups*);
extern  bool_t xdr_groupnode(XDR *, groupnode*);
extern  bool_t xdr_exports(XDR *, exports*);
extern  bool_t xdr_exportnode(XDR *, exportnode*);

#else /* K&R C */
extern bool_t xdr_fhandle();
extern bool_t xdr_fhstatus();
extern bool_t xdr_dirpath();
extern bool_t xdr_name();
extern bool_t xdr_mountlist();
extern bool_t xdr_mountbody();
extern bool_t xdr_groups();
extern bool_t xdr_groupnode();
extern bool_t xdr_exports();
extern bool_t xdr_exportnode();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_MOUNT_ */
