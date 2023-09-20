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
/* $Id: shrink_path.c,v 1.4 2002/02/12 18:51:13 pumatst Exp $ */

#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include "puma.h"
#include "errno.h"
#include "shrink_path.h"

/******************************************************************************/

/*
** This function eliminates double slashes (//), slash-dots (/./), and
** douple dots (/../) from a file name path. The function expects a
** pointer to a full path (starting with /) as its argument. It returns
** NULL in case of an error, or a pointer to the newly formed string.
**
**
**
** state         |     /      |       .       |     \0      |   any
** --------------+------------+---------------+-------------+-------------
** START_STATE   | out <- in  | error         | error       | error
**               | SEEN_SLASH | return NULL   | return NULL | return NULL
**
** SEEN_SLASH    | no-op      | no-op         | go back 1   | out <- in 
**               | SEEN_SLASH | SLASH_DOT     | return str  | COPY_CHAR
**
** COPY_CHAR     | out <- in  | out <- in     | out <- in   | out <- in 
**               | SEEN_SLASH | COPY_CHAR     | return str  | COPY_CHAR
**
** SLASH_DOT     | out <- in  | out <- in     | go back 1   | out<-'.'<-in
**               | SEEN_SLASH | SLASH_DOT_DOT | return str  | COPY_CHAR
**
** SLASH_DOT_DOT | <-- to /   | out<-'.'<-in  | <-- to /    | out<-'.'<-in
**               | SEEN_SLASH | COPY_CHAR     | return str  | COPY_CHAR
*/



static CHAR temp[MAXPATHLEN];


CHAR *
shrink_path(CHAR *path)
{

#define START_STATE	(1)
#define SEEN_SLASH	(2)
#define COPY_CHAR	(3)
#define SLASH_DOT	(4)
#define SLASH_DOT_DOT	(5)

INT32 state= 1;
CHAR *in= path;
CHAR *out= temp;
CHAR ch;


    while (1)   {
	switch (state)   {
	case START_STATE:
		if ((*out++= *in++) == '/')   {
			state= SEEN_SLASH;
		} else {
			return NULL;
		}
		break;

	case SEEN_SLASH:
		if (*in == '.')   {
			state= SLASH_DOT;
		} else if (*in == '\0')   {
			*(--out)= '\0';
			return temp;
		} else if (*in != '/')   {
			*out++= *in;
			state= COPY_CHAR;
		}
		in++;
		break;

	case COPY_CHAR:
		ch= *in;
		*out++= *in++;
		if (ch == '/')   {
			state= SEEN_SLASH;
		} else if (ch == '\0')   {
			return temp;
		}
		break;

	case SLASH_DOT:
		if (*in == '.')   {
			*out++= *in;
			state= SLASH_DOT_DOT;
		} else if (*in == '/')   {
			state= SEEN_SLASH;
		} else if (*in == '\0')   {
			*(--out)= '\0';
			return temp;
		} else {
			*out++= '.';
			*out++= *in;
			state= COPY_CHAR;
		}
		in++;
		break;

	case SLASH_DOT_DOT:
		if (*in == '\0')   {
			if ((out - temp) >= 3) {
				out= out - 3;
				while ((out > (temp+1)) && (*out != '/'))   {
					out--;
				}
			} else {
				/*
				** out looks like "/.", so just get rid of the "."
				*/
				out--;
			}
			*out= '\0';
			return temp;
		} else if (*in == '/')   {
			if ((out - temp) >= 3) {
				out= out - 3;
				while ((out > (temp+1)) && (*out != '/'))   {
					out--;
				}
			} else {
				/*
				** out looks like "/.", so just get rid of the "."
				*/
				out--;
			}
			state= SEEN_SLASH;
		} else {
			*out++= '.';
			*out++= *in;
			state= COPY_CHAR;
		}
		in++;
		break;
	}
    }
}  /* end of shrink_path() */

