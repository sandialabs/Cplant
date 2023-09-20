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
#ifndef _CPLANT_TASK_
#define _CPLANT_TASK_

#include "ppid.h"

enum{CPL_SET_PHYS_NID, CPL_GET_PHYS_NID, 
     CPL_SET_NETWORK, CPL_COPY_NETWORK,
     CPL_SET_NETMASK, CPL_COPY_NETMASK,
     CPL_SYS_APCB,
     CPL_MLOCKALL, CPL_MLOCK, CPL_MUNLOCK,
     CPL_SET_NID_MAP, CPL_SET_PID_MAP,
     CPL_GET_NUM_PHYS_NODES, CPL_SET_NUM_PHYS_NODES,
     CPL_CLEAR_DEAD};


/* entries for portals task table */
typedef struct {
    int                        free;      /* whether or not */
    ppid_type                  ppid;      /* portal */
    nid_type                   pnid;      /* physical node id */
    nid_type                   rid;       /* rank id */
    nid_type                   nprocs;    /* size of group */
    gid_type                   gid;       /* group */
    spid_type                  spid;      /* system */
    struct task_struct        *task;    
    char name[1024];
} taskInfo_t;

extern int cTaskGetNumPhysNodes(void);
extern int cTaskGetPhysNid(void);
extern unsigned int cTaskGetNetwork(void);
extern unsigned int cTaskGetNetmask(void);

extern void clearTheDead(void);

extern taskInfo_t* cTaskGetCurrentEntry( void );
extern taskInfo_t *cTaskGetEntry( int index );
extern char *cTaskGetName( ppid_type ppid );
extern int cTaskGetRank( ppid_type ppid );

extern struct task_struct *cTaskGetTask( ppid_type ppid );

extern ppid_type cTaskGetCurrentPPID( void );
extern ppid_type cTaskSet_dynamic( taskInfo_t *info);
extern ppid_type cTaskSet_static(  taskInfo_t *info);

extern spid_type ppid2spid( ppid_type ppid );
extern int getSpidIndex( spid_type spid );

extern void cTaskInit( void );
extern int cTaskClear( ppid_type ppid );

extern int rank2pnid(int nid, taskInfo_t *taskInfo);
extern int rank2ppid(int nid, taskInfo_t *taskInfo);

#endif
