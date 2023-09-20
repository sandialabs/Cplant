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
#ifndef VROUTE_H
#define VROUTE_H

#include <string.h>
#include <unistd.h>

static const char* handshake_msg = "unique_vroute_handshake_msg";

static char istr[21];

void itoa(int n, char s[]);
int doRead(int fd, char buf[], int size, int nbytes);

/* integer to ascii */
void itoa(int n, char s[]) {
  int i, j, c, sign;
  if ((sign = n) < 0)
    n = -n;
  j = 0;
  do {
    s[j++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    s[j++] = '-';
  s[j--] = '\0';
  for (i=0; i<j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

int
doRead(int fd, char buf[], int size, int nbytes)
{
  int n=0, m;

//printf("doRead: nbytes= %d\n",nbytes);

  while ( n < nbytes ) {
    m = read(fd, buf, size);
  //printf("doRead: m= %d\n",m);
    n += m;
  }
  return n;
}

#endif
