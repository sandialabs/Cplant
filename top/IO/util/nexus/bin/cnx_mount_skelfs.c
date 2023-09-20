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
#include "skelfsmnt.h"

IDENTIFY("$Id: cnx_mount_skelfs.c,v 1.3 2000/05/23 09:33:32 smorgan Exp $");

const char *optstr =
	"";

static void usage(void);
static int mount(CLIENT *, const char *, char *);

int
main(int argc, char * const *argv)
{
	char	*conf;
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

	if (argc - optind > 0)
		conf = argv[optind + 1];
	else
		conf = (char *) NULL;

	err = mount(clnt, argv[optind], conf);
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

static char *nullconf = "\0";

static int
mount(CLIENT *clnt, const char *path, char *conf)
{
	skelfsmnt skelfsmnt;
	int	err;
	XDR	xdrs;
	static cnx_mountarg mountarg;
	cnx_status *result;

	(void )memset(&mountarg, 0, sizeof(mountarg));

	/*
	 * Build the file system specific mount arg (ie, configuration
	 * file
	 */
	if (conf != NULL) {
		(void ) memset(&skelfsmnt, 0, sizeof(skelfsmnt));
		skelfsmnt.cpath = malloc(strlen(conf) + 1);
		if (skelfsmnt.cpath == NULL) {
			perror("malloc");
			return errno;
		}

		(void ) strcpy(skelfsmnt.cpath, conf);
	}
	else {
		skelfsmnt.cpath = nullconf;
	}

	/* currently, you can only mount the internal skelfs */
	skelfsmnt.mount_internal = 1;

	(void ) memset(&xdrs, 0, sizeof(xdrs));
	xdrmem_create(&xdrs, mountarg.arg, sizeof(mountarg.arg), XDR_ENCODE);
	if (!xdr_skelfsmnt(&xdrs, &skelfsmnt)) {
		(void ) fprintf(stderr, "can't encode '%s'\n", conf);
		free(skelfsmnt.cpath);
		return -1;
	}

	free(skelfsmnt.cpath);

	/*
	 * Build the argument.
	 */
	mountarg.type = "skelfs";
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
		       "Usage: cnx_mount_skelfs%s server [path] [conf file]\n",
		       optstr);
}
