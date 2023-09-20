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
** $Id: proc_id.h,v 1.7 2001/02/05 02:24:52 lafisk Exp $
*/


/* define the structure containing what we need to talk to yod or 
** pct
**
** IPC type specific definitions
*/

#ifndef PROC_IDH
#define PROC_IDH

#include <idtypes.h>

typedef struct _server_id {
  int           ptl;
  nid_type      nid;
  spid_type     pid; 
} server_id;

#define INVAL           (-1)     /* invalid value for nid, pid or job ID */
#define INVAL_PPID      0xffff   /* invalid value for portal ID */

#endif /* PROC_IDH */
