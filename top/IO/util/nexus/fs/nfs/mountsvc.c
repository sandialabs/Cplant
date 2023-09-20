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
#include "cmn.h"
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"
#include "creds.h"
#include "vnode.h"
#include "vfs.h"
#include "mountsvc.h"

IDENTIFY("$Id: mountsvc.c,v 1.2 2000/03/03 23:05:02 lward Exp $");

/*
 * Mount protocol version 1 server side support.
 */

static mutex_t mutex = MUTEX_INITIALIZER;		/* package mutex */

/*
 * Mount server start routine.
 */
int
mount_startsvc(const void *arg IS_UNUSED,
	       size_t arglen IS_UNUSED,
	       struct creds *crp)
{
	int	err;
	static int started = 0;
	extern void mountprog_1(struct svc_req, SVCXPRT *);

	if (!is_suser(crp))
		return EPERM;

	mutex_lock(&mutex);
	if (started) {
		mutex_unlock(&mutex);
		return EBUSY;
	}
	started = 1;
	mutex_unlock(&mutex);

	mount_initialize_server();

	(void )pmap_unset(MOUNTPROG, MOUNTVERS);
	err = service_create(MOUNTPROG, MOUNTVERS, mountprog_1, IPPROTO_UDP, 0);
	if (err) {
		LOG(LOG_ERR,
		    "couldn't create (MOUNTPROG, MOUNTVERS, udp) service");
		return err;
	}
	return 0;
}
