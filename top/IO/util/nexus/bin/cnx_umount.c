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

IDENTIFY("$Id: cnx_umount.c,v 1.1 1999/11/29 19:34:39 lward Exp $");

static void usage(void);
static int umount(CLIENT *, const char *);

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

	if (argc - optind != 2) {
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

	err = umount(clnt, argv[optind]);
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
umount(CLIENT *clnt, const char *path)
{
	int	err;
	cnx_status *result;

	result = cnxproc_unmount_1((cnx_path *)&path, clnt);
	if (result == NULL) {
		clnt_perror(clnt, "call failed");
		return -1;
	}
	err = cnx_staterr(*result);
	xdr_free((xdrproc_t )xdr_cnx_status, (char *)result);

	return err;
}

static void
usage(void)
{

	(void )fprintf(stderr, "Usage: cnx_umount server path\n");
}
