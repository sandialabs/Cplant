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

TITLE(fyod_h, "@(#) $Id: fyod.h,v 1.9 2001/02/16 05:21:48 lafisk Exp $");

#include <stdio.h>
#include <sys/types.h>
#include "host_msg.h"
#include "fyod_map.h"

extern void fyodInitWhoIam(void);

/* I-O Units that I am attached to  */
int n_fyod_units;
int fyod_units[MAX_DISKS_PER_FYOD_NODE];

/* Proto-types from fyod_fyod_comm.c    */

void fyod_comm_init(void);
void fyod_setup(void);
int I_have(int unit);


/* Proto-types from fyod_comm.c    */

int host_cmd_init(void);
INT32 get_next_cmd(control_msg_handle *mh);
INT32 free_cmd(control_msg_handle *mh);
