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
/* $Id: getcwd.c,v 1.6 2002/02/12 18:51:13 pumatst Exp $ */

#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include "puma.h"
#include "errno.h"

char *
getwd( char *pathname) {
    strcpy(pathname, _CLcwd);
    return(pathname);
}

/*****************************************************************************/

#ifdef __osf__
char *
getcwd_(char *buffer, size_t size)
{
    return getcwd( buffer, size);
}
#endif


char *
__getcwd(char *buffer, size_t size)
{
    return getcwd( buffer, size);
}


char *
getcwd(char *buffer, size_t size)
{

size_t cwdsize;

    if (size <= 0) {
	errno = EINVAL;
	return(NULL);
    }

    cwdsize = strlen(_CLcwd) + 1;
    if (size < cwdsize) {
	errno = EINVAL;
	return(NULL);
    }

    if (buffer == NULL) {
	buffer = (char *)malloc(size);
	if (buffer == NULL) {
	    errno = ENOMEM;
	    return(NULL);
	}
    }

    strcpy(buffer, _CLcwd);

    return(buffer);
}

