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
#include <errno.h>

#include "cmn.h"

IDENTIFY("$Id: perror.c,v 0.1.6.1 2001/04/23 21:36:08 rklundt Exp $");

/*
 * Local implementation of the perror (section 3) library routine. This
 * one uses our logging interface.
 */
void
_my_perror(const char *file, unsigned line, const char *s)
{

	logmsg(LOG_ERR, file, line, "%s: %s", s, strerror(errno));
}
