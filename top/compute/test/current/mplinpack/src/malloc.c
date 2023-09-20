/*
Copyright (c) 1993 Sandia National Laboratories
 
  Redistribution and use in source and binary forms are permitted
  provided that source distributions retain this entire copyright
  notice.  Neither the name of Sandia National Laboratories nor
  the name of the author may be used to endorse or promote products
  derived from this software without specific prior written permission.
 
Warranty:
  This software is provided "as is" and without any express or
  implied warranties, including, without limitation, the implied
  warranties of merchantability and fitness for a particular
  purpose.

The README file contains addition information
*/

#include<stdio.h>
#include<stdlib.h>
#include "malloc.h" 

#define ALIGNED_TO   ((unsigned)32) /* number of bytes to align to */
#define EXTRA        (ALIGNED_TO)
#define MASK         (~(EXTRA-1))

/*
** On the Intel Paragon we are CHAMELEON aligned. No problem.
*/

void *MALLOC_8(int size, char** ptr)
{
    *ptr = malloc(size);
    return((void *) *ptr);
}  /* end of MALLOC_8() */



/* --------------------------------------------------------------- */
/* return a void pointer that is aligned along ALIGNED_TO boundary */
/* heaven help you if you free one of these...                     */
/* --------------------------------------------------------------- */

static long ALIGN_64 = ~0x3f;

void *MALLOC_64(len, ptr)
size_t len;
char **ptr;
{
  void *p;

  //p=malloc(len+63);

  *ptr=malloc(len+63);

  if(*ptr==NULL) {
    return(NULL);
  }
  else{
     p = (void *)(((unsigned long)*ptr+63) & ALIGN_64);
  }
  return p;
}


void *MALLOC_32(len, ptr)
size_t len;
char **ptr;
{
  //void *p;

  //p=malloc(len+EXTRA);

  *ptr = malloc(len+EXTRA);

  if(*ptr==NULL) {
    return(NULL);
  }
  else{
    return((void *)((((unsigned long)*ptr) + EXTRA) & MASK));
  }
}

