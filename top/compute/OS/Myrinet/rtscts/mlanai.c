/* mlanai.c */
/* $Id: mlanai.c,v 1.12 2001/08/22 16:29:07 pumatst Exp $ */
/*************************************************************************
 *                                                                       *
 * Myricom Myrinet Software                                              *
 *                                                                       *
 * Copyright (c) 1994-1997 by Myricom, Inc.                              *
 * All rights reserved.                                                  *
 *                                                                       *
 * Permission to use, copy, modify and distribute this software and its  *
 * documentation in source and binary forms for non-commercial purposes  *
 * and without fee is hereby granted, provided that the modified software*
 * is returned to Myricom, Inc. for redistribution. The above copyright  *
 * notice must appear in all copies.  Both the copyright notice and      *
 * this permission notice must appear in supporting documentation, and   *
 * any documentation, advertising materials and other materials related  *
 * to such distribution and use must acknowledge that the software was   *
 * developed by Myricom, Inc. The name of Myricom, Inc. may not be used  *
 * to endorse or promote products derived from this software without     *
 * specific prior written permission.                                    *
 *                                                                       *
 * Myricom, Inc. makes no representations about the suitability of this  *
 * software for any purpose.                                             *
 *                                                                       *
 * THIS FILE IS PROVIDED "AS-IS" WITHOUT WARRANTY OF ANY KIND, WHETHER   *
 * EXPRESSED OR IMPLIED, INCLUDING THE WARRANTY OF MERCHANTIBILITY OR    *
 * FITNESS FOR A PARTICULAR PURPOSE. MYRICOM, INC. SHALL HAVE NO         *
 * LIABILITY WITH RESPECT TO THE INFRINGEMENT OF COPYRIGHTS, TRADE       *
 * SECRETS OR ANY PATENTS BY THIS FILE OR ANY PART THEREOF.              *
 *                                                                       *
 * In no event will Myricom, Inc. be liable for any lost revenue         *
 * or profits or other special, indirect and consequential damages, even *
 * if Myricom has been advised of the possibility of such damages.       *
 *                                                                       *
 * Other copyrights might apply to parts of this software and are so     *
 * noted when applicable.                                                *
 *                                                                       *
 * Myricom, Inc.                                                         *
 * 325 N. Santa Anita Ave.                                              *
 * Arcadia, CA 91006                                                     *
 * 818 821-5555                                                          *
 * http://www.myri.com                                                   *
 *************************************************************************/

#include <linux/param.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/wrapper.h>
#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <linux/time.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/major.h>

#include "myri.h"
#include "printf.h"
#include "lanai_device.h"
#include "MyrinetPCI.h"

/* #define PRINTF(x) printk */

#ifdef LINUX24
#define vma_get_offset(vma)    vma->vm_pgoff
#define vma_get_start(vma)     vma->vm_start
#define vma_get_end(vma)       vma->vm_end
#define vma_get_page_prot(vma) vma->vm_page_prot
#endif

extern char kernel_version[];

#ifndef _PAGE_PCD
#define _PAGE_PCD    0
#endif


/**********************************************************************
    mmap driver stuff

    Gives user access to the board

**********************************************************************/
int mlanai_count = 0;

struct mlanai_info mlanai_private[MLANAI_MAX] =
{
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
};

/*
   struct MYRINET_BOARD_MAPPED {
   volatile unsigned short lanai_control[PAGE_OF_SHORTS];
   volatile unsigned int lanai_registers[PAGE_OF_INTS];
   volatile unsigned int lanai_memory   [(1024*1024)/sizeof(int)];
   };
 */


