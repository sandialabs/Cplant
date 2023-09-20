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

#include "cnxsvc.h"

IDENTIFY("$Id");

/*
 * Map system error number to CNX status.
 */
cnx_status
cnx_errstat(int err)
{
	unsigned indx;
	struct errmap {
		int	e;
		cnx_status status;
	};
	struct errmap *mp;
	static struct errmap map[] = {
		{ 0,		CNX_OK },
		{ EPERM,	CNXERR_PERM },
		{ ENOENT,	CNXERR_NOENT },
		{ EIO,		CNXERR_IO },
		{ ENXIO,	CNXERR_NXIO },
		{ ENOMEM,	CNXERR_NOMEM },
		{ EACCES,	CNXERR_ACCES },
		{ ENOTBLK,	CNXERR_NOTBLK },
		{ EBUSY,	CNXERR_BUSY },
		{ ENODEV,	CNXERR_NODEV },
		{ ENOTDIR,	CNXERR_NOTDIR },
		{ EINVAL,	CNXERR_INVAL },
		{ EMFILE,	CNXERR_MFILE },
		{ ENAMETOOLONG,	CNXERR_NAMETOOLONG }
	};

	for (indx = 0, mp = map;
	     indx < sizeof(map) / sizeof(map[0]);
	     indx++, mp++)
		if (err == mp->e)
			break;
	if (indx >= sizeof(map) / sizeof(map[0])) {
		LOG(LOG_WARNING, "cnx_errstat: unmapped error # %d", err);
		return CNXERR_IO;
	}

	return mp->status;
}

/*
 * Start a CONNEX service.
 */
int
cnx_startsvc(const char *name, void *arg, size_t arglen, struct creds *crp)
{
	struct cnxconf *cnxconfp;

	for (cnxconfp = cnxconftbl;
	     (cnxconfp->cnxconf_name != NULL &&
	      cnxconfp->cnxconf_name[0] != '\0');
	     cnxconfp++)
		if (strncasecmp(cnxconfp->cnxconf_name,
				name,
				CNX_MAXTYPNAMLEN) == 0)
			break;

	if (cnxconfp->cnxconf_name == NULL ||
	    cnxconfp->cnxconf_name[0] == '\0')
		return ENOENT;

	return (*cnxconfp->cnxconf_startsvc)(arg, arglen, crp);
}
