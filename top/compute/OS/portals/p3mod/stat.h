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
** $Id
** Statistics about the P3 module.
*/

#ifndef P3_STAT_H
#define P3_STAT_H

#include <p30/lib-dispatch.h>	/* For LIB_MAX_DISPATCH */


typedef struct   {
    unsigned long NIInitOK;		/* Good open() */
    unsigned long NIInitBAD;		/* Bad open() */
    unsigned long NIFini;		/* close() */
    unsigned long FwdOK;		/* Good ioctl() (fwd) */
    unsigned long FwdBAD1;		/* Access to usr lvl myrnal_fwd failed*/
    unsigned long FwdBAD2;		/* Args to fwd too long */
    unsigned long FwdBAD3;		/* Return arg too long */
    unsigned long FwdBAD4;		/* Access to fwd args failed */
    unsigned long FwdBAD5;		/* Access to return args failed */
    unsigned long FwdBAD6;		/* Disptach cmd out of range */
    unsigned long FwdBAD7;		/* Can't get index for spid */
    unsigned long FwdBAD8;		/* Writing to return args failed */
    unsigned long ioctlBad;		/* Access to icotl args failed */
    unsigned long ioctlOK;		/* Good icotl() */
    unsigned long ioctlMlockall;	/* Mlock_all calls */
    unsigned long ioctlUnknown;		/* Unknown ioctl calls */
    unsigned long FwdArgLen;		/* Fwd argument list length */
    unsigned long FwdArgLenMax;		/* Max fwd argument list length */
    unsigned long FwdArgLenMin;		/* Min fwd argument list length */
    unsigned long FwdRetLen;		/* Fwd return argument list length */
    unsigned long FwdRetLenMax;		/* Max return fwd argument list len */
    unsigned long FwdRetLenMin;		/* Min return fwd argument list len */
    unsigned long dispatch[LIB_MAX_DISPATCH];	/* Func called through fwd */
} nalstat_t;

extern nalstat_t nalstat;

void p3_stat_init(void);
void p3_stat_proc(char **pb_ptr);

#endif /* P3_STAT_H */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
