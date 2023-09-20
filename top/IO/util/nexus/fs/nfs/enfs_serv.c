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
#ifdef ENFS

#include <stdlib.h>					/* for free() */
#include <string.h>

#include "cmn.h"
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"
#include "creds.h"
#include "vnode.h"
#include "vfs.h"
#include "direct.h"
#include "nfssvc.h"
#include "../../include/rpcsvc/enfs_prot.h"

/*
 * ENFS V1 server.
 *
 * ENFS is designed to be backward compatible with NFS version 2. There
 * are exceptions:
 *	1)	The addition of the `synchronize' operation. This is not
 * 		found in NFS v2 but is in ENFS. It is proc # 18.
 *	2)	Data payload limit is increased from 8k to 64k.
 *
 * This sub-system depends heavily on the ability of the compiler (GCC 2.91.66)
 * to alias symbols. The protocol include file aliases all but the new
 * synchronize operation.
 *
 * In reality, the readargs, readres and writeargs formats are different.
 * However, we *know* how rpcgen defines them and how the ONC RPC library
 * uses them. We are assuming that payloads are always declared as pointers
 * to the base type instead of arrays. That allows us to use the associated
 * read and write service routines from the existing NFS service.
 * The client side routines must be handled separately so that the proper
 * argument encoders are called. Beware of this hack.
 *
 */

IDENTIFY("$Id: enfs_serv.c,v 1.2 2000/03/03 23:05:01 lward Exp $");

/*
 * ENFS server start routine.
 */
int
enfs_startsvc(const void *arg IS_UNUSED,
	      size_t arglen IS_UNUSED,
	      struct creds *crp)
{
	int	err;
	static int started = 0;
	static mutex_t mutex = MUTEX_INITIALIZER;
	extern void enfs_program_2(struct svc_req, SVCXPRT *);

	if (!is_suser(crp))
		return EPERM;

	mutex_lock(&mutex);
	if (started)
		return EBUSY;
	started = 1;
	mutex_unlock(&mutex);

	nfs_initialize_server();

	/*
	 * Start ENFS service.
	 */
	(void )pmap_unset(ENFS_PROGRAM, ENFS_VERSION);
	err =
	    service_create(ENFS_PROGRAM, ENFS_VERSION,
			   enfs_program_2,
			   IPPROTO_UDP,
			   ENFS_PORT);
	if (err) {
		LOG(LOG_ERR,
		   "couldn't create (ENFS_PROGRAM, ENFS_VERSION, udp) service");
		return err;
	}
	return 0;
}

bool_t
enfsproc_synchronize_2_svc(nfs_fh *argp,
			   nfsstat *result,
			   struct svc_req *rqstp)
{

	NFS_SVC_ENTER(rqstp, argp, NULL);
	*result = NFS_OK;
        NFS_SVC_RETURN(TRUE, rqstp, result);
}

int
enfs_program_2_freeresult(SVCXPRT *transp IS_UNUSED,
			  xdrproc_t xdr_result,
			  caddr_t result)
{
	(void) xdr_free(xdr_result, result);

	return 1;
}
#endif /* defined(ENFS) */
