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
** bug fix at line 262 to fix access of for_msg.cat under linux
** this bug was reported by USI with a patch to maintainers of glibc so
** our wrapper in libgrp_io should be removed when we upgrade glibc
*/

/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.org>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef __linux__

#define _LIBC

#include <byteswap.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#undef _POSIX_MAPPED_FILES
#ifdef _POSIX_MAPPED_FILES
# include <sys/mman.h>
#endif
#include <sys/stat.h>

#include "catgetsinfo.h"


#define SWAPU32(w) bswap_32 (w)


void
__open_catalog (__nl_catd catalog)
{
  int fd = -1;
  struct stat st;
  int swapping;
  size_t cnt;
  size_t max_offset;
  size_t tab_size;
  const char *lastp;

  /* Make sure we are alone.  */
  __libc_lock_lock (catalog->lock);

  /* Check whether there was no other thread faster.  */
  if (catalog->status != closed)
    /* While we waited some other thread tried to open the catalog.  */
    goto unlock_return;

  if (strchr (catalog->cat_name, '/') != NULL || catalog->nlspath == NULL)
    fd = __open (catalog->cat_name, O_RDONLY);
  else
    {
      const char *run_nlspath = catalog->nlspath;
#define ENOUGH(n)							      \
  if (bufact + (n) >=bufmax) 						      \
    {									      \
      char *old_buf = buf;						      \
      bufmax += 256 + (n);						      \
      buf = (char *) alloca (bufmax);					      \
      memcpy (buf, old_buf, bufact);					      \
    }

      /* The RUN_NLSPATH variable contains a colon separated list of
	 descriptions where we expect to find catalogs.  We have to
	 recognize certain % substitutions and stop when we found the
	 first existing file.  */
      char *buf;
      size_t bufact;
      size_t bufmax;
      size_t len;

      buf = NULL;
      bufmax = 0;

      fd = -1;
      while (*run_nlspath != '\0')
	{
	  bufact = 0;

	  if (*run_nlspath == ':')
	    {
	      /* Leading colon or adjacent colons - treat same as %N.  */
	      len = strlen (catalog->cat_name);
	      ENOUGH (len);
	      memcpy (&buf[bufact], catalog->cat_name, len);
	      bufact += len;
	    }
	  else
	    while (*run_nlspath != ':' && *run_nlspath != '\0')
	      if (*run_nlspath == '%')
		{
		  const char *tmp;

		  ++run_nlspath;	/* We have seen the `%'.  */
		  switch (*run_nlspath++)
		    {
		    case 'N':
		      /* Use the catalog name.  */
		      len = strlen (catalog->cat_name);
		      ENOUGH (len);
		      memcpy (&buf[bufact], catalog->cat_name, len);
		      bufact += len;
		      break;
		    case 'L':
		      /* Use the current locale category value.  */
		      len = strlen (catalog->env_var);
		      ENOUGH (len);
		      memcpy (&buf[bufact], catalog->env_var, len);
		      bufact += len;
		      break;
		    case 'l':
		      /* Use language element of locale category value.  */
		      tmp = catalog->env_var;
		      do
			{
			  ENOUGH (1);
			  buf[bufact++] = *tmp++;
			}
		      while (*tmp != '\0' && *tmp != '_' && *tmp != '.');
		      break;
		    case 't':
		      /* Use territory element of locale category value.  */
		      tmp = catalog->env_var;
		      do
			++tmp;
		      while (*tmp != '\0' && *tmp != '_' && *tmp != '.');
		      if (*tmp == '_')
			{
			  ++tmp;
			  do
			    {
			      ENOUGH (1);
			      buf[bufact++] = *tmp++;
			    }
			  while (*tmp != '\0' && *tmp != '.');
			}
		      break;
		    case 'c':
		      /* Use code set element of locale category value.  */
		      tmp = catalog->env_var;
		      do
			++tmp;
		      while (*tmp != '\0' && *tmp != '.');
		      if (*tmp == '.')
			{
			  ++tmp;
			  do
			    {
			      ENOUGH (1);
			      buf[bufact++] = *tmp++;
			    }
			  while (*tmp != '\0');
			}
		      break;
		    case '%':
		      ENOUGH (1);
		      buf[bufact++] = '%';
		      break;
		    default:
		      /* Unknown variable: ignore this path element.  */
		      bufact = 0;
		      while (*run_nlspath != '\0' && *run_nlspath != ':')
			++run_nlspath;
		      break;
		    }
		}
	      else
		{
		  ENOUGH (1);
		  buf[bufact++] = *run_nlspath++;
		}

	  ENOUGH (1);
	  buf[bufact] = '\0';

	  if (bufact != 0)
	    {
	      fd = __open (buf, O_RDONLY);
	      if (fd >= 0)
		break;
	    }

	  ++run_nlspath;
	}
    }

  /* Avoid dealing with directories and block devices */
  if (fd < 0)
    {
      catalog->status = nonexisting;
      goto unlock_return;
    }

  if (__fxstat (_STAT_VER, fd, &st) < 0)
    {
      catalog->status = nonexisting;
      goto close_unlock_return;
    }
  if (!S_ISREG (st.st_mode) || st.st_size < sizeof (struct catalog_obj))
    {
      /* `errno' is not set correctly but the file is not usable.
	 Use an reasonable error value.  */
      __set_errno (EINVAL);
      catalog->status = nonexisting;
      goto close_unlock_return;
    }

  catalog->file_size = st.st_size;
#ifdef _POSIX_MAPPED_FILES
# ifndef MAP_COPY
    /* Linux seems to lack read-only copy-on-write.  */
#  define MAP_COPY MAP_PRIVATE
# endif
# ifndef MAP_FILE
    /* Some systems do not have this flag; it is superfluous.  */
#  define MAP_FILE 0
# endif
# ifndef MAP_INHERIT
    /* Some systems might lack this; they lose.  */
#  define MAP_INHERIT 0
# endif
  catalog->file_ptr =
    (struct catalog_obj *) __mmap (NULL, st.st_size, PROT_READ,
				   MAP_FILE|MAP_COPY|MAP_INHERIT, fd, 0);
  if (catalog->file_ptr != (struct catalog_obj *) MAP_FAILED)
    /* Tell the world we managed to mmap the file.  */
    catalog->status = mmapped;
  else
#endif /* _POSIX_MAPPED_FILES */
    {
      /* mmap failed perhaps because the system call is not
	 implemented.  Try to load the file.  */
      size_t todo;
      catalog->file_ptr = malloc (st.st_size);
      if (catalog->file_ptr == NULL)
	{
	  catalog->status = nonexisting;
	  goto close_unlock_return;
	}
      todo = st.st_size;
      /* Save read, handle partial reads.  */
      do
	{
	  // following line cause sigsegv later
	  //size_t now = __read (fd, (((char *) &catalog->file_ptr)
	  size_t now = __read (fd, (((char *) catalog->file_ptr)
				    + (st.st_size - todo)), todo);
	  if (now == 0)
	    {
	      free ((void *) catalog->file_ptr);
	      catalog->status = nonexisting;
	      goto close_unlock_return;
	    }
	  todo -= now;
	}
      while (todo > 0);
      catalog->status = malloced;
    }

  /* Determine whether the file is a catalog file and if yes whether
     it is written using the correct byte order.  Else we have to swap
     the values.  */
  if (catalog->file_ptr->magic == CATGETS_MAGIC)
    swapping = 0;
  else if (catalog->file_ptr->magic == SWAPU32 (CATGETS_MAGIC))
    swapping = 1;
  else
    {
    invalid_file:
      /* Invalid file.  Free the resources and mark catalog as not
	 usable.  */
#ifdef _POSIX_MAPPED_FILES
      if (catalog->status == mmapped)
	__munmap ((void *) catalog->file_ptr, catalog->file_size);
      else
#endif	/* _POSIX_MAPPED_FILES */
	free (catalog->file_ptr);
      catalog->status = nonexisting;
      goto close_unlock_return;
    }

#define SWAP(x) (swapping ? SWAPU32 (x) : (x))

  /* Get dimensions of the used hashing table.  */
  catalog->plane_size = SWAP (catalog->file_ptr->plane_size);
  catalog->plane_depth = SWAP (catalog->file_ptr->plane_depth);

  /* The file contains two versions of the pointer tables.  Pick the
     right one for the local byte order.  */
#if __BYTE_ORDER == __LITTLE_ENDIAN
  catalog->name_ptr = &catalog->file_ptr->name_ptr[0];
#elif __BYTE_ORDER == __BIG_ENDIAN
  catalog->name_ptr = &catalog->file_ptr->name_ptr[catalog->plane_size
						  * catalog->plane_depth
						  * 3];
#else
# error Cannot handle __BYTE_ORDER byte order
#endif

  /* The rest of the file contains all the strings.  They are
     addressed relative to the position of the first string.  */
  catalog->strings =
    (const char *) &catalog->file_ptr->name_ptr[catalog->plane_size
					       * catalog->plane_depth * 3 * 2];

  /* Determine the largest string offset mentioned in the table.  */
  max_offset = 0;
  tab_size = 3 * catalog->plane_size * catalog->plane_depth;
  for (cnt = 2; cnt < tab_size; cnt += 3)
    if (catalog->name_ptr[cnt] > max_offset)
      max_offset = catalog->name_ptr[cnt];

  /* Now we can check whether the file is large enough to contain the
     tables it says it contains.  */
  if (st.st_size <= (sizeof (struct catalog_obj) + 2 * tab_size + max_offset))
    /* The last string is not contained in the file.  */
    goto invalid_file;

  lastp = catalog->strings + max_offset;
  max_offset = (st.st_size
		- sizeof (struct catalog_obj) + 2 * tab_size + max_offset);
  while (*lastp != '\0')
    {
      if (--max_offset == 0)
	goto invalid_file;
      ++lastp;
    }

  /* Release the lock again.  */
 close_unlock_return:
  __close (fd);
 unlock_return:
  __libc_lock_unlock (catalog->lock);
}
#endif /* __linux__ */
