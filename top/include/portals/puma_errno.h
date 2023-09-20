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
** $Id: puma_errno.h,v 1.13 2001/11/04 22:10:48 lafisk Exp $
*/

#ifndef PUMA_ERRNO_H
#define PUMA_ERRNO_H

#include <errno.h>
#include "p30/errno.h"

/********************************************************************/

/*
** General Cplant error types.  Error strings are in pumaerr.c.
** Portal specific error types and strings are in 
** ptlerr.h and ptlerr.c.  We print out strings in errorUtil.c.
*/

#define ESUCCESS                0

#define BASE_GEN_ERR       (1)
#define LAST_GEN_ERR       (128)    /* linux errno.h range */

#define BASE_GEN_CP_ERR    (1400)    

#define ESENDTRAP          (BASE_GEN_CP_ERR + 0)  /* send trap failed */
#define EPTLTRAP           (BASE_GEN_CP_ERR + 1)  /* portal action trap */
#define EALIGN             (BASE_GEN_CP_ERR + 2)  /* address alignment*/
#define ELOCK              (BASE_GEN_CP_ERR + 3)  /* memory lock      */
#define EUNLOCK            (BASE_GEN_CP_ERR + 4)  /* memory unlock    */
#define ESYSPTL            (BASE_GEN_CP_ERR + 5)  /* sys_portal_lib trap */
#define EPROTOCOL          (BASE_GEN_CP_ERR + 6)  /* general protocol err*/
#define EPORTAL            (BASE_GEN_CP_ERR + 7)  /* general portal err*/
#define ESENDTIMEOUT       (BASE_GEN_CP_ERR + 8)  /* timeout on send   */
#define ERECVTIMEOUT       (BASE_GEN_CP_ERR + 9)  /* timeout on recv */
#define EPTLTIMEOUT        (BASE_GEN_CP_ERR + 10) /* timeout on portal structure */
#define EOPENCPLANTDEV     (BASE_GEN_CP_ERR + 11) /* opening cplant device file */
#define ERESOURCE          (BASE_GEN_CP_ERR + 12) /* used up a lib resource */
#define EOHHELL            (BASE_GEN_CP_ERR + 13) /* library internal error (i.e. bug) */
#define ECORRUPT           (BASE_GEN_CP_ERR + 14) /* incoming message corrupt */
#define LAST_GEN_CP_ERR    (BASE_GEN_CP_ERR + 14)

/*
** strings defined in pumaerr.c
*/
extern const char *gen_cp_sys_errlist[LAST_GEN_CP_ERR-BASE_GEN_CP_ERR+1];

/*
** Library can define it's own range of errors and strings.  If
** CPerrno is set to one of these, CPstrerror will print out
** the text.  Note that CPerrno is set by portal library functions.
**
** Choose base/last error number in the reserved range of 5000 - 9999.
*/
extern int User_def_base_err;
extern int User_def_last_err;
extern char **User_def_err_strings;

/*
** Using Linux error facility, so don't touch errno.  We may
** have both an errno and a higher level library defined CPerrno 
** to report.
**
** CPerrno is never set back to 0 by the puma library.
*/
extern int CPerrno;

#define ERRNO    CPerrno
#define ERRNO_TRACE(x)

/*
** Print out error string for error number n
*/
extern const char *CPstrerror(int n);

/*
** to print out error strings
**    s1 - s2 : range of error codes
**    n       : code of current error 
**    strings : array of strings associated with error codes
*/
#define TYPE_TO_STRING(s1,s2,n,strings) \
        (( ((int)(n) >= (int)(s1)) && ((int)(n) <= (int)(s2)) ) ? strings[n-s1] : "?")


#define BASE_P3_ERR   (1300)
#define LAST_P3_ERR   (BASE_P3_ERR + PTL_MAX_ERRNO - 1)

char *p30strerror(int errnum);

/********************************************************************/
/********************************************************************/

#endif /* PUMA_ERRNO_H */
