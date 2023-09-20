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
 *  $Id: typebase.c,v 1.1 1997/10/29 22:45:41 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/*
 */
#include "mpi.h"
#include <stdio.h>

/* 
 * This program checks that the type inquiry routines work with the 
 * basic types
 */

#define MAX_TYPES 12
#if defined(__STDC__) 
static int ntypes = 12;
#else
static int ntypes = 11;
#endif
static MPI_Datatype BasicTypes[MAX_TYPES];
static char         *(BasicTypesName[MAX_TYPES]);
static int          BasicSizes[MAX_TYPES];

void 
SetupBasicTypes()
{
    BasicTypes[0] = MPI_CHAR;
    BasicTypes[1] = MPI_SHORT;
    BasicTypes[2] = MPI_INT;
    BasicTypes[3] = MPI_LONG;
    BasicTypes[4] = MPI_UNSIGNED_CHAR;
    BasicTypes[5] = MPI_UNSIGNED_SHORT;
    BasicTypes[6] = MPI_UNSIGNED;
    BasicTypes[7] = MPI_UNSIGNED_LONG;
    BasicTypes[8] = MPI_FLOAT;
    BasicTypes[9] = MPI_DOUBLE;

    BasicTypesName[0] = "MPI_CHAR";
    BasicTypesName[1] = "MPI_SHORT";
    BasicTypesName[2] = "MPI_INT";
    BasicTypesName[3] = "MPI_LONG";
    BasicTypesName[4] = "MPI_UNSIGNED_CHAR";
    BasicTypesName[5] = "MPI_UNSIGNED_SHORT";
    BasicTypesName[6] = "MPI_UNSIGNED";
    BasicTypesName[7] = "MPI_UNSIGNED_LONG";
    BasicTypesName[8] = "MPI_FLOAT";
    BasicTypesName[9] = "MPI_DOUBLE";

    BasicSizes[0] = sizeof(char);
    BasicSizes[1] = sizeof(short);
    BasicSizes[2] = sizeof(int);
    BasicSizes[3] = sizeof(long);
    BasicSizes[4] = sizeof(unsigned char);
    BasicSizes[5] = sizeof(unsigned short);
    BasicSizes[6] = sizeof(unsigned);
    BasicSizes[7] = sizeof(unsigned long);
    BasicSizes[8] = sizeof(float);
    BasicSizes[9] = sizeof(double);

#if defined (__STDC__)
    if (MPI_LONG_DOUBLE) {
	BasicTypes[10] = MPI_LONG_DOUBLE;
	BasicSizes[10] = sizeof(long double);
	BasicTypes[11] = MPI_BYTE;
	BasicSizes[11] = sizeof(unsigned char);
	BasicTypesName[10] = "MPI_LONG_DOUBLE";
	BasicTypesName[11] = "MPI_BYTE";
	}
    else {
	ntypes = 11;
	BasicTypes[10] = MPI_BYTE;
	BasicSizes[10] = sizeof(unsigned char);
	BasicTypesName[10] = "MPI_BYTE";
	}
#else
    BasicTypes[10] = MPI_BYTE;
    BasicSizes[10] = sizeof(unsigned char);
    BasicTypesName[10] = "MPI_BYTE";
#endif
    }

int main(argc, argv)
int  argc;
char **argv;
{
int      i, errs;
int      size;
MPI_Aint extent, lb, ub;
 
MPI_Init( &argc, &argv );

/* This should be run by a single process */

SetupBasicTypes();

errs = 0;
for (i=0; i<ntypes; i++) {
    MPI_Type_size( BasicTypes[i], &size );
    MPI_Type_extent( BasicTypes[i], &extent );
    MPI_Type_lb( BasicTypes[i], &lb );
    MPI_Type_ub( BasicTypes[i], &ub );
    if (size != extent) {
	errs++;
	printf( "size (%d) != extent (%d) for basic type %s\n", size, extent,
	       BasicTypesName[i] );
	}
    if (size != BasicSizes[i]) {
	errs++;
	printf( "size(%d) != C size (%d) for basic type %s\n", size, 
	       BasicSizes[i], BasicTypesName[i] );
	}
    if (lb != 0) {
	errs++;
	printf( "Lowerbound of %s was %d instead of 0\n", 
	        BasicTypesName[i], (int)lb );
	}
    if (ub != extent) {
	errs++;
	printf( "Upperbound of %s was %d instead of %d\n", 
	        BasicTypesName[i], (int)ub, (int)extent );
	}
    }

if (errs) {
    printf( "Found %d errors in testing C types\n", errs );
    }
else {
    printf( "Found no errors in basic C types\n" );
    }

MPI_Finalize( );
return 0;
}
