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
/* CYGNUS LOCAL entire file/law */
/* Define __gnuc_va_list. */

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef void *__gnuc_va_list;
#endif /* not __GNUC_VA_LIST */

/* If this is for internal libc use, don't define anything but
   __gnuc_va_list.  */
#if defined (_STDARG_H) || defined (_VARARGS_H)
#define __gnuc_va_start(AP) (AP = (__gnuc_va_list)__builtin_saveregs())
#define __va_ellipsis ...

#ifdef _STDARG_H
#define va_start(AP, LASTARG) \
 (AP = ((__gnuc_va_list) __builtin_next_arg (LASTARG)))
#else
#define va_alist __builtin_va_alist
#define va_dcl int __builtin_va_alist; __va_ellipsis
#define va_start(AP)  AP=(char *) &__builtin_va_alist
#endif

/* Now stuff common to both varargs & stdarg implementations.  */
#define __va_rounded_size(TYPE)						\
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))
#undef va_end
void va_end (__gnuc_va_list);
#define va_end(AP) ((void)0)
#define va_arg(AP, TYPE)						\
 (sizeof (TYPE) > 8							\
  ? (AP = (__gnuc_va_list) ((char *) (AP) + __va_rounded_size (char *)),\
    **((TYPE **) (void *) ((char *) (AP) - __va_rounded_size (char *))))\
  : (AP = (__gnuc_va_list) ((char *) (AP) + __va_rounded_size (TYPE)),	\
    *((TYPE *) (void *) ((char *) (AP) - __va_rounded_size (TYPE)))))
#endif
/* END CYGNUS LOCAL */
