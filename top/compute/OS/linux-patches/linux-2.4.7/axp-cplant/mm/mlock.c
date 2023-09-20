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
 *	linux/mm/mlock.c
 *
 *  (C) Copyright 1995 Linus Torvalds
 */
#include <linux/slab.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/smp_lock.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>

static inline int mlock_fixup_all(struct vm_area_struct * vma, int newflags)
{
	spin_lock(&vma->vm_mm->page_table_lock);
	vma->vm_flags = newflags;
	spin_unlock(&vma->vm_mm->page_table_lock);
	return 0;
}

static inline int mlock_fixup_start(struct vm_area_struct * vma,
	unsigned long end, int newflags)
{
	struct vm_area_struct * n;

	n = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
	if (!n)
		return -EAGAIN;
	*n = *vma;
	n->vm_end = end;
	n->vm_flags = newflags;
	n->vm_raend = 0;
	if (n->vm_file)
		get_file(n->vm_file);
	if (n->vm_ops && n->vm_ops->open)
		n->vm_ops->open(n);
	lock_vma_mappings(vma);
	spin_lock(&vma->vm_mm->page_table_lock);
	vma->vm_pgoff += (end - vma->vm_start) >> PAGE_SHIFT;
	vma->vm_start = end;
	__insert_vm_struct(current->mm, n);
	spin_unlock(&vma->vm_mm->page_table_lock);
	unlock_vma_mappings(vma);
	return 0;
}

static inline int mlock_fixup_end(struct vm_area_struct * vma,
	unsigned long start, int newflags)
{
	struct vm_area_struct * n;

	n = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
	if (!n)
		return -EAGAIN;
	*n = *vma;
	n->vm_start = start;
	n->vm_pgoff += (n->vm_start - vma->vm_start) >> PAGE_SHIFT;
	n->vm_flags = newflags;
	n->vm_raend = 0;
	if (n->vm_file)
		get_file(n->vm_file);
	if (n->vm_ops && n->vm_ops->open)
		n->vm_ops->open(n);
	lock_vma_mappings(vma);
	spin_lock(&vma->vm_mm->page_table_lock);
	vma->vm_end = start;
	__insert_vm_struct(current->mm, n);
	spin_unlock(&vma->vm_mm->page_table_lock);
	unlock_vma_mappings(vma);
	return 0;
}

static inline int mlock_fixup_middle(struct vm_area_struct * vma,
	unsigned long start, unsigned long end, int newflags)
{
	struct vm_area_struct * left, * right;

//CPLANT
//        unsigned long jflags;
//CPLANT

	left = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
	if (!left)
		return -EAGAIN;
	right = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
	if (!right) {
		kmem_cache_free(vm_area_cachep, left);
		return -EAGAIN;
	}
//CPLANT
//        save_flags(jflags);
//        cli();
//CPLANT
	*left = *vma;
	*right = *vma;
	left->vm_end = start;
	right->vm_start = end;
	right->vm_pgoff += (right->vm_start - left->vm_start) >> PAGE_SHIFT;
	vma->vm_flags = newflags;
	left->vm_raend = 0;
	right->vm_raend = 0;
	if (vma->vm_file)
		atomic_add(2, &vma->vm_file->f_count);

	if (vma->vm_ops && vma->vm_ops->open) {
		vma->vm_ops->open(left);
		vma->vm_ops->open(right);
	}
	lock_vma_mappings(vma);
	spin_lock(&vma->vm_mm->page_table_lock);
	vma->vm_pgoff += (start - vma->vm_start) >> PAGE_SHIFT;
	vma->vm_start = start;
	vma->vm_end = end;
	vma->vm_flags = newflags;
	vma->vm_raend = 0;
	__insert_vm_struct(current->mm, left);
	__insert_vm_struct(current->mm, right);
	spin_unlock(&vma->vm_mm->page_table_lock);
	unlock_vma_mappings(vma);
//CPLANT
//        restore_flags(flags);
//CPLANT
	return 0;
}

static int mlock_fixup(struct vm_area_struct * vma, 
	unsigned long start, unsigned long end, unsigned int newflags)
{
	int pages, retval;

	if (newflags == vma->vm_flags)
		return 0;

	if (start == vma->vm_start) {
		if (end == vma->vm_end)
			retval = mlock_fixup_all(vma, newflags);
		else
			retval = mlock_fixup_start(vma, end, newflags);
	} else {
		if (end == vma->vm_end)
			retval = mlock_fixup_end(vma, start, newflags);
		else
			retval = mlock_fixup_middle(vma, start, end, newflags);
	}
	if (!retval) {
		/* keep track of amount of locked VM */
		pages = (end - start) >> PAGE_SHIFT;
		if (newflags & VM_LOCKED) {
			pages = -pages;
			make_pages_present(start, end);
		}
		vma->vm_mm->locked_vm -= pages;
	}
	return retval;
}

