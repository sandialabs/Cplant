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
 * $Id: savstr.c,v 0.1 2001/07/18 18:57:26 rklundt Exp $
 */
#include <stdlib.h>
#include <string.h>

#include "cmn.h"

IDENTIFY("$Id: savstr.c,v 0.1 2001/07/18 18:57:26 rklundt Exp $");

/*
 * Save a copy of the passed string in the heap.
 */
char *
savstr(const char *s)
{
	char	*snew;

	snew = m_alloc(strlen(s) + 1);
	if (snew == NULL)
		return NULL;
	(void )strcpy(snew, s);
	return snew;
}
