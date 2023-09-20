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
** $Id: ia64.h,v 1.1.4.1 2002/03/21 00:34:34 ktpedre Exp $
*/

#ifndef ALPHA_H
#define ALPHA_H

#include <asm/current.h>
#include <asm/uaccess.h>
#include <cTask/cTask.h>
#include <portals/base/calcPhysAddress.h>

#ifdef TIMER_TESTS
#include "/usr/local/jotto/timer/mtimer.h"
#endif

#define __gcc_barrier() __asm__ __volatile__("": : :"memory")

int verifyArea(struct task_struct *task, int type, 
                           const void * addr, unsigned long size);

static inline char get_user_char(char* ptr)
{
  char x;
  __get_user(x,ptr);
  return x;
}
static inline unsigned short int get_user_ushort(unsigned short int* ptr)
{
  unsigned short int x;
  __get_user(x,ptr);
  return x;
}

enum { FROM_USER, TO_USER };

static inline int memcpy2( int dir, struct task_struct *task,
                           void *to, void *from, int size );

static inline int memcpy_fromfs2( int pid, void *to, void *from, int size )
{
    struct task_struct  *task;
    unsigned long flags;
    int status;

    PRINTK("memcpy_fromfs2() pid=%i to=0x%p from=0x%p size=%i\n",
            pid, to, from, size);
    __gcc_barrier();
    save_flags( flags );
    cli();	
    if ( ( task = cTaskGetTask( pid ) ) ) {
	if ( verifyArea( task, VERIFY_READ, from, size ) ) {
          PRINTK("memcpy_fromfs2(): verifiyArea read failed %p, len %d, pid %d\n", from, size, pid);
            status= PERR_VERIFYMEM;
	} else {
	    if (memcpy2( FROM_USER, task, to, from, size ) != 0)   {
		status= PERR_VERIFYMEM;
	    } else   {
		status= PERR_NOTHING;
	    }
	}
    } else {
        PRINTK( "memcpy_fromfs2(): failed could not get task info\n");
        status= PERR_NO_TASK_INFO;
    }
    restore_flags( flags );
    __gcc_barrier();
    return status;
} 
static inline int memcpy_tofs2( int pid, void *to, void *from, int size )
{
    unsigned long flags;
    struct task_struct *task;
    int status;

    PRINTK("memcpy_tofs2() pid=%i to=0x%p from=0x%p size=%i\n",
            pid, to, from, size);
    __gcc_barrier();
    save_flags( flags );
    cli();	

    if ( ( task = cTaskGetTask( pid ) ) ) {
	if ( verifyArea( task, VERIFY_WRITE, to, size ) ) {
	    PRINTK("memcpy_tofs2(): verifiyArea write failed %p, len %d, "
		"pid %d\n", to, size, pid);
            status= PERR_VERIFYMEM;
	} else {
            if (memcpy2( TO_USER, task, to, from, size ) != 0)   {
		status= PERR_VERIFYMEM;
	    } else   {
		status= PERR_NOTHING;
            }
	}
    } else {
        PRINTK( "memcpy_tofs2(): failed could not get task info\n");
        status= PERR_NO_TASK_INFO;
    }
    restore_flags( flags );
    __gcc_barrier();
    return status;
}

static inline int memcpy2( int dir, struct task_struct *task, 
                           void *to, void *from, int nbytes ) 
{
    unsigned long addr;
    unsigned long chunk;

    if (task == NULL || task->mm == NULL) return 0;

    while ( nbytes ) {
	if ( dir == FROM_USER ) {
	    addr = calcPhysAddress( task, (unsigned long)from );
	} else {
	    addr = calcPhysAddress( task, (unsigned long)to );
        }

	if (addr == 0)   {
	    /* Address verification failed */
            PRINTK("memcpy2: calcPhysAddress failed...\n");
	    return -1;
	}
    
	/* if we are not on a page boundary */
	if ( addr & ~PAGE_MASK ) {
	    chunk = PAGE_SIZE - ( addr & ~PAGE_MASK );
	    if ( nbytes < chunk ) {
		chunk = nbytes;
	    } 
	} else {
	    if ( nbytes >= PAGE_SIZE ) {
		chunk = PAGE_SIZE;
	    } else {
		chunk = nbytes;
	   }
	}  
	if ( dir == FROM_USER ) {
	    memcpy( to, (void*) addr, chunk );    
	} else {
	    memcpy( (void*) addr, from, chunk );    
	}
	to += chunk;
	from += chunk;
	nbytes -= chunk;
    }
    return 0;
}
#endif
