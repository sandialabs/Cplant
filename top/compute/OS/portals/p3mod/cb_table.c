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
** $Id: cb_table.c,v 1.18.4.1 2002/05/15 00:55:37 ktpedre Exp $
*/

#include <linux/slab.h>		/* For kmalloc() */
#include <lib-p30.h>
#include "runtime.h"
#include "cb_table.h"

#define VERBOSE
#undef VERBOSE


static cb_table_t cb_table[MAX_NUM_CB];

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** Each Portals 3 process has it's own control block.
** The P3 module keeps a table of cb's, because each process that
** initialized an interface, has a different rank, pid, group, and
** number of processes in that group.
*/
void
init_cb(void)
{

int i;


    for (i= 0; i < MAX_NUM_CB; i++)   {
	cb_table[i].nid= -1;
	cb_table[i].pid= -1;
	cb_table[i].spid= -1;
	cb_table[i].task= NULL;
	cb_table[i].cb= NULL;
    }

}  /* end of init_cb() */


/*
** Free all memory associated with the cb table
*/
void
free_cb(void)
{

int i;
nal_cb_t *cb;


    for (i= 0; i < MAX_NUM_CB; i++)   {
	cb= cb_table[i].cb;
	if (cb != NULL)   {
	    if (cb->nal_data != NULL)   {
		kfree(cb->nal_data);
	    }
	    if (cb->ni.tbl.tbl != NULL)   {
		kfree(cb->ni.tbl.tbl);
	    }
	    kfree(cb);
	    cb_table[i].nid= -1;
	    cb_table[i].pid= -1;
	    cb_table[i].spid= -1;
	    cb_table[i].task= NULL;
	    cb_table[i].cb= NULL;
	}
    }

}  /* end of free_cb() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/* This function returns a pointer to the i'th cb. */
nal_cb_t *
index2cb(int index)
{

    if ((index < 0) || (index >= MAX_NUM_CB))   {
	return NULL;
    } else   {
	return cb_table[index].cb;
    }

}  /* end of index2cb() */

/*
** Use current->spid because spid2index() may not work if the process
** closes /dev/cTask first.
*/
void *
get_cb_task(long spid)
{

int i;


    for (i= 0; i < MAX_NUM_CB; i++)   {
	if (cb_table[i].spid == spid)   {
	    return cb_table[i].task;
	}
    }
    return NULL;

}  /* end of get_cb_task() */

/* This function returns a pointer to the cb belonging to rid, gid. */
nal_cb_t *
get_cb(int nid, int pid, void **task, long *spid)
{

int i;


    for (i= 0; i < MAX_NUM_CB; i++)   {
	if ((cb_table[i].nid == nid) && (cb_table[i].pid == pid))   {
	    if (task != NULL)   {
		*task= cb_table[i].task;
	    }
	    if (spid != NULL)   {
		*spid= cb_table[i].spid;
	    }
	    return cb_table[i].cb;
	}
    }
    return NULL;

}  /* end of get_cb() */


int
spid2index(long spid)
{
    /*
    ** spid is the Linux (system) process ID. This is really the only
    ** trustable piece of information we have when we get called via
    ** open(), close(), and icotl(). The runtime environment must
    ** figure out a way to translate that into an idex we can use for
    ** our cb table.
    */
    return getTskIndex(spid);

}  /* end of spid2index() */


/* Return a pointer to the cb belonging to pid and remove it from the table. */
nal_cb_t *
getclr_cb(long spid)
{

int i;
int entries;
nal_cb_t *cb;


    entries= 0;
    for (i= 0; i < MAX_NUM_CB; i++)   {
	if (cb_table[i].nid >= 0)   {
	    #ifdef VERBOSE
	    printk("P3 getclr_cb() [%d] ppid %d, spid %ld, nid %d\n", i,
		cb_table[i].pid, cb_table[i].spid, cb_table[i].nid);
	    #endif /* VERBOSE */
	    entries++;
	}
    }
    #ifdef VERBOSE
	printk("P3 getclr_cb() Found %d entries\n", entries);
    #endif /* VERBOSE */

    for (i= 0; i < MAX_NUM_CB; i++)   {
	if (cb_table[i].spid == spid)   {
	    #ifdef VERBOSE
	    printk("P3 getclr_cb() freeing data structs [%d] for ppid %d, spid "
		"%ld\n", i, cb_table[i].pid, cb_table[i].spid);
	    #endif /* VERBOSE */
	    cb= cb_table[i].cb;
	    cb_table[i].nid= -1;
	    cb_table[i].pid= -1;
	    cb_table[i].spid= -1;
	    cb_table[i].task= NULL;
	    cb_table[i].cb= NULL;
	    return cb;
	}
    }

    return NULL;

}  /* end of getclr_cb() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/* Enter a cb pointer in a free slot in the table */
int
enter_cb(nal_cb_t *cb, int index, int nid, int pid)
{

#ifdef VERBOSE
int i;
int entries= 0;


	for (i= 0; i < MAX_NUM_CB; i++)   {
	    if (cb_table[i].nid >= 0)   {
		printk("P3 enter_cb() [%d], ppid %d, spid %ld, nid %d\n", i,
		    cb_table[i].pid, cb_table[i].spid, cb_table[i].nid);
		entries++;
	    }
	}
	printk("P3 enter_cb() Found %d entries\n", entries);
#endif /* VERBOSE */


    if (cb_table[index].nid >= 0) {
        struct task_struct *p;
        int slot_free = 1;

        printk("enter_cb(): slot %d potentially stale\n", index);

        /* try to determine if slot is really busy */
        for_each_task(p) {
            if (cb_table[index].spid == p->pid) {
                slot_free = 0;
                break;
            }
        }

        if (slot_free) {
            nal_cb_t *nal = cb_table[index].cb;

            printk("enter_cb(): slot %d was stale (was spid %d, ppid %d)\n",
                index, (int) cb_table[index].spid, (int) cb_table[index].pid);
      
            if (nal != NULL) {
                printk("enter_cb():  freeing non-null nal\n");
                lib_fini(nal);
                if (nal->nal_data != NULL) kfree(nal->nal_data);
                kfree(nal);
            }
            cb_table[index].nid = -1;
        }
    }

    if (cb_table[index].nid < 0)   {
	/* It is free */
	cb_table[index].task= getTsk(pid);
	if (cb_table[index].task == NULL)   {
	    printk("enter_cb() getTsk() failed\n");
	    return FALSE;
	}
	cb_table[index].nid= nid;
	cb_table[index].pid= pid;
	cb_table[index].spid= current->pid;
	cb_table[index].cb= cb;
	#ifdef VERBOSE
	    printk("P3 enter_cb() index %d, ppid %d, spid %ld\n", index,
		cb_table[index].pid, cb_table[index].spid);
	#endif /* VERBOSE */
	return TRUE;
    } else   {
	printk("enter_cb() index %d not free, cb_table[%d].nid=%d, ppid=%d, spid=%d\n", 
                                        index, index, cb_table[index].nid,
					(int) cb_table[index].pid, (int) cb_table[index].spid);

	return FALSE;
    }

}  /* end of enter_cb() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

