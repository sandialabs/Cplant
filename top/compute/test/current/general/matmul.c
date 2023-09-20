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

#define ORDER 120

extern double dclock(void);

int main()
{
  float A[ORDER][ORDER], B[ORDER][ORDER], C[ORDER][ORDER]; 
  double start, end, diff;
  int i, j, k;

  /* C = A x B */

  start = dclock();

  for (i=0; i<ORDER; i++) {
    for (j=0; j<ORDER; j++) {
       A[i][j] = 0.0;
       B[i][j] = 0.0;
    }
  }

  for (i=0; i<ORDER; i++) {
    for (j=0; j<ORDER; j++) {
      C[i][j] = 0.0;
      for (k=0; k<ORDER; k++) {
        C[i][j] += A[i][k]*B[k][j];  
      }
    }
  }

  end = dclock();

  diff = end-start;

  printf("time for matmul start: %f\n", start);
  printf("time for matmul finish: %f\n", end);

  printf("time for matmul to run: %f\n", diff);


  return 0;
}
