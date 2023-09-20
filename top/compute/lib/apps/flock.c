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
/* $Id: flock.c,v 1.3 2001/02/16 05:28:43 lafisk Exp $ */

#include "puma.h"
#include "errno.h"
#include <stdio.h>

/******************************************************************************/

INT32
__funlockall( INT32 fd, INT32 operation )
{
    errno = EOPNOTSUPP;
    return -1;
}

INT32
__flockall( INT32 fd, INT32 operation )
{
    errno = EOPNOTSUPP;
    return -1;
}

INT32
__flock( INT32 fd, INT32 operation )
{
    errno = EOPNOTSUPP;
    return -1;
}

INT32
flock( INT32 fd, INT32 operation )
{
    errno = EOPNOTSUPP;
    return -1;
}
VOID
flockfile(FILE *f)
{
    errno = EOPNOTSUPP;
}
VOID
__flockfile(FILE *f)
{
    errno = EOPNOTSUPP;
}
VOID
funlockfile(FILE *f)
{
    errno = EOPNOTSUPP;
}
VOID
__funlockfile(FILE *f)
{
    errno = EOPNOTSUPP;
}
