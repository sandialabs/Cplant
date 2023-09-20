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
#include <string.h>

#include "cmn.h"
#include "smp.h"
#include "creds.h"

IDENTIFY("$Id: creds.c,v 0.2 2000/02/15 23:28:19 lward Exp $");

#ifdef DEBUG
void
dbg_creds(struct creds *crp, const char *leader)
{
	static char buf[256];
	char	*s;
	unsigned indx;
	static mutex_t mutex = MUTEX_INITIALIZER;

	mutex_lock(&mutex);
	s = buf;
	for (indx = 0; indx < crp->cr_ngroups; indx++) {
		if (s != buf)
			*s++ = ' ';
		(void )sprintf(s,
			       "%s%lu",
			       leader,
			       (unsigned long )crp->cr_groups[indx]);
		s += strlen(s);
	}
	*s = '\0';
	LOG_DBG(1, "creds: %u [%s]", crp->cr_uid, buf);
	mutex_unlock(&mutex);
}
#endif
