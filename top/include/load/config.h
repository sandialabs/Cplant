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
** $Id: config.h,v 1.26 2002/01/16 00:06:14 jrstear Exp $
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include "puma.h"
/*
*******************************************************************************
**  Node name file
*******************************************************************************
*/
int phys2name(int physnid);
int phys_to_SU_number(int physnid);

//void print_node_names(void);

/*
*******************************************************************************
**  site specific path names
*******************************************************************************
*/
const char *node_names_file_name(void);
const char *bebopd_restart_file(void);
const char *ram_disk(void);
const char *disk_disk(void);
const char *log_file_name(void);
const char *notify_list(void);
const char *vm_name_file(void);
const char *cplant_host_file(void);
const char *solicit_pcts_tmout(void);
const char *pfs(void);
const char *pct_health_check(void);

#ifdef TWO_STAGE_COPY
const char *vm_global_storage(void);
const char *vm_global_from_srv_node(void);
const char *su_global_storage(void);
const char *local_exec_path(void);
const char *single_level_global_storage(void);
const char *single_level_exec_path(void);
const char *su_name_format(void);
const char *vm_global_machine(void);
#endif

const char *pbs_prefix(void);
const char *daemon_timeout(void);
const char *client_timeout(void);
const char *nice_kill_interval(void);

void refresh_config(void);   /* re-read site file */

#define NO_VM  "none"
#define MAX_SU 30
#define NAME_PREFIX "exec-"

/*
*******************************************************************************
** For backward compatibility with libutil.a
*******************************************************************************
*/
#define MAX_WIDTH 20
#define MAX_HEIGHT 25

#define MAX_MESH_SIZE (MAX_WIDTH*MAX_HEIGHT)

/*
*******************************************************************************
** parse a node list
*******************************************************************************
*/
int parse_node_list(char *inlist, int *outlist, int size, int min, int max);
int simpleNodeRange(int *alist, int size);
int print_node_list(FILE *fp, int *nids, int size, int width, int lmargin);
int write_node_list(nid_type *nids, int size, char *buf, int len);
int findInList(char *list, int nodenum);




#endif /* CONFIG_H */
