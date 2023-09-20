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
** $Id: pumaerr.c,v 1.10 2001/05/02 16:18:38 pumatst Exp $
*/
#include "puma_errno.h"

/********************************************************************/
/********************************************************************/
#ifndef LINUX_PORTALS

char *
gen_puma_sys_errlist[LAST_GEN_PUMA_ERR-BASE_GEN_PUMA_ERR+1] = {
    "Q-Kernel returned an error",               /* EQK */
    "Bad address alignment",                    /* EALIGN */
    "nrecv returned -1",                        /* ENRECV */
    "nsend returned -1"                         /* ENSEND */
};
 
#endif !LINUX_PORTALS
 
/********************************************************************/
/********************************************************************/
 
#ifdef LINUX_PORTALS

/*
** General Cplant error strings.  
** Portal specific error types and strings are in 
** ptlerr.h and ptlerr.c.  We print out strings in errorUtil.c.
*/

const char *
gen_cp_sys_errlist[LAST_GEN_CP_ERR-BASE_GEN_CP_ERR+1] = {
    "send trap failure",                    /* ESENDTRAP      */
    "portal action trap failure",           /* EPTLTRAP       */
    "Bad address alignment",                /* EALIGN         */
    "failed memory lock",                   /* ELOCK          */
    "failed memory unlock",                 /* EUNLOCK        */
    "failed sys_portal_lib call",           /* ESYSPTL        */
    "protocol failure",                     /* EPROTOCOL      */
    "portal misbehavior",                   /* EPORTAL        */
    "time out on message send",             /* ESENDTIMEOUT   */
    "time out on message wait",             /* ERECVTIMEOUT   */
    "time out waiting on portal state",     /* EPTLTIMEOUT    */
    "could not open cplant device file",    /* EOPENCPLANTDEV */
    "out of resource in library",           /* ERESOURCE      */
    "library internal error",               /* EOHHELL        */
    "incoming message failed check sum"     /* ECORRUPT       */
};
 

#endif LINUX_PORTALS
 
/********************************************************************/
/********************************************************************/

