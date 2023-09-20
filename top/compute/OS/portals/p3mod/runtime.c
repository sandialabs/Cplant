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
** $Id: runtime.c,v 1.19 2001/08/22 16:51:31 pumatst Exp $
** Portals 3.0 module file that provides runtime services to the P3 module
** Right now these are functions that call other functions in the old
** Portals 2.0 module. Eventually, there should be a seperate Cplant module
** that deals with process allocation, memory mapping, etc. When that
** comes along, all we have to change in the P3 module is contained in
** this file.
*/

#include <asm/uaccess.h>		/* For get_user() */
#include <sys/defines.h>                /* For TRUE */
#include <cTask/cTask.h>		/* For getSpidIndex */
#include <base/machine.h>		/* For memcpy2() */
#include <ppid.h>			/* For NUM_PTL_TASKS */

#include "runtime.h"


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
/*
** Is this process registered with the Portals 2.0 module?
*/
int
checktask(void)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	printk("%s: checktask() No Portals task found for pid %d\n", __FILE__,
	    current->pid);
	return FALSE;
    }

    #ifdef VERBOSE
    printk("P3 checktask() ppid %d, nprocs %d, spid %d, gid %d\n",
	taskInfo->ppid, taskInfo->nprocs, taskInfo->spid, taskInfo->gid);
    #endif /* VERBOSE */

    return TRUE;

}  /* end of checktask() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
getpid(void)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: getpid() returning %d\n", __FILE__, taskInfo->ppid);
    #endif /* VERBOSE */

    return taskInfo->ppid;

}  /* end of getpid() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
getnum_nodes(void)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: getnum_nodes() returning %d\n", __FILE__, taskInfo->nprocs);
    #endif /* VERBOSE */

    return taskInfo->nprocs;

}  /* end of getnum_nodes() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
getgid(void)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: getgid() returning %d\n", __FILE__, taskInfo->gid);
    #endif /* VERBOSE */

    return taskInfo->gid;

}  /* end of getgid() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** The rank of a process within its group
*/
int
getrank(void)
{
taskInfo_t *taskInfo;

    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: getrank() returning %d\n", __FILE__, taskInfo->rid);
    #endif /* VERBOSE */

    return taskInfo->rid;

}  /* end of getrank() */

/*
** Return the physical node ID
*/
int
getnid(void)
{
taskInfo_t *taskInfo;

    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: getnid() returning %d\n", __FILE__, taskInfo->pnid);
    #endif /* VERBOSE */

    return taskInfo->pnid;

}  /* end of getnid() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
getTskIndex(long spid)
{
    return getSpidIndex(spid);
}  /* end of getTskIndex() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void *
getTsk(int ppid)
{
    return (void *)cTaskGetTask(ppid);
}  /* end of getTsk() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3rank2pnid(int nid)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: p3rank2pnid() nid %d returning %d\n", __FILE__, nid,
	rank2pnid(nid, taskInfo));
    #endif /* VERBOSE */

    return rank2pnid(nid, taskInfo);

}  /* end of p3rank2pnid() */


int
p3rank2ppid(int nid)
{

taskInfo_t *taskInfo;


    taskInfo= cTaskGetCurrentEntry();
    if (taskInfo == NULL)   {
	return -1;
    }

    #ifdef VERBOSE
    printk("%s: p3rank2ppid() nid %d returning %d\n", __FILE__, nid,
	rank2ppid(nid, taskInfo));
    #endif /* VERBOSE */

    return rank2ppid(nid, taskInfo);

}  /* end of p3rank2ppid() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
memcpy2user(void *task, void *to, void *from, size_t len)
{
    int retval = 0;
    __gcc_barrier();
    if (memcpy2(TO_USER, task, to, from, len) != 0)   {
	printk("memcpy2() from %p to %p (len %ld) failed. Process gone?\n",
	    from, to, len);
      retval = -1;
    }
    __gcc_barrier();
    return retval;
}  /* end of memcpy2user() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
