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
/* GNU C varargs support for the TMS320C[34]x  */

/* C[34]x arguments grow in weird ways (downwards) that the standard
   varargs stuff can't handle. */

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST

typedef void *__gnuc_va_list;

#endif /* not __GNUC_VA_LIST */

/* If this is for internal libc use, don't define anything but
   __gnuc_va_list.  */
#if defined (_STDARG_H) || defined (_VARARGS_H)

#ifdef _STDARG_H /* stdarg.h support */

#define va_start(AP,LASTARG) AP=(__gnuc_va_list) __builtin_next_arg (LASTARG)

#else /* varargs.h support */

#define	__va_ellipsis	...
#define	va_alist	__builtin_va_alist
#define	va_dcl		int __builtin_va_alist; __va_ellipsis
#define va_start(AP)	AP=(__gnuc_va_list) ((int *)&__builtin_va_alist +  1)

#endif /* _STDARG_H */

#define va_end(AP)	((void) 0)
#define va_arg(AP,TYPE)	(AP = (__gnuc_va_list) ((char *) (AP) - sizeof(TYPE)), \
			 *((TYPE *) ((char *) (AP))))

#endif /* defined (_STDARG_H) || defined (_VARARGS_H) */
