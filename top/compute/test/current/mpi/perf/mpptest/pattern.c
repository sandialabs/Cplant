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
 *  $Id: pattern.c,v 1.1 1997/10/29 20:31:54 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mpi.h"
extern int __NUMNODES, __MYPROCID;static MPI_Status _mpi_status;static int _n, _MPILEN;

/*
    This file contains routines to choose the "partners" given a distance or 
    index.
 */

enum { RING, DOUBLE, BFLY, HYPERCUBE, SHIFT } Pattern;

void SetPattern( argc, argv )
int *argc;
char **argv;
{
Pattern = RING;

if (SYArgHasName( argc, argv, 1, "-nbrring" ))  Pattern = RING;
if (SYArgHasName( argc, argv, 1, "-nbrdbl" ))   Pattern = DOUBLE;
if (SYArgHasName( argc, argv, 1, "-nbrhc" ))    Pattern = HYPERCUBE;
if (SYArgHasName( argc, argv, 1, "-nbrshift" )) Pattern = SHIFT;
}

int GetMaxIndex( )
{
int i, cnt;
switch (Pattern) {
    case RING: 
    case SHIFT:
        return __NUMNODES-1;
    case DOUBLE:
    case HYPERCUBE:
    i   = 1;
    cnt = 1;
    while (i < __NUMNODES) {
	i <<= 1;
	cnt++;
	}
    return cnt;
    }
return 0;
}

/* For operations that do not involve pair operations, we need to separate the
   source and destination 
 */
int GetDestination( loc, index, is_master )
int loc, index, is_master;
{
switch (Pattern) {
    case SHIFT:
    return (loc + index) % __NUMNODES;
    }
return GetNeighbor( loc, index, is_master );
}

int GetSource( loc, index, is_master )
int loc, index, is_master;
{
switch (Pattern) {
    case SHIFT:
    return (loc - index + __NUMNODES) % __NUMNODES;
    }
return GetNeighbor( loc, index, is_master );
}

/* Exchange operations (partner is both source and destination) */
int GetNeighbor( loc, index, is_master )
int loc, index, is_master;
{
int np   = __NUMNODES;

switch (Pattern) {
    case RING: 
        if (is_master) return (loc + index) % np;
	return (loc + np - index) % np;
    case DOUBLE:
	if (is_master) return (loc + (1 << (index-1))) % np;
	return (loc - (1 << (index-1)) + np) % np;
    case HYPERCUBE:
	return loc ^ (1 << (index-1));
    default:
	fprintf( stderr, "Unknown or unsupported pattern\n" );
    }
return loc;
}

PrintPatternHelp()
{
fprintf( stderr, "\n\
Pattern (Neighbor) choices:\n\
  -nbrring  - neighbors are +/- distance\n\
  -nbrdbl   - neighbors are +/- 2**distance\n\
  -nbrhc    - neighbors are hypercube\n\
  -nbrshift - neighbors are + distance (wrapped)\n\
" );
}
