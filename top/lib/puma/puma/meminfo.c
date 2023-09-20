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
/*****************************************************************************
$Id: meminfo.c,v 1.1 2000/02/24 23:16:30 dwdoerf Exp $

 name:		meminfo()

 purpose:	Parse the /proc/meminfo file for memory usage

 author: 	d.w. doerfler, 9223

 date:		2/10/2000

 parameters:	

 returns:	

 comments:	

 revisions:	

*****************************************************************************/
#include <stdio.h>
#include <string.h>

/* 64-bit quantity */
typedef long meminfo_t;
typedef struct {
  meminfo_t total;
  meminfo_t free;
  meminfo_t shared;
  meminfo_t buffers;
  meminfo_t cached;
  meminfo_t swap_total;
  meminfo_t swap_free;
} MEMINFO;

#define MAXLINE_SIZE 80

int meminfo_(MEMINFO *meminfo)
{
  FILE *fp;
  char buf[MAXLINE_SIZE], qualifier[MAXLINE_SIZE];
  meminfo_t value;

  if ((fp = fopen("enfs:/proc/meminfo", "r")) == NULL)
    return (1);

  memset(meminfo, 0, sizeof(MEMINFO));
  while (fgets(buf, MAXLINE_SIZE, fp) != NULL)
  {
    if (sscanf(buf, "%s %ld", qualifier, &value))
    {
      if (!strncmp("MemTotal:", qualifier, sizeof("MemTotal:")))
        meminfo->total = value << 10;
      else if (!strncmp("MemFree:", qualifier, sizeof("MemFree:")))
        meminfo->free = value << 10;
      else if (!strncmp("MemShared:", qualifier, sizeof("MemShared:")))
        meminfo->shared = value << 10;
      else if (!strncmp("Buffers:", qualifier, sizeof("Buffers:")))
        meminfo->buffers = value << 10;
      else if (!strncmp("Cached:", qualifier, sizeof("Cached:")))
        meminfo->cached = value << 10;
      else if (!strncmp("SwapTotal:", qualifier, sizeof("SwapTotal:")))
        meminfo->swap_total = value << 10;
      else if (!strncmp("SwapFree:", qualifier, sizeof("SwapFree:")))
        meminfo->swap_free = value << 10;
    }
  }

  fclose (fp);
  return(0);
}

int meminfo(MEMINFO *meminfo)
{
  return (meminfo_(meminfo));
}
