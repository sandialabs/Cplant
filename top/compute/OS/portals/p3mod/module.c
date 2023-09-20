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
** $Id: module.c,v 1.12 2000/12/01 23:19:44 pumatst Exp $
** Portals 3.0 main module file
*/

#define EXPORT_SYMTAB
#include <linux/module.h>

#include <sys/defines.h>        /* For FALSE */
#include <lib-p30.h>		/* For lib_parse() */
#include "proc_init.h"		/* For (un)register_proc() */
#include "fileio.h"		/* For p3open(), p3release(), p3ioctl() */
#include "devices.h"
#include "lib_myrnal.h"		/* For myrnal_init(), myrnal_down() */
#include "cb_table.h"		/* For get_cb() */
#include "debug.h"		/* For p3_debug_*() */
#include "stat.h"		/* For p3_stat_init() */
#include "module.h"


MODULE_AUTHOR("Rolf Riesen, Sandia National Laboratories");
MODULE_DESCRIPTION("Portals 3.0 Module");

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

EXPORT_SYMBOL(lib_parse);
EXPORT_SYMBOL(lib_finalize);
EXPORT_SYMBOL(get_cb);
EXPORT_SYMBOL(p3register_dev);
EXPORT_SYMBOL(p3unregister_dev);
EXPORT_SYMBOL(print_hdr);
EXPORT_SYMBOL(p3_debug_add);

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
inc_use_count(void)
{
    MOD_INC_USE_COUNT;
}  /* end of insert_module() */


void
dec_use_count(void)
{
    MOD_DEC_USE_COUNT;
}  /* end of remove_module() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
/*
** Module insert, init, and removal.
*/

int
init_module(void)
{

    if (register_chrdev(P3_MAJOR, P3_DEV_NAME, &p3_fops))   {
	printk("P3 init_module(): unable to get major_device_num = %d\n",
	    P3_MAJOR);
	return -EIO;
    }

    register_proc();
    devices_init();
    p3_debug_init();
    p3_stat_init();
    myrnal_up();

    printk("P3 module inserted and registered\n");
    return 0;	    

}  /* end of init_module() */


void
cleanup_module(void)
{

    myrnal_down();
    unregister_chrdev(P3_MAJOR, P3_DEV_NAME);
    unregister_proc();

    printk("P3 module removed\n");

}  /* end of cleanup_module() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
