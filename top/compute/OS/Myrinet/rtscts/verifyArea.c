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
** $Id: verifyArea.c,v 1.5 2001/10/02 22:48:21 jbogden Exp $
**
** This really sucks! This is a copy of the same file in portals/base
** I hope we can get rid of this sometime soon....
*/
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/uaccess.h>

int
verifyArea(struct task_struct *task, int type, const void * addr,
	unsigned long size)
{

struct vm_area_struct * vma;
unsigned long start = (unsigned long) addr;


    #ifdef VERBOSE
    printk("in verifyArea()...\n");
    #endif /* VERBOSE */

    if (!size)   {
	return 0;
    }

    vma = find_vma(task->mm, start);
    if (!vma)   {
	#ifdef VERBOSE
	    printk("verifyArea (rtscts.mod): find_vma() failed...\n");
	#endif
	goto bad_area;
    }

    if (vma->vm_start > start)   {
	goto check_stack;
    }

good_area:
    #ifdef VERBOSE
    printk("verifyArea()...good_area:\n");
    #endif /* VERBOSE */
    if (type == VERIFY_WRITE)   {
	goto check_write;
    }

    for (;;)   {
	struct vm_area_struct * next;
	
	if (!(vma->vm_flags & VM_READ))   {
	    #ifdef VERBOSE
		printk("verifyArea (rtscts.mod): bad READ flags()...\n");
	    #endif
	    goto bad_area;
	}
	if (vma->vm_end - start >= size)   {
	    return 0;
	}
	next = vma->vm_next;
	if (!next || vma->vm_end != next->vm_start)   {
	    #ifdef VERBOSE
		printk("verifyArea (rtscts.mod): list corrupt, size= %ld\n",
		    size);
	    #endif
	    goto bad_area;
	}
	vma = next;
    }

check_write:
    #ifdef VERBOSE
    printk("verifyArea()...check_write:\n");
    #endif /* VERBOSE */
    if (!(vma->vm_flags & VM_WRITE))   {
	#ifdef VERBOSE
	    printk("verifyArea (rtscts.mod): bad WRITE flags()...\n");
	#endif
	goto bad_area;
    }

    for (;;)   {
	if (vma->vm_end - start >= size)   {
	    break;
	}
	if (!vma->vm_next || vma->vm_end != vma->vm_next->vm_start)   {
	    #ifdef VERBOSE
		printk("verifyArea (rtscts.mod): list corrupt...\n");
	    #endif
	    goto bad_area;
	}
	vma = vma->vm_next;
	if (!(vma->vm_flags & VM_WRITE))   {
	    #ifdef VERBOSE
		printk("verifyArea (rtscts.mod): bad WRITE flags()...\n");
	    #endif
	    goto bad_area;
	}
    }
    #ifdef VERBOSE
    printk("out verifyArea()...\n");
    #endif /* VERBOSE */
    return 0;

check_stack:
    #ifdef VERBOSE
    printk("verifyArea()...check_stack:\n");
    #endif /* VERBOSE */
    if (!(vma->vm_flags & VM_GROWSDOWN))  {
	#ifdef VERBOSE
	    printk("verifyArea (rtscts.mod): check_stack failed...\n");
	#endif
	goto bad_area;
    }
    if (expand_stack(vma, start) == 0)   {
	goto good_area;
    }

bad_area:
    #ifdef VERBOSE
	printk("verifyArea (rtscts.mod): failed addr %p, start %p size %li, "
	    "pid %d\n", addr, (void*)start, size, task->pid);
    #endif
    if (vma)   {
	#ifdef VERBOSE
	    printk("verifyArea (rtscts.mod): vma->vm_start %p, vma->vm_end "
		"%p\n", (void*) vma->vm_start, (void*) vma->vm_end);
	#endif
	if (vma->vm_next)   {
	    if (vma->vm_end != vma->vm_next->vm_start) {
		#ifdef VERBOSE
		    printk("verifyArea (rtscts.mod): vma->vm_end %p != "
			"vma->vm_next->vm_start %p\n", (void*) vma->vm_end,
			(void*) vma->vm_next->vm_start);
		#endif
	    }
	} else   {
	    #ifdef VERBOSE
	    printk("verifyArea (rtscts.mod): vma->vm_next NULL (could not "
		"verify all %ld bytes\n", size);
	    #endif
	}
    }
    #if 0 /* #ifdef VERBOSE - where the heck is printMemInfo? */
	printMemInfo(task->mm);
    #endif
    return -EFAULT;

}  /* end of verifyArea() */
