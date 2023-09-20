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
/* register.c - Portal registration */
/*
** $Id: register.c,v 1.14 2001/04/01 23:47:22 pumatst Exp $
*/

#include <stdio.h>
#include <sys_portal_lib.h>
#include <string.h>
#include "puma.h"
#include "cTask/cTask.h"
#include "ppid.h"

ppid_type register_ppid(taskInfo_t *info, ppid_type ppid, gid_type gid,
                        char *name)
{
  /* ppid should be 0 on input unless it specifies
     a must-have fixed pid
  */

  if ( info == NULL ) {
    return 0;           /* 0 is an invalid ppid */
  }

  /* Put requested ppid and gid in pcb */
  info->ppid     = ppid;
  info->gid      =  gid;

  if ( name ) {
    strcpy( info->name, name);
  }
  else {
    strcpy( info->name, "unknown");
  }

  if ( ntv_sys(__NR_ioctl, cplant_fd, CPL_SYS_APCB, 
                                   (unsigned long)info) < 0 ){
    return 0;
  }

  /* Load the global variable in case the app. writer forgets to */
  _my_ppid = info->ppid;

  /* Return the actual ppid */
  return info->ppid;
}
