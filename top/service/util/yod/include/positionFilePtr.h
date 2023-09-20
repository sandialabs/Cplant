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
** $Id: positionFilePtr.h,v 1.1 1997/09/28 21:54:31 lafisk Exp $
*/
 
#ifndef POSITIONFILEPTR_H
#define POSITIONFILEPTR_H

#include <unistd.h>
#include <sys/types.h>
#include "fileHandle.h"

BIGGEST_OFF
positionFilePtr(fileHandle_t *fh, BIGGEST_OFF nodeCurPos, UINT16 nid,
    UINT16 pid);

#endif

