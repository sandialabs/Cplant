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
/* top/compute/OS/addrCache/cache.h */

#ifndef _USR_ADDR_CACHE_H_
#define _USR_ADDR_CACHE_H_

#define ADDR_LIMIT     10000
#define NUM_SUBCACHES  3

enum{CACHE_GET_ADDR_LIST, CACHE_GET_VAL, 
     CACHE_INVALIDATE, CACHE_USER_PAGE_IS_RDONLY};

typedef struct addr_entry_t      addr_entry_t;
typedef struct addr_summary_t    addr_summary_t;

struct addr_entry_t {
  unsigned long addr;
  int len;
};

struct addr_summary_t {
  char name[1024];
  /* an int for each subcache */
  int  numvals[NUM_SUBCACHES];
};

#endif
