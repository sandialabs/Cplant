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
 * CONNEX service configuration table.
 */
#include <stdio.h>

#include "cnxsvc.h"

IDENTIFY("$Id: cnx_conf.c,v 1.5 2000/03/03 23:09:59 lward Exp $");

extern int nfs_startsvc(const void *, size_t, struct creds *);
extern int mount_startsvc(const void *, size_t, struct creds *);
#ifdef ENFS
extern int enfs_startsvc(const void *, size_t, struct creds *);
#endif
#ifdef SKELFS
extern int skelfs_startsvc(const void *, size_t, struct creds *);
#endif

struct cnxconf cnxconftbl[] = {
	{ "nfs",	nfs_startsvc },
	{ "mount",	mount_startsvc },
#ifdef ENFS
	{ "enfs",	enfs_startsvc },
#endif
#ifdef SKELFS
	{ "skelfs",	skelfs_startsvc },
#endif
	{ "",		NULL }

};
