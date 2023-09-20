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
** $Id: fileio.c,v 1.15 2001/11/03 23:40:04 pumatst Exp $
** Portals 3.0 module file that deals with access through /dev/protals3
*/

#include <asm/uaccess.h>
#include <asm/segment.h>
#include <linux/major.h>
#include <linux/unistd.h>

#include <p30/errno.h>
#include <p30/lib-dispatch.h>	/* For dispatch_table[] */
#include <p30/lib-nal.h>	/* For nal_cb_t */
#include "runtime.h"		/* For getpid() */
#include "myrnal.h"		/* For myrnal_forward_t */
#include "lib_myrnal.h"		/* For open_lib_myrnal(), close_lib_myrnal() */
#include "module.h"		/* For inc_use_count(), dec_use_count() */
#include "cb_table.h"		/* For index2cb() */
#include "devices.h"		/* For p3exit(), p3find_dev() */
#include "cb_table.h"		/* For get_cb_task() */
#include "stat.h"		/* For nalstat */
#include "fileio.h"


/* stuff used to do mlock syscalls thru ioctl */
typedef int (*FP)(unsigned long, ... );
extern FP *sys_call_table;
FP        *syscalls = (FP *)&sys_call_table;


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

struct file_operations p3_fops =
{
    ioctl  :  p3ioctl,
    open   :  p3open,
    release: p3release,
};

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3open(struct inode *inodeP, struct file *fileP)
{

int rc;


    rc= open_lib_myrnal();
    if (rc != -1)   {
	/* We're open */
	inc_use_count();
	nalstat.NIInitOK++;
    } else   {
	nalstat.NIInitBAD++;
    }
    return rc;

}  /* end of p3open() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3release(struct inode *inodeP, struct file *fileP)
{

    p3exit(p3find_dev(), get_cb_task(current->pid));
    close_lib_myrnal();
    dec_use_count();
    nalstat.NIFini++;
    return 0;

}  /* end of p3release() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3ioctl(struct inode *inodeP, struct file *fileP, unsigned int cmd,
    unsigned long arg)
{

int rc;
myrnal_forward_t myrnal_forward;
char arg_block[MAX_ARGS_LEN];
char ret_block[MAX_RET_LEN];
unsigned long sys_arg;
nal_cb_t *cb;
int index;
int p3cmd;


    #ifdef VERBOSE
	printk("p3ioctl: major %d, minor %d, pid %d, cmd %d)\n",
	    MAJOR(inodeP->i_rdev), MINOR(inodeP->i_rdev), current->pid, cmd);
    #endif /* VERBOSE */

    /* This includes an access check via access_ok() */
    rc= copy_from_user(&myrnal_forward, (void *)arg, sizeof(myrnal_forward_t));
    if (rc != 0) {
	printk("p3ioctl() read access to myrnal_forward denied\n");
	nalstat.FwdBAD1++;
	return PTL_SEGV;
    }

    if (cmd == P3CMD)   {
	/*
	** This is a P3 command to be handed of to dispatch
	*/
	if (myrnal_forward.args_len > MAX_ARGS_LEN)   {
	    printk("p3ioctl() args_len %ld > %d\n", myrnal_forward.args_len,
		MAX_ARGS_LEN);
	    nalstat.FwdBAD2++;
	    return PTL_SEGV;
	}
	nalstat.FwdArgLen += myrnal_forward.args_len;
	if (myrnal_forward.args_len > nalstat.FwdArgLenMax)   {
	    nalstat.FwdArgLenMax= myrnal_forward.args_len;
	}
	if (myrnal_forward.args_len < nalstat.FwdArgLenMin)   {
	    nalstat.FwdArgLenMin= myrnal_forward.args_len;
	}

	if (myrnal_forward.ret_len > MAX_RET_LEN)   {
	    printk("p3ioctl() ret %ld > %d\n", myrnal_forward.ret_len,
		MAX_RET_LEN);
	    nalstat.FwdBAD3++;
	    return PTL_SEGV;
	}
	nalstat.FwdRetLen += myrnal_forward.ret_len;
	if (myrnal_forward.ret_len > nalstat.FwdRetLenMax)   {
	    nalstat.FwdRetLenMax= myrnal_forward.ret_len;
	}
	if (myrnal_forward.ret_len < nalstat.FwdRetLenMin)   {
	    nalstat.FwdRetLenMin= myrnal_forward.ret_len;
	}


	/* Get the arguments from user space (including access check) */
	rc= copy_from_user(arg_block, myrnal_forward.args,
		myrnal_forward.args_len);
	if (rc != 0)   {
	    printk("p3ioctl() reading from args failed\n");
	    nalstat.FwdBAD4++;
	    return PTL_SEGV;
	}

	/* Before we do anything make sure we'll be able to copy the ret vals */
	rc= access_ok(VERIFY_WRITE, myrnal_forward.ret, myrnal_forward.ret_len);
	if (rc == 0)   {
	    printk("p3ioctl() write access to ret block denied\n");
	    nalstat.FwdBAD5++;
	    return PTL_SEGV;
	}

	p3cmd= myrnal_forward.p3cmd;
	if (p3cmd < 0 || p3cmd >= LIB_MAX_DISPATCH)  {
	    printk("p3ioctl() p3cmd %d out of range\n", p3cmd);
	    nalstat.FwdBAD6++;
	    return PTL_SEGV;
	}

	index= spid2index(current->pid);
	if (index < 0)   {
	    printk("p3ioctl() No index found for spid %d\n", current->pid);
	    nalstat.FwdBAD7++;
	    return PTL_SEGV;
	}

	#ifdef VERBOSE
	    printk("p3ioctl() p3cmd %d, index %d\n", p3cmd, index);
	#endif /* VERBOSE */
	cb= index2cb(index);
	lib_dispatch(cb, NULL, p3cmd, arg_block, ret_block);
	nalstat.dispatch[p3cmd]++;

	rc= __copy_to_user(myrnal_forward.ret, ret_block,
		myrnal_forward.ret_len);
	if (rc != 0)   {
	    printk("p3ioctl() writing to return block failed\n");
	    nalstat.FwdBAD8++;
	    return PTL_SEGV;
	}

	nalstat.FwdOK++;
	rc= PTL_OK;
    }


    if (cmd == P3SYSCALL)   {
	/*
	** This is really a system call
	*/

	/* Get the arguments from user space (including access check) */
	rc= copy_from_user(&sys_arg, myrnal_forward.args, sizeof(sys_arg));
	if (rc != 0)   {
	    printk("p3ioctl() reading from args failed\n");
	    return -1;
	    nalstat.ioctlBad++;
	}

	switch (myrnal_forward.p3cmd)   {
	    case PTL_MLOCKALL:
#ifdef __ia64__
                rc= __ia64_syscall((int)sys_arg, 0,0,0,0, __NR_mlockall);
#else
		rc= syscalls[__NR_mlockall](sys_arg);
#endif
		nalstat.ioctlMlockall++;
		break;
	    default:
		printk("p3ioctl() Unknown command %d\n", myrnal_forward.p3cmd);
		rc= -1;
		nalstat.ioctlUnknown++;
	}
	nalstat.ioctlOK++;
    }

    return rc;

}  /* end of p3ioctl() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
