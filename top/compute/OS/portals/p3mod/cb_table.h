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
** $Id: cb_table.h,v 1.8 2001/08/22 16:51:31 pumatst Exp $
*/
#ifndef CB_TABLE_H
#define CB_TABLE_H

#include <p30/lib-nal.h>	/* For nal_cb_t */


/*
** We're using taskInfo index from the Portals 2.0 module to index into
** the table of Portals 3.0 CB. Therefore, we need to have at least as
** many entries in our table. Including the ppid.h file should be in
** runtime.[ch] to isolate it from this module.
*/
#include <ppid.h>	/* For NUM_PTL_TASKS */
#define MAX_NUM_CB	(NUM_PTL_TASKS)	/* Max num Portals processes really */

typedef struct   {
    int nid;		/* Portal process (phys) node ID */
    int pid;		/* Portal process ID */
    long spid;		/* (Linux) system pid */
    void *task;		/* The P2 task address */
    nal_cb_t *cb;	/* cb for this pid */
} cb_table_t;


void init_cb(void);
void free_cb(void);
nal_cb_t * index2cb(int index);
nal_cb_t * get_cb(int nid, int pid, void **task, long *spid);
void *get_cb_task(long spid);
void *get_task(int index);
int spid2index(long spid);
nal_cb_t *getclr_cb(long spid);
int enter_cb(nal_cb_t *cb, int index, int nid, int pid);

#endif /* CB_TABLE_H */
