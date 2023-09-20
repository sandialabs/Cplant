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

#include "rpc/rpc.h"

IDENTIFY("$Id: rpc_svc.c,v 1.2 2000/03/15 00:10:06 lward Exp $");

/*
 * Create RPC service offerring.
 */
int
service_create(u_long program,
	       u_long version,
	       void (*dispatch)(),
	       int proto,
	       u_short port)
{
	int	err;
	int	type;
	int	sock;
	struct sockaddr_in addr;
	SVCXPRT	*transp;
	int	i;

	err = 0;
	switch (proto) {

	case IPPROTO_TCP:
		type = SOCK_STREAM;
		break;
	case IPPROTO_UDP:
		type = SOCK_DGRAM;
		break;
	default:
		return EPROTONOSUPPORT;
	}
	(void )svc_unregister(program, version);
	sock = socket(AF_INET, type, proto);
	if (sock < 0)
		return errno;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		err = errno;
	if (!err) {
		transp = svcudp_create(sock);
		if (transp == NULL) {
			LOG(LOG_ERR,
			    "can't create transport %lu.%lu [%hu]",
			    program, version,
			    type);
			err = -1;
		}
	}
	if (!err) {
		i =
		    svc_register(transp,
		    		 program, version,
				 dispatch,
				 proto);
		if (!i) {
			LOG(LOG_ERR,
			    "can't register transport for %lu.%lu [%hu]",
			    program, version,
			    type);
			err = -1;
		}
	}
	if (err)
		(void )close(sock);

	return err;
}