#ifdef LINUX24
int
mlanai_mmap(struct file *filep, struct vm_area_struct *vma)
#else
int
mlanai_mmap(file_handle *filep, vm_area_handle *vma)
#endif
{
    int minor = MINOR(filep->f_dentry->d_inode->i_rdev);
    int major = MAJOR(filep->f_dentry->d_inode->i_rdev);
    struct MYRINET_BOARD *mb;
    struct MYRINET_BOARD *mb_remap;
    int unit = minor;
    unsigned long offset;

    PRINTF(4) ("mlanai_mmap(%p, %p)\n", filep, vma);
    PRINTF(4) ("mlanai_mmap:  major = %d  minor = %d\n", major, minor);
    PRINTF(4) ("mlanai_mmap:  offset = %lx  start = %lx  end = %lx (%lx)\n",
       (unsigned long) vma_get_offset(vma), (unsigned long) vma_get_start(vma),
       (unsigned long) vma_get_end(vma), vma_get_end(vma) - vma_get_start(vma));

    if (!mlanai_private[minor].myriP) {
        PRINTF(0) ("mlanai_mmap: minor = %d, no info found??\n", minor);
        return -ENODEV;
    }

    mb = mlanai_private[minor].myriP->mb;
    mb_remap = mlanai_private[minor].myriP->mb_remap;



    /* for L5 boards, just map the whole board to user space */
    if (((vma_get_end(vma) - vma_get_start(vma)) >= (16 * 1024 * 1024)) &&
		    ((mlanai_private[minor].myriP->revision == 2) ||
		     (mlanai_private[minor].myriP->revision == 3))) {

	    PRINTF(4) ("mlanai_mmap: mapping NEW L5 board - revision==2,3\n");
#ifdef intel_linux
	    offset = (unsigned long) mb;
#else
	    offset = (unsigned long)LANAI[unit];
#endif

	    if (remap_page_range(vma_get_start(vma),
						     offset, 16 * 1024 * 1024,
						     vma_get_page_prot(vma))) {
		    return -EAGAIN;
	    }
	    PRINTF(4) ("mapped 16meg board at %lx\n", offset);


	    if (vma_get_end(vma) - vma_get_start(vma) >= (16 * 1024 * 1024)) {

#ifndef intel_linux
		    pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
#endif /* intel_linux */

	    }

	    return (0);
    }



    /* 1st page is EEPROM */

#ifdef intel_linux
    offset = ((unsigned long) LANAI_EEPROM[minor] - (unsigned long) mb_remap) +
        (unsigned long) mb;
#else
    offset = (unsigned long) LANAI_EEPROM[minor];
    pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
#endif

    if (remap_page_range(vma_get_start(vma),
	     offset, PAGE_SIZE, vma_get_page_prot(vma))) {
        return -EAGAIN;
    }
    PRINTF(4) ("mapped LANAI_EEPROM at %lx\n", offset);


    /* 2nd page is FPGA control */
#ifdef intel_linux
    offset = ((unsigned long) LANAI_CONTROL[minor] - (unsigned long) mb_remap) +
        (unsigned long) mb;
#else
    offset = (unsigned long) LANAI_CONTROL[minor];
    pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
#endif /* intel_linux */

    if (remap_page_range(vma_get_start(vma) + PAGE_SIZE,
	     offset, PAGE_SIZE, vma_get_page_prot(vma))) {
        return -EAGAIN;
    }
    PRINTF(4) ("mapped LANAI_CONTROL at %lx\n", offset);



    /* 3rd page is LANAI_SPECIAL */
#ifdef intel_linux
    offset = ((unsigned long) LANAI_SPECIAL[minor] - (unsigned long) mb_remap) +
        (unsigned long) mb;
#else
    offset = (unsigned long) LANAI_SPECIAL[minor];
    pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
#endif

    if (remap_page_range(vma_get_start(vma) + (2 * PAGE_SIZE),
	     offset, PAGE_SIZE, vma_get_page_prot(vma))) {
        return -EAGAIN;
    }
    PRINTF(4) ("mapped LANAI_SPECIAL at %lx\n", offset);



    /* next set of pages is LANAI_MEMORY */
#ifdef intel_linux
    offset = ((unsigned long) LANAI3[minor] - (unsigned long) mb_remap) +
        (unsigned long) mb;
#else
    offset = (unsigned long) LANAI3[minor];
#endif

    pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
    if (remap_page_range(vma_get_start(vma) + (3 * PAGE_SIZE),
	    offset, lanai_memory_size(unit), vma_get_page_prot(vma))) {
        return -EAGAIN;
    }
    PRINTF(4) ("mapped LANAI_MEMORY at %lx  len = 0x%x bytes\n",
               offset, lanai_memory_size(unit));

    if (vma_get_end(vma) - vma_get_start(vma) >
            ((3 * PAGE_SIZE) + lanai_memory_size(unit))) {
        pgprot_val(vma->vm_page_prot) |= (_PAGE_PCD);
    }

    return 0;
}



