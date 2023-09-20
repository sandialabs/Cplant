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
 * $Id: cnx_prot.h,v 1.1 1999/11/29 19:43:53 lward Exp $
 */

#ifndef _CNX_PROT_H_RPCGEN
#define	_CNX_PROT_H_RPCGEN

#include "rpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	CNX_MAXTYPNAMLEN 32
#define	CNX_MAXPATHLEN 1024
#define	CNX_OPAQARGSSIZ 2048

enum cnx_status {
	CNX_OK = 0,
	CNXERR_PERM = 1,
	CNXERR_NOENT = 2,
	CNXERR_IO = 5,
	CNXERR_NXIO = 6,
	CNXERR_NOMEM = 12,
	CNXERR_ACCES = 13,
	CNXERR_NOTBLK = 15,
	CNXERR_BUSY = 16,
	CNXERR_NODEV = 19,
	CNXERR_NOTDIR = 20,
	CNXERR_INVAL = 22,
	CNXERR_MFILE = 24,
	CNXERR_NAMETOOLONG = 36
};
typedef enum cnx_status cnx_status;

typedef char *cnx_typenam;

typedef char *cnx_path;

typedef char cnx_opaqarg[CNX_OPAQARGSSIZ];

typedef u_int cnx_svcid;

struct cnx_mountarg {
	cnx_typenam type;
	cnx_path path;
	cnx_opaqarg arg;
};
typedef struct cnx_mountarg cnx_mountarg;

struct cnx_offerarg {
	cnx_typenam type;
	cnx_opaqarg arg;
};
typedef struct cnx_offerarg cnx_offerarg;

struct cnx_svcidres {
	cnx_status status;
	union {
		cnx_svcid id;
	} cnx_svcidres_u;
};
typedef struct cnx_svcidres cnx_svcidres;

#define	CNX_PROGRAM ((unsigned long)(400050))
#define	CNX_VERSION ((unsigned long)(1))

#if defined(__STDC__) || defined(__cplusplus)
#define	CNXPROC_MOUNT ((unsigned long)(1))
extern  enum clnt_stat cnxproc_mount_1(cnx_mountarg *, cnx_status *, CLIENT *);
extern  bool_t cnxproc_mount_1_svc(cnx_mountarg *, cnx_status *, struct svc_req *);
#define	CNXPROC_UNMOUNT ((unsigned long)(2))
extern  enum clnt_stat cnxproc_unmount_1(cnx_path *, cnx_status *, CLIENT *);
extern  bool_t cnxproc_unmount_1_svc(cnx_path *, cnx_status *, struct svc_req *);
#define	CNXPROC_OFFER ((unsigned long)(3))
extern  enum clnt_stat cnxproc_offer_1(cnx_offerarg *, cnx_svcidres *, CLIENT *);
extern  bool_t cnxproc_offer_1_svc(cnx_offerarg *, cnx_svcidres *, struct svc_req *);
#define	CNXPROC_STOP ((unsigned long)(4))
extern  enum clnt_stat cnxproc_stop_1(cnx_svcid *, cnx_status *, CLIENT *);
extern  bool_t cnxproc_stop_1_svc(cnx_svcid *, cnx_status *, struct svc_req *);
extern int cnx_program_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define	CNXPROC_MOUNT ((unsigned long)(1))
extern  enum clnt_stat cnxproc_mount_1();
extern  bool_t cnxproc_mount_1_svc();
#define	CNXPROC_UNMOUNT ((unsigned long)(2))
extern  enum clnt_stat cnxproc_unmount_1();
extern  bool_t cnxproc_unmount_1_svc();
#define	CNXPROC_OFFER ((unsigned long)(3))
extern  enum clnt_stat cnxproc_offer_1();
extern  bool_t cnxproc_offer_1_svc();
#define	CNXPROC_STOP ((unsigned long)(4))
extern  enum clnt_stat cnxproc_stop_1();
extern  bool_t cnxproc_stop_1_svc();
extern int cnx_program_1_freeresult();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_cnx_status(XDR *, cnx_status*);
extern  bool_t xdr_cnx_typenam(XDR *, cnx_typenam*);
extern  bool_t xdr_cnx_path(XDR *, cnx_path*);
extern  bool_t xdr_cnx_opaqarg(XDR *, cnx_opaqarg);
extern  bool_t xdr_cnx_svcid(XDR *, cnx_svcid*);
extern  bool_t xdr_cnx_mountarg(XDR *, cnx_mountarg*);
extern  bool_t xdr_cnx_offerarg(XDR *, cnx_offerarg*);
extern  bool_t xdr_cnx_svcidres(XDR *, cnx_svcidres*);

#else /* K&R C */
extern bool_t xdr_cnx_status();
extern bool_t xdr_cnx_typenam();
extern bool_t xdr_cnx_path();
extern bool_t xdr_cnx_opaqarg();
extern bool_t xdr_cnx_svcid();
extern bool_t xdr_cnx_mountarg();
extern bool_t xdr_cnx_offerarg();
extern bool_t xdr_cnx_svcidres();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_CNX_PROT_H_RPCGEN */
