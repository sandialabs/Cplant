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
/* fault.c -- segfault if _my_rank is odd */

#include "stdio.h"
#include "puma.h"

void fpe(float* x);
void segv(int* x);

int main()
{
  int* y = (int*)4;

  if ( 1 ) {
//  if ( _my_rank % 2 ) {
  //fpe(&x);
    segv(y);
  }

  return 0;
}

void fpe(float* x)
{
  *x /= 0;
}

void segv(int* x)
{
  *x = 11;
}
