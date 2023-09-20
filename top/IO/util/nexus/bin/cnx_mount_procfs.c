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

#if !defined(IS_UNUSED)
#if defined(__GNUC___)
#define IS_UNUSED	__attribute__ ((unused))
#else
#define IS_UNUSED
#endif
#endif

IDENTIFY("$Id: cnx_mount_procfs.c,v 1.1 2000/06/22 17:13:41 smorgan Exp $");

const char *optstr =
	"";

static void usage(void);
static int mount(CLIENT *, const char *, char *);

int
main(int argc, char * const *argv)
{
	int	i;
	CLIENT	*clnt;
	AUTH	*auth;
	int	err;

	while ((i = getopt(argc, argv, "")) != -1)
		switch (i) {

		default:
			usage();
			exit(1);
		}

	if (argc - optind < 2) {
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
		(void )fprintf(stderr, "can't create RPC credentials");
		clnt_destroy(clnt);
		return 1;
	}
	auth_destroy(clnt->cl_auth);
	clnt->cl_auth = auth;

	optind++;

	err = mount(clnt, argv[optind], NULL);
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
mount(CLIENT *clnt, const char *path, char *arg IS_UNUSED)
{
	int	err;
	XDR	xdrs;
	static cnx_mountarg mountarg;
	cnx_status *result;

	/* currently, the process file system has no fs specific argument */
	(void )memset(&mountarg, 0, sizeof(mountarg));
	(void ) memset(&xdrs, 0, sizeof(xdrs));
	xdrmem_create(&xdrs, mountarg.arg, sizeof(mountarg.arg), XDR_ENCODE);

	/*
	 * Build the argument.
	 */
	mountarg.type = "procfs";
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
		       "Usage: cnx_mount_procfs%s server [path]\n",
		       optstr);
}
