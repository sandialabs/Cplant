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
** $Id: IP_init.c,v 1.10 2002/01/18 20:57:35 jbogden Exp $
** Initialize the module
*/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */

#ifdef USE_PROC_FS
    #include <linux/proc_fs.h>
    #include "IP_proc.h"
#endif /* USE_PROC_FS */

#include "../rtscts/RTSCTS_ip.h"	/* For register_IPrecv, unreg... */
#include "IP_module.h"			/* For MYRIP_MTU, etc. */
#include "IP_recv.h"			/* For IP_init_recv() */
#include "addr_convert.h"		/* For IP_ADDR_BASE etc. */

MODULE_AUTHOR("Rolf Riesen, Sandia National Laboratories");
MODULE_DESCRIPTION("Myrinet IP driver over rtscts");

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif


/*
** The functions dummy_config(), dummy_ioctl(), and dummy_change_mtu()
** are here only for test purposes.
*/
int
dummy_config(struct NETDEV *dev, struct ifmap *map)
{
    printk("dummy_config()\n");

    if (dev->flags & IFF_UP)   {
	/* can't act on a running interface */
        printk("dummy_config() I/F is running. Ignoring request\n");
        return -EBUSY;
    }

    /* Don't allow changing the I/O address */
    if (map->base_addr != dev->base_addr) {
        printk("dummy_config() Can't change I/O address\n");
        return -EOPNOTSUPP;
    }

    /* Allow changing the IRQ */
    if (map->irq != dev->irq) {
        dev->irq = map->irq;
        printk("dummy_config() is delayed to open-time\n");
    }

    /* ignore other fields */
    return 0;
}


int
dummy_ioctl(struct NETDEV *dev, struct ifreq *rq, int cmd)
{
 
    printk("dummy_ioctl() cmd %d\n", cmd);
    return 0;
}

int
dummy_change_mtu(struct NETDEV *dev, int new_mtu)
{
    printk("dummy_change_mtu() new MTU %d\n", new_mtu);
    return 0; /* success */
}

/*> <----------------------------------><----------------------------------> <*/

int
dummy_rebuild_header(struct sk_buff *skb)
{
    printk("dummy_rebuild_header() should not be called\n");
    return 0;

}  /* end of dummy_rebuild_header() */

/*> <----------------------------------><----------------------------------> <*/

/*
** Public Functions
*/
int init_module(void);
void cleanup_module(void);


/*
** Local functions
*/
#ifdef LINUX24
    static int myrIP_write (struct file *filp, const char *buf, 
                unsigned long count, void *data);
#else
    static long myrIP_write (struct file *filp, const char *buf, size_t count,
		loff_t *ppos);
    static long myrIP_read (struct file *filp, char *buf, size_t count,
		loff_t *ppos);
#endif

/*> <----------------------------------><----------------------------------> <*/
/*
** This gets called by register_netdev() from init_module(). ifconfig
** insists that there be a init function.
** Do some more initialization and connect to the RTSCTS module
*/
static int
probe(struct NETDEV *dev)
{
    #ifdef VERBOSE
	printk("myrIP: probe(dev name %s)\n", dev->name);
    #endif /* VERBOSE */

    ether_setup(dev); /* assign some of the fields */

    dev->hard_start_xmit= myrIP_xmit;
    dev->open= myrIP_open;
    dev->stop= myrIP_close;
    dev->get_stats= myrIP_get_stats;
    dev->set_config= dummy_config;
    dev->do_ioctl= dummy_ioctl;
    dev->change_mtu= dummy_change_mtu;

    dev->accept_fastpath= NULL;
    dev->rebuild_header= dummy_rebuild_header;

    dev->mtu                = MYRIP_MTU - sizeof(struct ethhdr);
#ifndef LINUX24
    dev->tbusy              = 0;
#endif
    dev->hard_header_len    = ETH_HLEN;			/* 14 */
    dev->addr_len           = ETH_ALEN;			/* 6  */
    dev->tx_queue_len       = 100;			/* ? */
    dev->type               = ARPHRD_ETHER;
    dev->flags              = dev->flags & ~(IFF_MULTICAST | IFF_BROADCAST);
    dev->flags              = dev->flags | IFF_NOARP;

    return 0;
}

/*> <----------------------------------><----------------------------------> <*/

struct NETDEV myrIPdev;

#ifdef USE_PROC_FS
    /*
    ** The portals module already installed this. All we have to
    ** do is find it ;-)
    */
    struct proc_dir_entry *proc_cplant;

#ifdef LINUX24
    struct proc_dir_entry *proc_myrIP= NULL;
    struct proc_dir_entry *proc_versions= NULL;
#else
    struct file_operations myrIP_fops = {
	read:           myrIP_read,
	write:          myrIP_write,
    };

    struct inode_operations myrIP_iops = {
	&myrIP_fops,
    };

    struct proc_dir_entry proc_myrIP = {
	0,			/* low_ino: the inode -- dynamic */
	5, "myrIP",		/* len of name and name string */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	2048, &myrIP_iops,	/* "file" size is 2k; inode ops */
    };

    /* Install /proc/cplant/versions.rtscts */
    struct proc_dir_entry proc_versions = {
        0,                      /* low_ino: the inode -- dynamic */
        14, "versions.myrIP",   /* len of name and name */
        S_IFREG | S_IRUGO,      /* mode: regular, read by anyone */
        1, 0, 0,                /* nlinks, owner (root), group (root) */
        2048, NULL,             /* "file" size is 1024 B; inode ops unused */
        versions_proc,          /* Function that lists the versions */
    };
#endif

