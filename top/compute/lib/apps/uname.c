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
/* $Id: uname.c,v 1.7 2002/02/12 18:51:14 pumatst Exp $ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <string.h>
#include "puma.h"
#include "puma_unistd.h"
#include "errno.h"

const char *_puma_release_number = "0.0";

int
uname(lcoug_uname)
struct utsname *lcoug_uname;
{
	char *s,*ss;
	int n;

	if (gethostname(lcoug_uname->nodename,SYS_NMLN) < 0) {
		return(-1);
	}
	if ((s = strstr(_puma_release_number,"ALT OS =")) != NULL) {
		s += 8;
		if ((ss = strchr(s,' ')) != NULL) {
			if ((n = ss - s) >= SYS_NMLN) n = SYS_NMLN -1;
			lcoug_uname->sysname[0] = (char)NULL;
			strncat(lcoug_uname->sysname,s,n);
		}
	}
	if ((s = strstr(_puma_release_number,"Release")) != NULL) {
		s += 8;
		if ((ss = strchr(s,' ')) != NULL) {
			if ((n = ss - s) >= SYS_NMLN) n = SYS_NMLN -1;
			lcoug_uname->release[0] = (char)NULL;
			strncat(lcoug_uname->release,s,n);
		}
	}
	if ((s = strstr(_puma_release_number," - ")) != NULL) {
		s += 3;
		if ((n = strlen(s)) >= SYS_NMLN) n = SYS_NMLN -1;
		lcoug_uname->version[0] = (char)NULL;
		strncat(lcoug_uname->version,s,n);
	}
#ifdef __i860__
	sprintf(lcoug_uname->machine, "i860");
#endif
#ifdef __i386__
	sprintf(lcoug_uname->machine, "Pentium Pro Processor (TM)");
#endif
	return(0);
}  /* end of uname() */

