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
** $Id: yod.h,v 1.5 2001/02/16 05:44:34 lafisk Exp $
*/
#ifndef YOD_H
#define YOD_H

#include "portal.h"

extern int phys2log_map[];

/*
** Defined in yod_comm.c
*/
int initialize_yod(int nprocs, nid_type *nmap);
VOID takedown_yod_portals();

int send_init_pct_message(int app_rank, argenv_msg *lmsg);
int get_next_pct_msg(pct2yod_msg *msg, int tmout);
int send_global_pid_map(int app_rank, pid_map_msg *pmap);
int send_go_main(int app_rank, int job_id);
int send_all_done(int app_rank, int job_id);
int send_abort_load(int app_rank, int job_id);

int get_next_application_msg(hostCmd_t *msg, int tmout);

#endif /* YOD_H */
