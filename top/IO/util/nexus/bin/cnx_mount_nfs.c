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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cnx_prot.h"
#include "cnx.h"
#include "nfsmnt.h"

IDENTIFY("$Id: cnx_mount_nfs.c,v 1.5 2001/08/13 23:18:21 rklundt Exp $");

const char *optstr =
	" [-a <attr-timeout>]"
	" [-t <call-timeout>]";

static long attrtimeo = 15;				/* attr timeo (secs) */
static long rpctimeo = 0;				/* call timeo (secs) */

static void usage(void);
static int mount(CLIENT *, const char *, const char *);

int
main(int argc, char * const *argv)
{
	int	i;
	CLIENT	*clnt;
	AUTH	*auth;
	int	err;
	struct timeval retrytimeo = { 180, 0 };
	struct timeval totaltimeo = { 179, 0 };

	while ((i = getopt(argc, argv, "a:t:")) != -1)
		switch (i) {

		case 'a':				/* set attrtimeo */
			attrtimeo = atol(optarg);
			if (attrtimeo < 0) {
				(void )fprintf(stderr, "bad attr timeout\n");
				exit(1);
			}
			break;
		case 't':				/* set call timeo */
			rpctimeo = atol(optarg);
			if (rpctimeo <= 0) {
				(void )fprintf(stderr,
					       "bad RPC call timeout\n");
				exit(1);
			}
			break;
		default:
			usage();
			exit(1);
		}

	if (argc - optind < 2 || argc - optind > 3) {
		usage();
		exit (1);
	}

	/*
	 * Get client connection to remote.
	 */
	clnt = clnt_create(argv[optind], CNX_PROGRAM, CNX_VERSION, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror(argv[optind]);
		return 1;
	}

	auth = authunix_create_default();
	if (auth == NULL) {
		(void )fprintf(stderr, "can't create RPC credentials\n");
		clnt_destroy(clnt);
		return 1;
	}
	auth_destroy(clnt->cl_auth);
	clnt->cl_auth = auth;

	optind++;

	/*
	 * Temporary fix for startup problem. 
	 * Set timeout to prevent re-tries, they are crashing the proxy.
	 */
	if (! clnt_control(clnt, CLSET_RETRY_TIMEOUT, (char *) &retrytimeo) ) {
		(void )fprintf(stderr, "can't reset retry timeout\n");
		clnt_destroy(clnt);
		return 1;
	}
	if (! clnt_control(clnt, CLSET_TIMEOUT, (char *) &totaltimeo) ) {
		(void )fprintf(stderr, "can't reset total timeout\n");
		clnt_destroy(clnt);
		return 1;
	} /* end temp fix */

	err = mount(clnt, argv[optind], argv[optind + 1]);
	if (err) {
		if (err > 0)
			(void )fprintf(stderr,
				       "%s: %s\n",
				       argv[optind],
				       strerror(err));
		else
			err = 1;
	}

	clnt_destroy(clnt);

	return err;
}

static int
mount(CLIENT *clnt, const char *arg, const char *path)
{
	static nfsmnt nfsmnt;
	static char *cp;
	static XDR xdrs;
	int	err;
	static cnx_mountarg mountarg;
	cnx_status *result;

	/*
	 * Parse host:path argument and build the file system specific
	 * portion of the mount call.
	 */
	(void )memset(&mountarg, 0, sizeof(mountarg));
	(void )memset(&nfsmnt, 0, sizeof(nfsmnt));
	cp = strchr(arg, ':');
	if (cp == NULL || cp == arg || *(cp + 1) == '\0') {
		usage();
		return -1;
	}
	nfsmnt.rhost = malloc(cp - arg + 1);
	if (nfsmnt.rhost == NULL) {
		perror("malloc");
		return errno;
	}
	(void )strncpy(nfsmnt.rhost, arg, cp - arg);
	nfsmnt.rhost[cp - arg] = '\0';
	nfsmnt.rpath = cp + 1;
	nfsmnt.attrtimeo = attrtimeo;
	nfsmnt.rpctimeo = rpctimeo;
	(void )memset(&xdrs, 0, sizeof(xdrs));
	xdrmem_create(&xdrs,
		      mountarg.arg,
		      sizeof(mountarg.arg),
		      XDR_ENCODE);
	err = 0;
	if (!xdr_nfsmnt(&xdrs, &nfsmnt)) {
		(void )fprintf(stderr, "can't encode `%s'\n", arg);
		err = -1;
	}
	free(nfsmnt.rhost);
	if (err)
		return err;

	/*
	 * Build the argument.
	 */
	mountarg.type = "nfs";
	mountarg.path = (cnx_path )(path == NULL ? "" : path);

	result = cnxproc_mount_1(&mountarg, clnt);
	if (result == NULL) {
		clnt_perror(clnt, "call failed");
		return -1;
	}
	err = cnx_staterr(*result);
	if (err < 0)
		(void )fprintf(stderr, "cnxproc_mount_1: unknown error\n");
	xdr_free((xdrproc_t )xdr_cnx_status, (char *)result);

	return err;
}

static void
usage(void)
{

	(void )fprintf(stderr,
		       "Usage: cnx_mount_nfs%s server host:path [path]\n",
		       optstr);
}
