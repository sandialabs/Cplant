/* @(#)getrpcport.c	2.1 88/07/29 4.0 RPCSRC */
#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getrpcport.c 1.3 87/08/11 SMI";
#endif
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpc/rpc.h"
#include <netdb.h>
#include <sys/socket.h>
#include "rpc/pmap_clnt.h"

IDENTIFY("$Id: getrpcport.c,v 0.2 2001/07/18 18:57:50 rklundt Exp $");

int
getrpcport(host, prognum, versnum, proto)
	char *host;
{
	char	*buf;
	struct hostent h, *hp;
	int host_errno;
	struct sockaddr_in addr;

	buf = mem_alloc(BUFSIZ);
	if (buf == NULL) {
		my_perror("mem_alloc");
		return 0;
	}
	if (gethostbyname_r(host, &h, buf, BUFSIZ, &hp, &host_errno) != 0)
		hp = NULL;
	if (hp != NULL) {
		bcopy(hp->h_addr, (char *) &addr.sin_addr, hp->h_length);
		addr.sin_family = AF_INET;
		addr.sin_port =  0;
	}
	mem_free(buf, BUFSIZ);
	return (hp == NULL ? 0 : pmap_getport(&addr, prognum, versnum, proto));
}
