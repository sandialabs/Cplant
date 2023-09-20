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
/* $Id: fyodCompat.c,v 1.7 2001/09/26 07:09:54 lafisk Exp $ */

/*****************************************************************************
This globals are used by functions that are called by yod and not fyod.
However since we link everything into fyod we need to define them. They are 
define  in yod.c
*****************************************************************************/

int *physnid2rank;
int *rank2physnid;
int daemonWaitLimit;

int Server_done;

int prog_phase = 0;

int nodes_running;

void *un_ids;