static int do_mlock(unsigned long start, size_t len, int on)
{
	unsigned long nstart, end, tmp;
	struct vm_area_struct * vma, * next;
	int error;
//CPLANT
#if 0
	if (on && !capable(CAP_IPC_LOCK))
		return -EPERM;
#endif
//CPLANT
	len = PAGE_ALIGN(len);
	end = start + len;
	if (end < start)
		return -EINVAL;
	if (end == start)
		return 0;
	vma = find_vma(current->mm, start);
	if (!vma || vma->vm_start > start)
		return -ENOMEM;

	for (nstart = start ; ; ) {
		unsigned int newflags;

		/* Here we know that  vma->vm_start <= nstart < vma->vm_end. */

		newflags = vma->vm_flags | VM_LOCKED;
		if (!on)
			newflags &= ~VM_LOCKED;

		if (vma->vm_end >= end) {
			error = mlock_fixup(vma, nstart, end, newflags);
			break;
		}

		tmp = vma->vm_end;
		next = vma->vm_next;
		error = mlock_fixup(vma, nstart, tmp, newflags);
		if (error)
			break;
		nstart = tmp;
		vma = next;
		if (!vma || vma->vm_start != nstart) {
			error = -ENOMEM;
			break;
		}
	}
	return error;
}

asmlinkage long sys_mlock(unsigned long start, size_t len)
{
	unsigned long locked;
	unsigned long lock_limit;
	int error = -ENOMEM;

	down_write(&current->mm->mmap_sem);
	len = PAGE_ALIGN(len + (start & ~PAGE_MASK));
	start &= PAGE_MASK;

	locked = len >> PAGE_SHIFT;
	locked += current->mm->locked_vm;

	lock_limit = current->rlim[RLIMIT_MEMLOCK].rlim_cur;
	lock_limit >>= PAGE_SHIFT;

	/* check against resource limits */
	if (locked > lock_limit)
		goto out;

	/* we may lock at most half of physical memory... */
	/* (this check is pretty bogus, but doesn't hurt) */
	if (locked > num_physpages/2)
		goto out;

	error = do_mlock(start, len, 1);
out:
	up_write(&current->mm->mmap_sem);
	return error;
}

asmlinkage long sys_munlock(unsigned long start, size_t len)
{
	int ret;

	down_write(&current->mm->mmap_sem);
	len = PAGE_ALIGN(len + (start & ~PAGE_MASK));
	start &= PAGE_MASK;
	ret = do_mlock(start, len, 0);
	up_write(&current->mm->mmap_sem);
	return ret;
}

static int do_mlockall(int flags)
{
	int error;
	unsigned int def_flags;
	struct vm_area_struct * vma;

//CPLANT
#if 0
	if (!capable(CAP_IPC_LOCK))
		return -EPERM;
#endif
//CPLANT

	def_flags = 0;
	if (flags & MCL_FUTURE)
		def_flags = VM_LOCKED;
	current->mm->def_flags = def_flags;

	error = 0;
	for (vma = current->mm->mmap; vma ; vma = vma->vm_next) {
		unsigned int newflags;

		newflags = vma->vm_flags | VM_LOCKED;
		if (!(flags & MCL_CURRENT))
			newflags &= ~VM_LOCKED;
		error = mlock_fixup(vma, vma->vm_start, vma->vm_end, newflags);
		if (error)
			break;
	}
	return error;
}

asmlinkage long sys_mlockall(int flags)
{
	unsigned long lock_limit;
	int ret = -EINVAL;

	down_write(&current->mm->mmap_sem);
	if (!flags || (flags & ~(MCL_CURRENT | MCL_FUTURE)))
		goto out;

	lock_limit = current->rlim[RLIMIT_MEMLOCK].rlim_cur;
	lock_limit >>= PAGE_SHIFT;

	ret = -ENOMEM;
	if (current->mm->total_vm > lock_limit) {
//CPLANT
            printk("sys_mlockall: trying to lock > kernel's lock_limit=%d\n", lock_limit);
//CPLANT
		goto out;
        }

	/* we may lock at most half of physical memory... */
	/* (this check is pretty bogus, but doesn't hurt) */
	if (current->mm->total_vm > num_physpages/2) {
//CPLANT
                printk("sys_mlockall: trying to lock %d pages: numphys/2=%d\n", current->mm->total_vm, num_physpages/2);
//CPLANT
		goto out;
        }

	ret = do_mlockall(flags);
out:
	up_write(&current->mm->mmap_sem);
	return ret;
}

asmlinkage long sys_munlockall(void)
{
	int ret;

	down_write(&current->mm->mmap_sem);
	ret = do_mlockall(0);
	up_write(&current->mm->mmap_sem);
	return ret;
}
