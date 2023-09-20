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
#include <stdlib.h>
#include <string.h>

#include "cmn.h"

IDENTIFY("$Id: debug.c,v 0.4 2001/07/18 18:57:46 rklundt Exp $");

/*
 * Debugging support.
 */

#ifdef DEBUG

extern unsigned vnode_debug;
extern unsigned cnx_debug;
extern unsigned nfs_debug;

/*
 * Debugging configuration.
 */
static struct config {
	const char *name;				/* entry name */
	const char *explanation;			/* entry explanation */
	unsigned *var;					/* variable */
} conftbl[] = {
	{
		"vnode",
		"Vnode debugging",
		&vnode_debug
	},
	{
		"cnx",
		"CNX service debugging",
		&cnx_debug
	},
	{
		"nfs",
		"(E)NFS server and remote mount service debugging",
		&nfs_debug
	}
};

/*
 * Print usage message (for debugging options anyway).
 */
void
debug_usage(void)
{
	unsigned indx;

	(void )fprintf(stderr, " Debug flags:\n");
	for (indx = 0; indx < sizeof(conftbl) / sizeof(struct config); indx++)
		(void )fprintf(stderr,
			       "\t%s -- %s\n",
			       conftbl[indx].name,
			       conftbl[indx].explanation);
}

/*
 * Set a debug option.
 */
int
debug_set_option(const char *arg)
{
	const char *p;
	size_t	len;
	unsigned long value;
	char *endp;
	unsigned indx;

	p = strchr(arg, '=');
	if (p == NULL || *(p + 1) == '\0') {
		value = 1;
		len = strlen(arg);
	} else {
		len = p - arg;
		value = strtoul(p + 1, &endp, 0);
		if (*endp != '\0')
			return -1;
	}

	for (indx = 0; indx < sizeof(conftbl) / sizeof(struct config); indx++)
		if (strlen(conftbl[indx].name) == len &&
		    strncasecmp(arg, conftbl[indx].name, len) == 0)
			break;
	if (indx >= sizeof(conftbl) / sizeof(struct config)) {
		debug_usage();
		return -1;
	}
	*conftbl[indx].var = value;
	return 0;
}

/*
 * Dummy callback.
 */
void
noop()
{
}
#endif
