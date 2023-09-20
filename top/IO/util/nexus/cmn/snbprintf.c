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
 * snbprintf: wrapper around GNU snprintf() functions.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>

int
snbprintf (char *str, size_t n, const char *format, ... )
{
	int		ret;
	va_list		args;

	va_start(args, format);

	ret = vsnprintf(str, n, format, args);

	va_end(args);

	return ret;
}

int
vsnbprintf (char *str, size_t n, const char *format, va_list ap)
{
	return vsnprintf(str, n, format, ap);
}

