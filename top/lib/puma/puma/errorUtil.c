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
$Id: errorUtil.c,v 1.10 2001/09/21 16:45:43 pumatst Exp $
*/

#ifdef LINUX_PORTALS

#include <string.h>
#include <stdio.h>
#include "puma_errno.h"
#include <p30/errno.h>

 
int User_def_base_err = 0;
int User_def_last_err = -1;
char **User_def_err_strings;

static const char *cp_noerror = "No CP error";
static const char *cp_unknown = "CP Error unknown";

const char *
CPstrerror(int errnum)
{
const char *rstr;
 
    if (errnum == 0){
        rstr = cp_noerror;
    }
    else if ((errnum >= BASE_GEN_ERR) && (errnum <= LAST_GEN_ERR)) {
        rstr = strerror(errnum);
    }
    else if ((errnum >= BASE_GEN_CP_ERR) && (errnum <= LAST_GEN_CP_ERR)) {
        rstr = gen_cp_sys_errlist[errnum-BASE_GEN_CP_ERR];
    }
    else if ((errnum >= BASE_P3_ERR) && (errnum <= LAST_P3_ERR)) {
        rstr = ptl_err_str[errnum - BASE_P3_ERR];
    }
    else if ((errnum >= User_def_base_err) && (errnum <= User_def_last_err)) {
        rstr = User_def_err_strings[errnum-User_def_base_err];
    }
    else{
        rstr = cp_unknown;
    }
    return rstr;
}
/*
** errnum is the return value of a p30 library call
*/
char *
p30strerror(int errnum)
{
 
    if ( (errnum < 0) || (errnum >= PTL_MAX_ERRNO)){
        return "unknown p30 error";
    }
    else {
        return (char *)ptl_err_str[errnum];
    }
}

#endif /* LINUX_PORTALS */