int
mlanai_ioctl(struct inode *inodeP, struct file *fileP, unsigned int cmd,
	unsigned long arg)
{

int i;
int minor = MINOR(inodeP->i_rdev);
struct board_info bi_copy;

   PRINTF(5)("mlanai_ioctl:  major %d, minor %d, cmd 0x%x\n",
       MAJOR(inodeP->i_rdev), minor, cmd);

#ifdef intel_linux
    flush_cache_all();
    flush_tlb_all();
    asm volatile ("wbinvd");
    mb();
#endif

    i = verify_area(VERIFY_WRITE, (void *) arg, sizeof(struct board_info));

    if (!i) {
        if (mlanai_private[minor].myriP) {
            struct MYRINET_BOARD *mb = mlanai_private[minor].myriP->mb;

	    /*
	    ** used to save pointers to the board and pass it to user
	    ** space etc.
	    */
            bi_copy.lanai_control =
		    (unsigned long) mlanai_private[minor].myriP->revision;
	    bi_copy.lanai_registers = (unsigned long) mb->lanai_registers;
	    bi_copy.lanai_eeprom = (unsigned long) mb->lanai_eeprom;


	    if ((!(((unsigned long) LANAI_CONTROL[minor]) & 0x40)) &&
		    (ntohs(LANAI_EEPROM[minor]->board_type) ==
			MYRINET_BOARDTYPE_1MEG_SRAM)) {
		bi_copy.lanai_memory = (unsigned long) mb->lanai_memory2;
		PRINTF(8) ("mlanai_ioctl:[%d] returning pointer to 1meg SRAM\n", minor);
	    } else {
		bi_copy.lanai_memory = (unsigned long) mb->lanai_memory;
		PRINTF(8) ("mlanai_ioctl:[%d] returning pointer to lanai_memory\n", minor);
	    }

	    bi_copy.lanai_memory_size = lanai_memory_size(minor);

	    copy_to_user((void *) arg, &bi_copy, sizeof(struct board_info));
        } else {
            return (-ENODEV);
        }
    }

    return i;

}

int
mlanai_open(struct inode *inodeP, struct file *fileP)
{
    int minor = MINOR(inodeP->i_rdev);
    /*
       int major = MAJOR(inodeP->i_rdev);

       PRINTF(5)("mlanai_open(%p, %p)\n", inodeP,  fileP);
       PRINTF(5)("mlanai_open:  major = %d  minor = %d\n", major, minor);
    */

    if (mlanai_private[minor].myriP) {
	return (0);
    } else {
	return (-ENODEV);
    }
}



struct file_operations mlanai_fops =
{
    ioctl: mlanai_ioctl,
    mmap : mlanai_mmap,
    open : mlanai_open,
};


int
mlanai_init(struct myri_private *myriP)
{
    PRINTF(0)("Myrinet mmap will use _PAGE_PCD = 0x%x  %s\n",
               _PAGE_PCD, (_PAGE_PCD) ? "UNcacheable" : "cacheable");

    if ((mlanai_count + 1) >= MLANAI_MAX) {
        PRINTF(0)("mlanai_init: no more devices available  mlanai_count = %d\n",
	    mlanai_count);
        return -EIO;
    }
    if (register_chrdev(MLANAI_MAJOR, "mlanai", &mlanai_fops)) {
        PRINTF(0) ("unable to get major_device_num =  %d for Myrinet board\n",
	    MLANAI_MAJOR);
        return -EIO;
    }

    mlanai_private[mlanai_count].unit = myriP->unit;
    mlanai_private[mlanai_count].myriP = myriP;
    PRINTF(0) ("mlanai_init succeeded at registering mlanai device at %d,%d\n",
	MLANAI_MAJOR, mlanai_count);
    PRINTF(0) ("mlanai_init myriP->unit = %d    mlanai_count = %d\n",
	myriP->unit, mlanai_count);

    mlanai_count++;
    return 0;
}

int
mlanai_uninit(struct myri_private *myriP)
{

    PRINTF(0) ("mlanai_UNinit called\n");

    if (0 /* test for inuse */ ) {
        PRINTF(0) ("mlanai: busy - remove delayed\n");
        return (-EBUSY);
    }
    else {
        PRINTF(0) ("mlanai_UNinit UNregistering device\n");
        unregister_chrdev(MLANAI_MAJOR, "mlanai");
        mlanai_count--;
        return (0);
    }
}
