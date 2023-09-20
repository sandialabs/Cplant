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
** $Id: errno.h,v 1.5 2001/01/26 01:38:06 pumatst Exp $
*/
#ifndef _P30_ERRNO_H_
#define _P30_ERRNO_H_


/*
 * p30/errno.h
 *
 * Shared error number lists
 */

#define PTL_ERRORS	\
	A(	PTL_OK			)	\
	A(	PTL_SEGV		)	\
	A(	PTL_NOSPACE		)	\
	A(	PTL_INUSE		)	\
	A(	PTL_VAL_FAILED		)	\
						\
	A(	PTL_NAL_FAILED		)	\
	A(	PTL_NOINIT		)	\
	A(	PTL_INIT_DUP		)	\
	A(	PTL_INIT_INV		)	\
						\
	A(	PTL_AC_INV_INDEX	)	\
	A(	PTL_INV_ASIZE		)	\
	A(	PTL_INV_HANDLE		)	\
	A(	PTL_INV_MD		)	\
	A(	PTL_INV_ME		)	\
	A(	PTL_INV_NI		)	\
	A(	PTL_ILL_MD		)	\
	A(	PTL_INV_PROC		)	\
	A(	PTL_INV_PSIZE		)	\
	A(	PTL_INV_PTINDEX		)	\
	A(	PTL_INV_REG		)	\
	A(	PTL_INV_SR_INDX		)	\
	A(	PTL_ML_TOOLONG		)	\
	A(	PTL_ADDR_UNKNOWN	)	\
						\
	A(	PTL_INV_EQ		)	\
	A(	PTL_EQ_DROPPED		)	\
	A(	PTL_EQ_EMPTY		)	\
						\
	A(	PTL_NOUPDATE		)	\
	A(	PTL_FAIL		)	\
	A(	PTL_NOT_IMPLEMENTED	)	\
	A(	PTL_NO_ACK		)

#define A(value)	value,
typedef enum {
	PTL_ERRORS
	PTL_MAX_ERRNO
} ptl_err_t;

#undef A

extern const char *ptl_err_str[];

#endif