#endif /* USE_PROC_FS */

/*> <----------------------------------><----------------------------------> <*/

/* Code duplicated in rtscts/myri.c */

struct proc_dir_entry *
find_proc_cplant(void)
{

struct proc_dir_entry *dir;


    dir= &proc_root;
    dir= dir->subdir;
    do   {
        if (strncmp("cplant", dir->name, 6) == 0)   {
            break;
        }
        dir= dir->next;
    } while (dir);

    if (dir == NULL)   {
        printk("Cannot find /proc/cplant; we'll use /proc\n");
        dir= &proc_root;
    }
    return dir;

}  /* end of find_proc_cplant() */

/*> <----------------------------------><----------------------------------> <*/

#ifndef LINUX24
    static long
    myrIP_read (struct file *filp, char *buf, size_t count, loff_t *ppos)
{

char *start;
int cplen;


    cplen= myrIPproc(buf, &start, filp->f_pos, count, 0);
    #ifdef VERBOSE
	printk("myrIP_read() got called, cnt %d, f_pos %Ld, cplen %d\n",
	    (int)count, filp->f_pos, cplen);
    #endif /* VERBOSE */
    filp->f_pos += cplen;
    return cplen;

}  /* end of myrIP_read() */
#endif


#ifdef LINUX24
    static int
    myrIP_write (struct file *filp, const char *buf, unsigned long count, 
                 void *data)
#else
    static long
    myrIP_write (struct file *filp, const char *buf, size_t count, loff_t *ppos)
#endif
{

    #ifdef VERBOSE
	printk("myrIP_write() got called, cnt %d\n", (int)count);
    #endif /* VERBOSE */
    myrIPprocInit();
    return count;

}  /* end of myrIP_write() */

/*> <----------------------------------><----------------------------------> <*/

int
init_module(void)
{

static char dev_name[]= "myrIP0";


    #ifdef VERBOSE
	printk("myrIP: init_module() called\n");
    #endif /* VERBOSE */

    memset(&myrIPdev, 0, sizeof(myrIPdev));
#ifdef LINUX24
    strcpy(myrIPdev.name, dev_name);
#else
    myrIPdev.name= dev_name;
#endif
    myrIPdev.init= probe;

    #ifdef USE_PROC_FS
       proc_cplant= find_proc_cplant();
#ifdef LINUX24
       proc_myrIP = create_proc_read_entry("myrIP",
                              S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,
                              proc_cplant, myrIPproc, NULL);
       if (proc_myrIP) {
         proc_myrIP->write_proc = myrIP_write;
       }
       else {
         printk("myrIP, init_module: cannot create proc entry \"myrIP\"...\n");
         return -1;
       }
       proc_versions = create_proc_read_entry("versions.myrIP", 
                               S_IFREG | S_IRUGO, proc_cplant,
                               versions_proc, NULL);
       if (!proc_versions) {
         printk("myrIP, init_module: cannot create proc entry \"versions.myrIP\"...\n");
         return -1;
       }
#else
       proc_register(proc_cplant, &proc_myrIP);
       proc_register(proc_cplant, &proc_versions);
#endif
    #endif /* USE_PROC_FS */

    if (IP_init_recv() != 0)   {
	printk("myrIP: init_module() call to IP_init_recv() failed\n");

	#ifdef USE_PROC_FS
#ifdef LINUX24          
          remove_proc_entry("versions.myrIP", proc_cplant);
          remove_proc_entry("myrIP", proc_cplant);
#else
	  proc_unregister(proc_cplant, proc_versions.low_ino);
	  proc_unregister(proc_cplant, proc_myrIP.low_ino);
#endif
	#endif /* USE_PROC_FS */
	return -ENODEV;
    }

    register_IPrecv(IP_rcv_start, &myrIPdev);

    if (register_netdev(&myrIPdev) != 0)   {
	printk("myrIP: init_module() call to register_netdev() failed\n");

	#ifdef USE_PROC_FS
#ifdef LINUX24
            remove_proc_entry("versions.myrIP", proc_cplant);
            remove_proc_entry("myrIP", proc_cplant);
#else
	    proc_unregister(proc_cplant, proc_versions.low_ino);
	    proc_unregister(proc_cplant, proc_myrIP.low_ino);
#endif
	#endif /* USE_PROC_FS */
	return -ENODEV;
    }

    printk("myrIP module inserted\n");

    return 0;

}  /* end of init_module() */

/*> <----------------------------------><----------------------------------> <*/

void
cleanup_module(void)
{

int keep_loaded = 0;
struct NETDEV *dev;


    #ifdef VERBOSE
	printk("myrIP: cleanup_module called\n");
    #endif /* VERBOSE */

    #ifdef USE_PROC_FS
#ifdef LINUX24
      remove_proc_entry("versions.myrIP", proc_cplant);
      remove_proc_entry("myrIP", proc_cplant);
#else
      proc_unregister(proc_cplant, proc_versions.low_ino);
      proc_unregister(proc_cplant, proc_myrIP.low_ino);
#endif
    #endif /* USE_PROC_FS */

    printk("myrIP: proc_unregister done\n");
    dev = &myrIPdev;
    if (dev->flags & IFF_UP) {
	printk("myrIP: %s: device busy, remove delayed\n", dev->name);
	keep_loaded++;
    }


    if (!keep_loaded) {
	unregister_netdev(dev);
	printk("myrIP: unregister_netdev done\n");
	unregister_IPrecv();
	printk("myrIP: unregister_IPrecv done\n");
    }

}  /* end of cleanup_module() */

/*> <----------------------------------><----------------------------------> <*/
