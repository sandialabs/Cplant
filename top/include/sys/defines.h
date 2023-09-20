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
** $Id: defines.h,v 1.4 2001/11/01 22:44:00 pumatst Exp $
**
** This files contains definitions that are used throughout the cplant code.
*/

#ifndef CPLANT_H
#define CPLANT_H

#define TITLE(fname,zmig)


/*
** TRUE and FALSE
*/
#undef TRUE
#define TRUE		(1)
#undef FALSE
#define FALSE		(0)


/*
** Return codes from functions
*/
#undef OK
#define OK		(0)
#undef ERROR
#define ERROR		(-1)



/*
** The GCC macro for a safe max() that works on all types arithmetic types.
*/
#ifndef MAX
#define MAX(a, b)	(a) > (b) ? (a) : (b)
#endif /* MAX */

#ifndef MIN
#define MIN(a, b)	(a) < (b) ? (a) : (b)
#endif /* MIN */

/*
** The rest is from the old qkdefs.h
*/

#ifndef __linux__
#define __inline__
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __osf__
#define PRIVATE static
#define PUBLIC
#endif

#ifndef __osf__
typedef unsigned char           uchar;
#endif

typedef char                    CHAR;
typedef unsigned char           UCHAR;
typedef char                    INT8;
typedef unsigned char           UINT8;
typedef short int               INT16;
typedef unsigned short int      UINT16;
typedef int                     INT32;
typedef unsigned int            UINT32;
typedef long                    LONG32;
typedef unsigned long           ULONG32;

/* long may be 32 or 64, so we can't really append the size to the definition */
typedef long                    LONG;
typedef unsigned long           ULONG;

#ifdef __alpha__
typedef long int_t;
#ifndef __osf__
typedef unsigned long uint_t;
#endif
#endif

#ifdef __i386__
typedef int int_t;
typedef unsigned int uint_t;
#endif

typedef float                   FLOAT32;
typedef double                  FLOAT64;
typedef void                    VOID;
typedef INT32                   BOOLEAN;
typedef void (*FCN_PTR)(void);

#ifndef off64_t

#if defined (__alpha__) || defined (__ia64__)
typedef long                     off64_t;
#else
typedef long long                off64_t;
#endif

#endif

/*
** Process related typedefs
*/
typedef UINT16 PID_TYPE;  /* Type of Local process ID */
typedef UINT16 NID_TYPE;  /* Type of Physical node ID */
typedef UINT16 GID_TYPE;  /* Type of Group ID */
typedef UINT16 RANK_TYPE; /* Type of Logical rank/process within a group */



#endif /* CPLANT_H */
