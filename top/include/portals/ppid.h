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
 * TITLE(ppid_h, "@(#) $Id: ppid.h,v 1.14 2001/04/01 02:31:20 pumatst Exp $");
 */

#ifndef _INCppidh_
#define _INCppidh_

#include "idtypes.h"


#define MAX_PPID         1000    /* this needs to fit into 16 bits so the 
                                    maximum value is 65535. having it "large"
                                    can help w/ debugging process accounting
                                    but there are reasons for making it 
                                    somewhat smaller than the maximum --
                                    requiring storage for arrays that index 
                                    on the ppid, eg...  */
                                 
#define MAX_GID          1000    /* this needs to fit into 16 bits... */

#define MAX_FIXED_PPID   100
#define MAX_FIXED_GID    100
#define PPID_FLOATING    MAX_FIXED_PPID+1   /* Floating area starts here */
#define GID_FLOATING     MAX_FIXED_GID+1    /* Floating area starts here */
#define NUM_PTL_TASKS    MAX_FIXED_PPID+80  /* Maximum no. portals tasks */

#define PPID_AUTO        0

/* Minimum PPID is 1 */
#define PPID_BEBOPD      1            /* bebopd */
#define  GID_BEBOPD      1            /* bebopd */

#define PPID_PCT         2            /* pct */
#define  GID_PCT         2            /* pct */

#define PPID_FYOD        3            /* fyod */
#define  GID_FYOD        3            /* fyod */

#define PPID_GDBWRAP     11           /* portals proxy for gdb */
#define  GID_GDBWRAP     11           /* portals proxy for gdb */

#define PPID_TEST        15           /* for portals tests */
#define  GID_TEST        15

#define  GID_YOD         5            /* yod */
#define  GID_PINGD       6            /* pingd */
#define  GID_BT          7            /* bt */
#define  GID_PTLTEST     8            /* ptltest */
#define  GID_CGDB        9            /* cgdb */
#define  GID_TVDSVR     10            /* start-tvdsvr */

#endif /* _INCppidh_ */
