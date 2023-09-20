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
#ifndef _FYOD_MAP
#define _FYOD_MAP

#include <string.h>

#define MAX_FYOD_NODES                 100
#define MAX_DISKS_PER_FYOD_NODE          2

/* should be MAX_FYOD_NODES * MAX_DISKS_PER_FYOD_NODE */
#define FYOD_MAP_SZ                    200

/* should be 10*(MAX_DISKS_PER_FYOD_NODE-1) */
#define FYOD_FACTOR                     10

#endif
