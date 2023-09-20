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
** $Id: calcPhysAddress.h,v 1.9 2001/11/08 02:19:12 pumatst Exp $
** Calculate a physical (hardware) address given a virtual (user) address
** and a task structure.
*/

#ifndef CALCPHYSADDRESS_H
#define CALCPHYSADDRESS_H

#include <linux/malloc.h>
#include <asm/pgtable.h>

#ifdef __i386__
#define page_frame(x) ((unsigned long) __va(pte_val(x) & PAGE_MASK))
#endif

#ifdef __ia64__
#define page_frame(x) ((unsigned long) __va(pte_val(x) & _PFN_MASK))
#endif

#ifdef __alpha__
#define ALPHA_PFN_MASK 0xFFFFFFFF00000000
static inline unsigned long page_frame(pte_t pte)
{ return PAGE_OFFSET + ((pte_val(pte) & ALPHA_PFN_MASK) >> (32-PAGE_SHIFT)); }
#endif

static inline 
unsigned long calcPhysAddress( struct task_struct *tsk, unsigned long virt_addr )
{
    int where = 0;
    pgd_t *page_dir;
    pmd_t *page_middle;
    pte_t *pte;
    unsigned long base, offset, phys_addr;

    page_dir = pgd_offset( tsk->mm, virt_addr );
  //page_dir = pgd_offset( tsk->active_mm, virt_addr );

#if 0
    if (pgd_none(*page_dir))
	{ where = 1; goto failed; }
    if (pgd_bad(*page_dir))
	{ where = 2; goto failed; }
#endif

    page_middle = pmd_offset(page_dir, virt_addr);
#if 0
    if (pmd_none(*page_middle))
	{ where = 3; goto failed; }
    if (pmd_bad(*page_middle))
	{ where = 4; goto failed; }
#endif

    pte = pte_offset(page_middle, virt_addr);
#if 1
    if (!pte_present(*pte))
	{ where = 5; goto failed; }
#endif

    base = page_frame( *pte );
    offset = virt_addr & ~PAGE_MASK;
    phys_addr = base + offset;
    return phys_addr;

failed:
    printk("calcPhysAddress(), failed at %i for address 0x%lx\n", where, virt_addr);	
//  printMemInfo( tsk->mm );
    return 0;
}

#endif
