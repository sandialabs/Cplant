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
/* $Id: test.c,v 1.3 2001/02/11 11:16:58 lafisk Exp $ Procedures for recording and printing test results */

#include <stdio.h>
#include <string.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

static int tests_passed = 0;
static int tests_failed = 0;
static char failed_tests[255][81];
static char suite_name[255];
FILE *fileout = NULL;

void Test_Init(suite, rank)
char *suite;
int rank;
{
    char filename[512];

    sprintf(filename, "%s-%d.out", suite, rank);
    strncpy(suite_name, suite, 255);
    fileout = fopen(filename, "w");
}

#if defined(__STDC__)
void Test_Printf(char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    if (fileout){
        (void)vfprintf(fileout, format, arglist);
    }
    va_end(arglist);
}
#else
void Test_Printf(va_alist)
va_dcl
{
    char *format;
    va_list arglist;

    va_start(arglist);
    format = va_arg(arglist, char *);
    if (fileout){
         (void)vfprintf(fileout, format, arglist);
         fflush(fileout);
    }
    va_end(arglist);
}
#endif

void Test_Message(mess)
char *mess;
{
    if (fileout){
        fprintf(fileout, "[%s]: %s\n", suite_name, mess);
        fflush(fileout);
    }
}

void Test_Failed(test)
char *test;
{
    strncpy(failed_tests[tests_failed], test, 81);
    if (fileout){
       fprintf(fileout, "[%s]: *** Test '%s' Failed! ***\n", suite_name, test);
       fflush(fileout);
    }
    tests_failed++;
}

void Test_Passed(test)
char *test;
{
    if (fileout){
       fprintf(fileout, "[%s]: Test '%s' Passed.\n", suite_name, test);
       fflush(fileout);
    }
    tests_passed++;
}

int Summarize_Test_Results()
{
    if (fileout){
        fprintf(fileout, "For test suite '%s':\n", suite_name);
        fprintf(fileout, "Of %d attempted tests, %d passed, %d failed.\n", 
	    tests_passed + tests_failed, tests_passed, tests_failed);
        if (tests_failed > 0) {
	    int i;

	    fprintf(fileout, "*** Tests Failed:\n");
	    for (i = 0; i < tests_failed; i++)
	        fprintf(fileout, "*** %s\n", failed_tests[i]);
        }
    }
    return tests_failed;
}

void Test_Finalize()
{
    if (fileout){
        fflush(fileout);
        fclose(fileout);
    }
}

#include "mpi.h"
/* Wait for every process to pass through this point.  This test is used
   to make sure that all processes complete, and that a test "passes" because
   it executed, not because it some process failed.  
 */
void Test_Waitforall( )
{
int m, one, myrank, n;

MPI_Comm_rank( MPI_COMM_WORLD, &myrank );
MPI_Comm_size( MPI_COMM_WORLD, &n );
one = 1;
MPI_Allreduce( &one, &m, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );

if (m != n) {
    printf( "[%d] Expected %d processes to wait at end, got %d\n", myrank, 
	    n, m );
    }
if (myrank == 0) 
    printf( "All processes completed test\n" );
}
