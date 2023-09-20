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
 * kmemcpy.c
 *
 * Kernel module to try to exercise memcpy bug in copy_to_user().
 * On the alpha we occasionally see a task get bad data from the
 * kernel.  The weird part is that the kernel is able to copy the
 * data from user space correctly, but when the user eventually gets
 * around to reading it, the first byte of four has -not- been written
 * correctly.  Very odd.
 *
 * So, this module works in concert with the memcpy.c to write data
 * directly into user space in an asyncronous fashion.  When the user
 * opens the device and attempts to read from it, the read will return
 * immediately and the address will be recorded.  Then, on every time
 * tick the kernel will write a known value into user space at that
 * address.  The user process spins on the buffer looking for an updated
 * value.  If the value is not correct a diagnostic is printed and
 * the process exits.  Otherwise it loops forever.
 *
 * The bug does not appear with this code on my i386 Linux box running 2.2.13.
 * It may be Alpha specific.
 *
 * Well, I've let it run for several thousand iterations and it has not
 * yet failed on me.  I'm not sure what the difference is in the
 * RTSCTS module other than that the user job is consuming some CPU
 * time.  The code needs some work and instrumentation; more on that
 * tomorrow.
 *
 * Tramm Hudson
 * 16 April 2000
 *
 */
#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/version.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/signal.h>

#include <linux/module.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/errno.h>	/* error codes */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/segment.h>
#include <asm/atomic.h>

static int unloading = 0;

typedef struct {
	int			*buf;
	int			value;
	int			seq;
	int			len;
	int			closed;
	struct task_struct	*task;
	struct tq_struct	timer;
} memcpy_t;


static void write_data( void *v_data )
{
	memcpy_t	*data = v_data;

	if( data->closed ) {
		kfree( data );
		return;
	}

	if( unloading )
		return;

	if( data->buf ) {
		if( data->task == current ) {
			data->value++;
			data->seq++;

			copy_to_user(
				&data->buf[data->seq % data->len],
				&data->value,
				sizeof( data->value )
			);
			copy_to_user(
				&data->buf[0],
				&data->seq,
				sizeof( data->seq )
			);
		} else {
			printk( "memcpy: pid %d (%p) is not current %d (%p)\n",
				data->task->pid, data->task,
				current->pid, current
			);
		}
	} else {
		/* The user hasn't tried to read anything yet */
	}

	queue_task( &data->timer, &tq_timer );
}



static int memcpy_open( struct inode *inode, struct file *file )
{
	memcpy_t	*data;
	

	MOD_INC_USE_COUNT;
	
	data = kmalloc( sizeof( memcpy_t ), GFP_KERNEL );
	printk( "memcpy: Attempting open %p %p data=%p\n", inode, file, data );

	if( !data ) {
		printk( "memcpy: Failed to allocate data\n" );
		return -1;
	}

	data->closed	= 0;
	data->buf	= NULL;
	data->task	= current;
	data->value	= 0xDEADBEEF;
	data->len	= 0;
	data->timer.routine	= write_data;
	data->timer.data	= data;
	file->private_data	= data;

	queue_task( &data->timer, &tq_timer );

	return 0;
}

static int memcpy_close( struct inode *inode, struct file *file )
{
	memcpy_t	*data;

	MOD_DEC_USE_COUNT;
	printk( "memcpy: Attempting close %p %p\n", inode, file );

	data		= file->private_data;
	data->closed	= 1;

	return -1;
}

static ssize_t memcpy_read( struct file *file, char *buf, size_t len, loff_t *off )
{
	int seq = 0;
	memcpy_t *data;
	printk( "memcpy: Attempting to read from fp %p into %p of size %ld\n",
		file, buf, (long) len );
	data = (memcpy_t*) file->private_data;

	data->value	= 0x89ABCDEF;
	data->buf	= (int*) buf;
	data->len	= len / sizeof(int);
	data->seq	= 0;

	copy_to_user( buf, &seq, sizeof( seq ) );

	return 0;
}



static struct file_operations memcpy_fops = {
	NULL,
	memcpy_read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	memcpy_open,
	NULL,
	memcpy_close,
};
	
int init_module(void)
{
	int mem_major = register_chrdev( 0, "memcpy", &memcpy_fops );
	if( mem_major < 0 ) {
		printk( "memcpy: Unable to register dynamic device\n" );
		return -1;
	}

	printk( "memcpy: Registered device %d\n", mem_major );

	return 0;
}

void cleanup_module(void)
{
	unloading = 1;
	printk( "memcpy_test: Closing down\n" );
}
