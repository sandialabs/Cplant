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
/* module.c a Myrinet PCI driver for linux */
/* $Id: module.c,v 1.26.2.1 2002/10/07 15:59:27 ktpedre Exp $ */
/* derived from de4x5.c */
/*   A DIGITAL DE425/DE434/DE435 ethernet driver for linux.
     Copyright 1994, 1995 Digital Equipment Corporation.
*/
/*
** Cut to size and adapted for Puma portals by Sandia National Laboratories
*/

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

/* "This module only works with kernels >= 2.2.x" */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

#define EXPORT_SYMTAB

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/malloc.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/wrapper.h>
#include <linux/major.h>
#include <linux/fs.h>

#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/segment.h>
#include <asm/uaccess.h>	/* For copy_from_user() */

MODULE_AUTHOR("Rolf Riesen, Sandia National Laboratories");
MODULE_DESCRIPTION("Myrinet rtscts/portals driver");

#include "MCPshmem.h"
#include "hstshmem.h"
#include "Pkt_module.h"
#include "Pkt_recv.h"
#include "Pkt_send.h"
#include "Pkt_proc.h"		/* For PRINT_LINE_LEN */
#include "RTSCTS_send.h"
#include "RTSCTS_recv.h"
#include "RTSCTS_route.h"	/* For sendRouteStat() */
#include "RTSCTS_ip.h"		/* For *register_IPrecv() */
#include "RTSCTS_p3.h"		/* For p3_recvBody() */
#include "RTSCTS_ioctl.h"	/* for rtscts dev/ioctl stuff */
#include "RTSCTS_info.h"	/* for send_info_req() */
#include "RTSCTS_cache.h"	/* for cache related */
#include "RTSCTS_debug.h"	/* for protocol_debug_init() */
#include "RTSCTS_protocol.h"	/* For init_portocol() */
#include "queue.h"		/* For reset_all_queues() */
#include "../../portals/p3mod/devices.h" /* For P3 (un)register functions */
#include "../../addrCache/cache.h"       /* For addrCache register fn */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/in.h>
#include "printf.h"
#include "../include/myriInterface.h"	/* For lanai_reset_unit() */


#include "RTSCTS_recv.h"	/* For register_IPrecv(), unregister_IPrecv() */
#include <sys/defines.h>
#ifdef USE_PROC_FS
    #include <linux/proc_fs.h>
    #include "Pkt_proc.h"
    #include "RTSCTS_proc.h"
#endif /* USE_PROC_FS */

#ifdef RTSCTS_OVER_ETHERNET
/* functions to make ethernet transmission */
void eth_reg_rcv(int (*rcver)(unsigned long page, struct NETDEV *dev));
int cTaskGetPhysNid(void);
#endif

/*
** Local Variables
*/
static int p3slot;
static int test_route_len = 0;
static int route_id = -1;
int rte_nids[2];
static rts_ioctl_arg_t rts_ioctl_arg;
static int rts_util_buf_allocated = 0;
static unsigned int *rts_util_buf = NULL;

int reverse_route[MAX_ROUTE_LEN / sizeof(int)]
	    __attribute__ ((aligned (8)));
/*
** Global and external Variables
*/
hstshmem_t *realhstshmem;
hstshmem_t *hstshmem;
unsigned int *dmatst_area;

#ifndef RTSCTS_OVER_ETHERNET
extern struct NETDEV thisMYRI;
#endif

/*
** Public Functions
*/
int init_module(void);
void cleanup_module(void);

/*
** Private functions
*/
#ifndef LINUX24
static ssize_t myrpkt_read (struct file *filep, char *buf, size_t count,
	    loff_t *ppos);
static ssize_t rtscts_read(struct file *filep, char *buf, size_t count,
	    loff_t *ppos);
static ssize_t remote_read(struct file *filep, char *buf, size_t count,
            loff_t *ppos);
static ssize_t routes_read (struct file *filep, char *buf, size_t count,
	    loff_t *ppos);
static ssize_t routes_usage_read (struct file *filp, char *buf, size_t count,
	    loff_t *ppos);
#endif
static int rtscts_ioctl(struct inode*, struct file*, unsigned int,
	    unsigned long);

#ifdef LINUX24
static int myrpkt_write (struct file *filep, const char *buf,
	    unsigned long count, void *data);
static int rtscts_write(struct file *filep, const char *buf,
	    unsigned long count, void  *data);
static int routes_write (struct file *filep, const char *buf,
	    unsigned long count, void *data);
static int routes_usage_write (struct file *filep, const char *buf,
	    unsigned long count, void *data);
#else
static ssize_t myrpkt_write (struct file *filep, const char *buf,
	    size_t count, loff_t *ppos);
static ssize_t rtscts_write(struct file *filep, const char *buf,
	    size_t count, loff_t *ppos);
static ssize_t routes_write (struct file *filep, const char *buf,
	    size_t count, loff_t *ppos);
static ssize_t routes_usage_write (struct file *filep, const char *buf,
	    size_t count, loff_t *ppos);
#endif

extern int start_myri(struct NETDEV *dev);
extern void stop_myri(struct NETDEV *dev);

#ifdef RTSCTS_OVER_ETHERNET
mcpshmem_t noshmem;
mcpshmem_t *mcpshmem;
#endif


#ifdef USE_PROC_FS
#ifdef LINUX24
    struct file_operations rtscts_fops = {
	ioctl: rtscts_ioctl,
    };
#else
    struct file_operations myrpkt_fops = {
	read : myrpkt_read,
	write: myrpkt_write,
    };

    struct file_operations rtscts_fops = {
	read : rtscts_read,
	write: rtscts_write,
	ioctl: rtscts_ioctl,
    };

    struct file_operations remote_fops = {
	read : remote_read,
    };

    struct file_operations routes_fops = {
	read : routes_read,
	write: routes_write,
    };


    struct file_operations routes_usage_fops = {
	read : routes_usage_read,
	write: routes_usage_write,
    };

    struct inode_operations myrpkt_iops = {
        &myrpkt_fops,
    };

    struct inode_operations rtscts_iops = {
        &rtscts_fops,
    };

    struct inode_operations remote_iops = {
        &remote_fops,
    };

    struct inode_operations routes_iops = {
        &routes_fops,
    };

    struct inode_operations routes_usage_iops = {
        &routes_usage_fops,
    };
#endif

    /*
    ** The portals module already installed this. All we have to
    ** do is find it ;-)
    */
    struct proc_dir_entry *proc_cplant;

#ifdef LINUX24
    struct proc_dir_entry *proc_myrpkt=NULL;
    struct proc_dir_entry *proc_rtscts=NULL;
    struct proc_dir_entry *proc_remote=NULL;
    struct proc_dir_entry *proc_routes=NULL;
    struct proc_dir_entry *proc_routes_usage=NULL;
    struct proc_dir_entry *proc_proto_debug=NULL;
#ifdef BUFDEBUG
    struct proc_dir_entry *proc_sndbuf=NULL;
    struct proc_dir_entry *proc_rcvbuf=NULL;
#endif
    struct proc_dir_entry *proc_versions=NULL;
#else
    /* Install /proc/cplant/myrpkt */
    struct proc_dir_entry proc_myrpkt = {
	0,			/* low_ino: the inode -- dynamic */
	6, "myrpkt",		/* len of name and name */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	2048, &myrpkt_iops,	/* "file" size is 2k; inode ops */
    };

    /* Install /proc/cplant/rtscts */
    struct proc_dir_entry proc_rtscts = {
	0,			/* low_ino: the inode -- dynamic */
	6, "rtscts",		/* len of name and name */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	8192, &rtscts_iops,	/* "file" size is 8kB; inode ops */
    };

    /* Install /proc/cplant/remote */
    struct proc_dir_entry proc_remote = {
	0,			/* low_ino: the inode -- dynamic */
	6, "remote",		/* len of name and name */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	MAX_NUM_ROUTES * 26,	/* file size */
	&remote_iops,		/* inode ops */
    };

    /* Install /proc/cplant/routes */
    struct proc_dir_entry proc_routes = {
	0,			/* low_ino: the inode -- dynamic */
	6, "routes",		/* len of name and name */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	(MAX_NUM_ROUTES + 1) * PRINT_LINE_LEN,	/* File size */
	&routes_iops,		/* inode ops */
    };

    /* Install /proc/cplant/routes_usage */
    struct proc_dir_entry proc_routes_usage = {
	0,			/* low_ino: the inode -- dynamic */
	12, "routes_usage",	/* len of name and name */
	S_IFREG|S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR,/* mode: -rw-r--r-- */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	MAX_NUM_ROUTES * PRINT_USAGE_LINE_LEN,	/* File size */
	&routes_usage_iops,	/* inode ops */
    };

    /* Install /proc/cplant/proto_debug */
    struct proc_dir_entry proc_proto_debug =   {
	 0,			/* low_ino: the inode -- dynamic */
	 11, "proto_debug",	/* len of name and name */       
	 S_IFREG | S_IRUGO,	/* mode */
	 1, 0, 0,		/* nlinks, owner, group */
	 19043,			/* size */
	 NULL,			/* operations -- use default */
	 &read_proto_debug_proc,/* function used to read data */
    };

    #ifdef BUFDEBUG
    /* Install /proc/cplant/sndbuf */
    struct proc_dir_entry proc_sndbuf = {
	0,			/* low_ino: the inode -- dynamic */
	6, "sndbuf",		/* len of name and name */
	S_IFREG | S_IRUGO,	/* mode: regular, read by anyone */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	1024, NULL,		/* "file" size is 1024 B; inode ops unused */
	listSndBuf,		/* Function that lists the rcv buffers */
    };

    /* Install /proc/cplant/rcvbuf */
    struct proc_dir_entry proc_rcvbuf = {
	0,			/* low_ino: the inode -- dynamic */
	6, "rcvbuf",		/* len of name and name */
	S_IFREG | S_IRUGO,	/* mode: regular, read by anyone */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	1024, NULL,		/* "file" size is 1024 B; inode ops unused */
	listRcvBuf,		/* Function that lists the rcv buffers */
    };
    #endif /* BUFDEBUG */

    /* Install /proc/cplant/versions.rtscts */
    struct proc_dir_entry proc_versions = {
	0,			/* low_ino: the inode -- dynamic */
	15, "versions.rtscts",	/* len of name and name */
	S_IFREG | S_IRUGO,	/* mode: regular, read by anyone */
	1, 0, 0,		/* nlinks, owner (root), group (root) */
	2048, NULL,		/* "file" size is 1024 B; inode ops unused */
	versions_proc,		/* Function that lists the versions */
    };
#endif
#endif /* USE_PROC_FS */


/******************************************************************************/

EXPORT_SYMBOL(myr_send);
EXPORT_SYMBOL(register_IPrecv);
EXPORT_SYMBOL(unregister_IPrecv);

/******************************************************************************/

/* Code duplicated in module/IP_init.c */

struct proc_dir_entry *
find_proc_cplant(void)
{

struct proc_dir_entry *dir;


    dir= &proc_root;
    dir= dir->subdir;
    do   {
	if (strncmp("cplant", dir->name, 6) == 0)   {
	    PRINTF(0) ("Found /proc/cplant; we'll attach to that.\n");
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

/******************************************************************************/

#ifndef LINUX24
static ssize_t
myrpkt_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
char *start;
int cplen;

    cplen= myrPktProc(buf, &start, filp->f_pos, count, 0);
    #ifdef VERBOSE
	printk("myrpkt_read() got called, cnt %ld, f_pos %Ld, cplen %d\n",
	    count, filp->f_pos, cplen);
    #endif /* VERBOSE */
    filp->f_pos += cplen;
    return cplen;

}  /* end of myrpkt_read() */
#endif

#ifdef LINUX24
static int
myrpkt_write (struct file *filep, const char *buf, unsigned long count,
    void *data)
#else
static ssize_t
myrpkt_write (struct file *filep, const char *buf, size_t count,
    loff_t *ppos)
#endif
{

    #ifdef VERBOSE
	printk("myrpkt_write() got called, cnt %ld\n", (long)count);
    #endif /* VERBOSE */
    myrPktProcInit();
    return count;

}  /* end of myrpkt_write() */

/******************************************************************************/

#ifndef LINUX24
static ssize_t
rtscts_read(struct file *filep, char *buf, size_t count, loff_t *ppos)
{
char *start;
int cplen;

    cplen= rtsctsProc(buf, &start, filep->f_pos, count, 0);
    #ifdef VERBOSE
	printk("rtscts_read() got called, cnt %ld, f_pos %Ld, cplen %d\n",
	    (long)count, filep->f_pos, cplen);
    #endif /* VERBOSE */
    filep->f_pos += cplen;
    return cplen;

}  /* end of rtscts_read() */
#endif

#ifdef LINUX24
static int
rtscts_write(struct file *filep, const char *buf, unsigned long count,
    void *data)
#else
static ssize_t
rtscts_write(struct file *filep, const char *buf, size_t count,
    loff_t *ppos)
#endif
{
    rtsctsProcInit();

    if ((*buf == 'Q') || (*buf == 'q'))   {
	/* Clear the queues as well */
	reset_all_queues();
    }
    return count;

}  /* end of rtscts_write() */

static int
rtscts_ioctl(struct inode *inodeP, struct file *fileP, unsigned int cmd,
    unsigned long arg)
{
    int retval = 0, rc;
    #ifdef DEBUG_LOG
	int minor = MINOR(inodeP->i_rdev); 
	int major = MAJOR(inodeP->i_rdev); 

	PRINTF(0) ("rtscts_ioctl: (major,minor) = (%d,%d)\n", major, minor);
	PRINTF(0) ("rtscts_ioctl: caller's pid= %d\n", current->pid);
	PRINTF(0) ("rtscts_ioctl: action = %d\n", cmd);
    #endif /* DEBUG_LOG */

    switch(cmd) {
    case RTS_REQ_P3_PING:
        /* Generate a P3_PING_REQ packet which is P3 message rtscts
         * level ping. It can test P3 message sequencing connectivity
         * between two nodes.
        */
        retval = p3_send_ping(arg);
        break;
    
    case RTS_GET_P3_PING:
        /* Check for a P3_PING_ACK packet in response to a P3_PING_REQ
         * packet we sent to a given node.
        */
        retval = p3_get_ping(arg);
        break;
        
	case RTS_SET_TEST_ROUTE_LEN:
            /* let mcpshmem->test_route_len have the card endian
               let test_route_len have the host endian
            */
            if ( arg > MAX_TEST_ROUTE_LEN/sizeof(int)) {
              mcpshmem->test_route_len = 0;
              test_route_len = 0;
              retval = -1;
            }
            else {
              test_route_len = (int) arg;
              mcpshmem->test_route_len = (int) htonl(arg);
            }
            break;

        case RTS_SET_REVERSE_ROUTE:
            /* copy into kernel a route to be used in the pingr
               protocol where a reverse route intended for the
               ack is sent along with a ping msg -- the length
               is determined by test_route_len which implies a
               preceeding call to RTS_SET_TEST_ROUTE_LEN...
            */
            if (test_route_len == 0) {
              retval = -1;
            }
            rc = copy_from_user(reverse_route, (void*)arg, 
                                                       test_route_len); 
            break;

	case RTS_DO_TEST_ROUTE:
	    /* ping self along route pointed to by "arg"; 
               send our node id in msgID -- the destination
               should be TEST_ROUTE_ID rather than our physical
               node id... 
            */
            if (test_route_len == 0) {
              retval = -1;
            }
            else {
              /* copy the route to shared memory: the mcp's start_msg() 
                 routine will catch the TEST_ROUTE_ID dest flag and do the 
                 appropriate alignment of the route...
              */
              rc = copy_from_user(mcpshmem->test_route, (void*)arg, 
                                                         test_route_len); 
              /* even though we are doing a 1-way ping it makes sense
                 to set this here because we know it is a self-ping */
              mcpshmem->ping_stat = (int)htonl(-1);
	      sendProtoMSG(PING, next_msgID(), 
                           (unsigned short) TEST_ROUTE_ID, 0, 0,
                           0, NULL);
            }
	    break;

	case RTS_DO_TEST_ROUTE_W_SIZE:
	    /* Ping self along a specified route. The route and
         * other needed info for this case are contained in the
         * rts_ioctl_arg_t structure pointed to by "arg".
         *
         * Send our node id in msgID -- the destination
         * should be TEST_ROUTE_ID rather than our physical
         * node id... 
        */
        
        if (test_route_len == 0) {
            retval = -1;
        }
        else {
            /* Get the structure with our 'instructions'
            */
            rc = copy_from_user(&rts_ioctl_arg,(void*)arg,sizeof(rts_ioctl_arg)); 
            
            if (rts_ioctl_arg.type == 1) {
                /* This is just a probe to see if this ioctl cmd is
                 * supported. This really is just to support
                 * the cplant_lan_check util and its backward
                 * compatibility with older rtscts modules.
                */
                retval = 10;
                break;
            }
            else if (rts_ioctl_arg.type == 2) {
                if (!rts_util_buf_allocated) {
                    rts_util_buf = (unsigned int *)kmalloc(MYRPKT_MTU, GFP_DMA);
                    rts_util_buf_allocated = 1;
                }
                /* even though we are doing a 1-way ping it makes sense
                 * to set this here because we know it is a self-ping
                */
                mcpshmem->ping_stat = (int)htonl(-1);
                
                memcpy(&mcpshmem->test_route,&rts_ioctl_arg.route[0],1);
                sendProtoMSG(PING, next_msgID(), 
                             (unsigned short) TEST_ROUTE_ID, 0, 0,
                             rts_ioctl_arg.buflen, &rts_util_buf);
            }
        }
	    break;

	case RTS_DO_TEST_ROUTE_W_ACK:
	    /* ping self along route pointed to by "arg"; 
               send our node id in msgID -- the destination
               should be TEST_ROUTE_ID rather than our physical
               node id... 
            */
            if (test_route_len == 0) {
              retval = -1;
            }
            else {
              /* copy the route to shared memory: the mcp's start_msg() 
                 routine will catch the TEST_ROUTE_ID dest flag and do the 
                 appropriate alignment of the route...
              */
              rc = copy_from_user(mcpshmem->test_route, (void*)arg, 
                                                         test_route_len); 
              /* even though we are doing a 1-way ping it makes sense
                 to set this here because we know it is a self-ping */
              mcpshmem->ping_stat = (int)htonl(-1);
	      sendProtoMSG(PINGA, next_msgID(), 
                           (unsigned short) TEST_ROUTE_ID, 0, 0,
                           0, NULL);
            }
	    break;

	case RTS_DO_TEST_ROUTE_W_REV:
	    /* ping self along route pointed to by "arg"; 
               send our node id in msgID -- the destination
               should be TEST_ROUTE_ID rather than our physical
               node id... 
            */
            if (test_route_len == 0) {
              retval = -1;
            }
            else {
              /* copy the route to shared memory: the mcp's start_msg() 
                 routine will catch the TEST_ROUTE_ID dest flag and do the 
                 appropriate alignment of the route...
              */
              rc = copy_from_user(mcpshmem->test_route, (void*)arg, 
                                                         test_route_len); 
              /* even though we are doing a 1-way ping it makes sense
                 to set this here because we know it is a self-ping */
              mcpshmem->ping_stat = (int)htonl(-1);
	      sendProtoMSG(PINGR, next_msgID(), 
                           (unsigned short) TEST_ROUTE_ID, 0, 0,
                           test_route_len, (void*)reverse_route);
            }
	    break;

	case RTS_DO_PING:
	    /* ping node "arg"; send our node id in msgID */
	    sendProtoMSG(PING, next_msgID(), (unsigned short) arg, 0, 0,
                         0, NULL);
	    break;

	case RTS_SET_PING:
	    /* clear the ping info entry for node "arg" */
	    pINgFO[(unsigned int) arg] = 0;
            mcpshmem->ping_stat = (int)htonl(-1);
	    break;

	case RTS_GET_PING:
	    /* read the ping info entry for node "arg" */
	    retval = pINgFO[(unsigned int) arg];
	    break;

	case RTS_ROUTE_STAT:
	    #ifdef VERBOSE
		printk("rtscts_ioctl() got RTS_ROUTE_STAT, arg %ld\n", arg);
	    #endif /* VERBOSE */
            /* arg should be the destination node id */
	    retval= sendRouteStat((unsigned short int)arg, -1);
	    break;

	case RTS_ROUTE_CHECK:
	    retval= route_status[arg];
	    break;

        case RTS_ROUTE_REQ:
           /* this one causes a protocol msg to be sent to node
              *arg[0] requesting that it send a ROUTE_STAT msg to
              *arg[1], and inform us when (if) the ping comes back.
           */
            rc = copy_from_user(&rte_nids[0], (void*)arg, sizeof(int)); 
            if ( rc < 0) {
              printk("rtscts_ioctl(REQ1): copy 0 failed\n");
              retval = -1;
              break;
            }
            rc = copy_from_user(&rte_nids[1], (void*)(arg+sizeof(int)), sizeof(int)); 
	    #ifdef VERBOSE
              printk("rte_nids[0]: %d, rte_nids[1]: %d\n", rte_nids[0], rte_nids[1]);
            #endif
            
            if ( rc < 0) {
              printk("rtscts_ioctl(REQ1): copy 1 failed\n");
              retval = -1;
              break;
            }

            /* this will be updated by the handler for ROUTE_REQ_REPLY */
            route_request = ROUTE_NO_ANSWER;
            mcpshmem->route_request = (int)htonl(ROUTE_NO_ANSWER);

            /* send my ID in msgID */
	    sendProtoMSG(ROUTE_REQ, next_msgID(),
                                    (unsigned short) rte_nids[0], 
                                    (unsigned int) rte_nids[1], 0, 0, NULL);
	    break;

        case RTS_ROUTE_REQ_RESULT:
            return route_request;
            break;

        case RTS_SET_ROUTE_ID:
            /* it takes 2 steps to extract a route from shared
               memory; this 1st step ids the slot. RTS_GET_ROUTE
               does the rest
            */
            route_id = (int) arg;
            if ( route_id < 0 || route_id >= MAX_NUM_ROUTES ) {
              retval = -1;
              route_id = -1;
            }
            break;

        case RTS_GET_ROUTE:
            /* 2nd part of getting a route out of shared memory,
               see RTS_SET_ROUTE_ID
            */
               if (route_id == -1) {
                 retval = -1;
               }
               else {
                 rc = copy_to_user((void*)arg, mcpshmem->route[route_id], 
                                   MAX_ROUTE_LEN);
                 if (rc < 0) {
                   retval = -1;
                 }
               }
            break;

	case RTS_GET_INFO:
	    retval= get_info_data((void *)arg);
	    break;

	case RTS_REQ_INFO:
	    retval= send_info_req((void *)arg);
	    break;

	case RTS_CACHE_REQ:
            /* arg is the destination node id */
	    retval= send_cache_req((int)arg);
	    break;

	case RTS_CACHE_RETRIEVE_DATA:
	    retval= retrieve_cache_data(arg);
	    break;

	default:
	    printk("rtscts_ioctl: unknown action: %d\n", cmd);
	    retval = -EINVAL;
    }

    PRINTF(0) ("rtscts_ioctl: out retval = %d\n", retval);
    return retval;
}


/******************************************************************************/

#ifndef LINUX24
static ssize_t
routes_read (struct file *filp, char *buf, size_t count, loff_t *ppos)
{

char *start;
int cplen;


    cplen= routesProc(buf, &start, filp->f_pos, count, 0);
    #ifdef VERBOSE
	printk("routes_read() got called, cnt %ld, f_pos %Ld, cplen %d\n",
	    count, filp->f_pos, cplen);
    #endif /* VERBOSE */
    filp->f_pos += cplen;
    return cplen;

}  /* end of routes_read() */
#endif

#ifdef LINUX24
static int
routes_write (struct file *filep, const char *buf, unsigned long count,
    void *data)
#else
static ssize_t
routes_write (struct file *filep, const char *buf, size_t count,
    loff_t *ppos)
#endif
{

    #ifdef VERBOSE
	printk("routes_write() got called, cnt %ld\n", (long)count);
    #endif /* VERBOSE */
    routesProcInit();
    return count;

}  /* end of routes_write() */

/******************************************************************************/

#ifndef LINUX24
static ssize_t
remote_read(struct file *filep, char *buf, size_t count, loff_t *ppos)
{

char *start;
int cplen;


    cplen= remoteProc(buf, &start, filep->f_pos, count, 0);
    #ifdef VERBOSE
	printk("remote_read() got called, cnt %ld, f_pos %Ld, cplen %d\n",
	    (long)count, filep->f_pos, cplen);
    #endif /* VERBOSE */
    filep->f_pos += cplen;
    return cplen;

}  /* end of remote_read() */
#endif

/******************************************************************************/

#ifndef LINUX24
static ssize_t
routes_usage_read (struct file *filp, char *buf, size_t count, loff_t *ppos)
{

char *start;
int cplen;


    cplen= routesUsageProc(buf, &start, filp->f_pos, count, 0);
    #ifdef VERBOSE
	printk("routes_usage_read() got called, cnt %ld, f_pos %Ld, cplen %d\n",
	    count, filp->f_pos, cplen);
    #endif /* VERBOSE */
    filp->f_pos += cplen;
    return cplen;

}  /* end of routes_usage_read() */
#endif

#ifdef LINUX24
static int
routes_usage_write (struct file *filep, const char *buf, unsigned long count,
    void *data)
#else
static ssize_t
routes_usage_write (struct file *filep, const char *buf, size_t count,
    loff_t *ppos)
#endif
{

    #ifdef VERBOSE
	printk("routes_usage_write() got called, cnt %ld\n", (long)count);
    #endif /* VERBOSE */
    routes_usedProcInit();
    return count;

}  /* end of routes_usage_write() */

/******************************************************************************/

int
init_module(void)
{
struct NETDEV *dev= NULL;
int i; 
#ifndef RTSCTS_OVER_ETHERNET
int rc;
#endif


    PRINTF(2) ("RTS-CTS: init_module called\n");

    #ifdef DO_TIMEOUT_PROTOCOL
	PRINTF(2)("\n\t    +++++++++++++++++++++++++++++++++++++++\n");
	PRINTF(2)("\t    +++                                 +++\n");
	PRINTF(2)("\t    +++  Error Correction and Timeout   +++\n");
	PRINTF(2)("\t    +++  Protocol is Enabled            +++\n");
	PRINTF(2)("\t    +++                                 +++\n");
	PRINTF(2)("\t    +++++++++++++++++++++++++++++++++++++++\n");
    #else /* DO_TIMEOUT_PROTOCOL */
	PRINTF(2)("\n\t    +++++++++++++++++++++++++++++++++++++++\n");
	PRINTF(2)("\t    +++  Error Correction is off        +++\n");
	PRINTF(2)("\t    +++++++++++++++++++++++++++++++++++++++\n");
    #endif /* DO_TIMEOUT_PROTOCOL */

    #ifdef REQUEST_RESENDS
	PRINTF(2)("\n\t    :::::::::::::::::::::::::::::::::::::::\n");
	PRINTF(2)("\t    :::  Requesting Resends is ON       :::\n");
	PRINTF(2)("\t    :::::::::::::::::::::::::::::::::::::::\n");
    #else
	PRINTF(2)("\n\t    :::::::::::::::::::::::::::::::::::::::\n");
	PRINTF(2)("\t    :::  Requesting Resends is OFF      :::\n");
	PRINTF(2)("\t    :::::::::::::::::::::::::::::::::::::::\n");
    #endif /* REQUEST_RESENDS */

    #ifdef DROP_PKT_TEST
	PRINTF(2)("\n\t    ***************************************\n");
	PRINTF(2)("\t    ***                                 ***\n");
	PRINTF(2)("\t    ***       DROP_PKT_TEST is on!      ***\n");
	PRINTF(2)("\t    ***  We will drop packets to test   ***\n");
	PRINTF(2)("\t    ***  the error correction protocol. ***\n");
	PRINTF(2)("\t    ***                                 ***\n");
	PRINTF(2)("\t    ***************************************\n");
    #endif /* DROP_PKT_TEST */


    hstshmem = realhstshmem =
        (hstshmem_t *) kmalloc(sizeof(hstshmem_t) + ALIGN_64b, GFP_DMA);
    if (hstshmem == NULL) {
	printk("RTS-CTS: Couldn't allocate memory for the hstshmem_t structure");
	return -ENOMEM;
    }

    if ((unsigned long)hstshmem & ALIGN_64b)   {
	hstshmem= (hstshmem_t *)(((unsigned long)hstshmem + ALIGN_64b) &
			~ALIGN_64b);
	printk("RTS-CTS: Aligning hstshmem to %p\n", (void *)hstshmem);
    }

    /*
    ** See if it crosses a page boundary. If it does, and we're using the
    ** PYXIS 1.0 chipset, then we could be in trouble.
    */
    if (((unsigned long)hstshmem & PAGE_SIZE) !=
	    ((unsigned long)&hstshmem->pad2 & PAGE_SIZE))   {
	printk("RTS-CTS: WARNING! hstshmem crosses a page boundary!\n");
	printk("RTS-CTS: This could be trouble, if you are using the 1.0 PYXIS "
	    "chipset\n");
    }


    /* Get some memory to run PCI bus tests */
    dmatst_area = (unsigned int *) kmalloc(DMATST_AREA_SIZE, GFP_DMA);
    if (dmatst_area == NULL) {
	printk("RTS-CTS: Could not allocate memory for the DMA test area");
	return -ENOMEM;
    }


    for (i = 0; i < (sizeof(hstshmem_t) / sizeof(int)); i++) {
	*((int *) hstshmem + i) = -1;
    }

    /* Some things must not be -1 */
    hstshmem->LANai2host= htonl(NO_REASON);
    hstshmem->unaligned_snd_head= 0;
    hstshmem->unaligned_snd_tail= 0;
    hstshmem->unaligned_rcv_head= 0;
    hstshmem->unaligned_rcv_tail= 0;
    hstshmem->total_snd_bytes= 0;
    hstshmem->total_snd_msgs= 0;
    hstshmem->total_rcv_bytes= 0;
    hstshmem->total_rcv_msgs= 0;
    /* We're a rts/cts driver module */
    hstshmem->mod_type= htonl(MCP_TYPE_RTSCTS);

    init_protocol();


    /*
    ** Now print the sizes of some data structures (in hex as well, so we
    ** can make sure they're powers of 2
    */
    PRINTF(2)("RTS-CTS: DBL_BUF_SIZE                      %6d   0x%08x\n",
	DBL_BUF_SIZE, (unsigned int)DBL_BUF_SIZE);
    PRINTF(2)("RTS-CTS: sizeof(mcpshmem_t)                %6ld   0x%08x\n",
	sizeof(mcpshmem_t), (unsigned int)sizeof(mcpshmem_t));
    PRINTF(2)("RTS-CTS: sizeof(hstshmem_t)                %6ld   0x%08x\n",
	sizeof(hstshmem_t), (unsigned int)sizeof(hstshmem_t));
    PRINTF(2)("RTS-CTS: hstshmem start (virt) %p, end %p\n", (void *)hstshmem,
	(void *)((char *)hstshmem + sizeof(hstshmem_t)));
    PRINTF(2)("RTS-CTS: hstshmem start (bus) %p, end %p\n",
	(void *)virt_to_bus(hstshmem),
	(void *)virt_to_bus((char *)hstshmem + sizeof(hstshmem_t)));


    /* Register with the P3 module */
    p3slot= p3register_dev("rtscts Myrinet", p3_send, p3_recvBody, p3_dist, p3_exit);
    if (p3slot < 0)   {
	printk("rtscts: Registration with the Portals 3.0 module failed\n");
    } else   {
	PRINTF(2)("rtscts: Registration with the Portals 3.0 module successful\n");
    }


    /*
    ** RTS/CTS related initialization
    */
    #ifdef USE_PROC_FS
	/* Clear counters and put them into /proc */
	rtsctsProcInit();
	routesProcInit();
	routes_usedProcInit();
	protocol_debug_init();
	proc_cplant= find_proc_cplant();
#ifdef LINUX24
	/* need an array whose entries we can loop thru... */
 
	/*--------------------------------------------------------------------*/
	proc_myrpkt = create_proc_read_entry("myrpkt", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, myrPktProc, NULL);
	if (proc_myrpkt) {
	    proc_myrpkt->write_proc = myrpkt_write;
	} else {
	    printk("rtscts, init: cannot create proc entry \"myrpkt\"...\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_rtscts = create_proc_read_entry("rtscts", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, rtsctsProc, NULL);
	if (proc_rtscts) {
	    proc_rtscts->write_proc = rtscts_write;
	} else {
	    printk("rtscts, init: cannot create proc entry \"rtscts\"...\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_remote = create_proc_read_entry("remote", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, rtsctsProc, NULL);
	if (proc_remote) {
	    proc_remote->write_proc = NULL;
	} else {
	    printk("rtscts, init: cannot create proc entry \"remote\"...\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_routes = create_proc_read_entry("routes", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, routesProc, NULL);
	if (proc_routes) {
	    proc_routes->write_proc = routes_write;
	} else {
	    printk("rtscts, init: cannot create proc entry \"routes\"...\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_routes_usage = create_proc_read_entry("routes_usage", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, routesUsageProc, NULL);
	if (proc_routes_usage) {
	    proc_routes_usage->write_proc = routes_usage_write;
	} else {
	    printk("rtscts, init: cannot create proc entry \"routes_usage\"\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_proto_debug = create_proc_read_entry("proto_debug", 
			S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR,
			proc_cplant, read_proto_debug_proc, NULL);
	if (proc_proto_debug) {
	    proc_proto_debug->write_proc = NULL;
	} else {
	    printk("rtscts, init: cannot create proc entry \"proto_debug\"\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	#ifdef BUFDEBUG
	proc_sndbuf = create_proc_read_entry("sndbuf", S_IFREG | S_IRUGO,
			proc_cplant, listSndBuf, NULL);
	if (proc_sndbuf) {
	    proc_sndbuf->write_proc = NULL;
	} else {
	    printk("rtscts, init: cannot create proc entry \"sndbuf\"...\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
	proc_rcvbuf = create_proc_read_entry("rcvbuf", S_IFREG | S_IRUGO,
			proc_cplant, listRcvBuf, NULL);
	if (proc_rcvbuf) {
	    proc_rcvbuf->write_proc = NULL;
	} else {
	    printk("rtscts, init: cannot create proc entry \"rcvbuf\"...\n");
	    return -1;
	}
	#endif /* BUFDEBUG */
	/*--------------------------------------------------------------------*/
	proc_versions = create_proc_read_entry("versions.rtscts",
			S_IFREG | S_IRUGO,
			proc_cplant, versions_proc, NULL);
	if (proc_versions) {
	    proc_versions->write_proc = NULL;
	} else {
	    printk("rtscts, init: can't create /proc/cplant/versions.rtscts\n");
	    return -1;
	}
	/*--------------------------------------------------------------------*/
#else
	proc_register(proc_cplant, &proc_rtscts);
	proc_register(proc_cplant, &proc_remote);
	proc_register(proc_cplant, &proc_myrpkt);
	proc_register(proc_cplant, &proc_routes);
	proc_register(proc_cplant, &proc_routes_usage);
	proc_register(proc_cplant, &proc_proto_debug);
	#ifdef BUFDEBUG
	    proc_register(proc_cplant, &proc_rcvbuf);
	    proc_register(proc_cplant, &proc_sndbuf);
	#endif /* BUFDEBUG */
	proc_register(proc_cplant, &proc_versions);
#endif /* LINUX24 */
    #endif /* USE_PROC_FS */

    #ifdef DO_TIMEOUT_PROTOCOL
	schedule_rtscts_timeout();
    #endif /* DO_TIMEOUT_PROTOCOL */

    /*** ----------------------------------------------------------------- ***/

#ifdef RTSCTS_OVER_ETHERNET
    mcpshmem = &noshmem;
    hstshmem->my_pnid = -1;
    hstshmem->my_pnid = cTaskGetPhysNid();
    if (hstshmem->my_pnid == -1) {
      printk("init_module: bad physical node id, probably a configuration issue... failing module initialization for rtscts\n");
      return -1;
    }
#else
    rc = start_myri(dev);
    if (rc < 0) {
      return rc;
    }
#endif

    if (register_chrdev(RTSCTS_MAJOR, "rtscts", &rtscts_fops)) {
	printk("RTS-CTS init_modulue: unable to get major_device_num = %d\n",
	    RTSCTS_MAJOR);
	free_irq(dev->irq, dev);
	PRINTF(0) ("Freeing IRQ %d for dev %s\n", dev->irq, dev->name);
	printk("*** Should free a whole bunch of resources!! (See above)\n");
	return -EIO;
    }

#ifndef RTSCTS_OVER_ETHERNET
    PRINTF(2)("RTS-CTS packet/driver module inserted for device \"%s\"\n",
	(&thisMYRI)->name);
#else
    PRINTF(2)("RTS-CTS packet/driver module inserted for ethernet device\n");
#endif

#ifdef KERNEL_ADDR_CACHE
    addrCache_reg_fn(send_cache_data);
#endif

#ifdef RTSCTS_OVER_ETHERNET
    eth_reg_rcv(rtscts_recv);
#endif

    printk("RTSCTS module inserted\n");
    
    return 0;
}  /* end of init_module() */

/******************************************************************************/

void
cleanup_module(void)
{

#ifndef RTSCTS_OVER_ETHERNET
struct NETDEV *dev;
#endif

    PRINTF(0) ("cleanup_module called for MYRINET\n");
    #ifdef DO_TIMEOUT_PROTOCOL
	remove_rtscts_timeout();
    #endif /* DO_TIMEOUT_PROTOCOL */

    if (rts_util_buf != NULL) {
        kfree(rts_util_buf);
    }
    
    /*
    ** Get out of /proc
    */
    #ifdef USE_PROC_FS
#ifdef LINUX24
	remove_proc_entry("versions.rtscts", proc_cplant);
#ifdef BUFDEBUG
        remove_proc_entry("rcvbuf", proc_cplant);
        remove_proc_entry("sndbuf", proc_cplant);
#endif /* BUFDEBUG */
        remove_proc_entry("proto_debug", proc_cplant);
        remove_proc_entry("routes_usage", proc_cplant);
        remove_proc_entry("routes", proc_cplant);
        remove_proc_entry("remote", proc_cplant);
        remove_proc_entry("rtscts", proc_cplant);
        remove_proc_entry("myrpkt", proc_cplant);
#else
	proc_unregister(proc_cplant, proc_versions.low_ino);
	#ifdef BUFDEBUG
	    proc_unregister(proc_cplant, proc_rcvbuf.low_ino);
	    proc_unregister(proc_cplant, proc_sndbuf.low_ino);
	#endif /* BUFDEBUG */
	proc_unregister(proc_cplant, proc_proto_debug.low_ino);
	proc_unregister(proc_cplant, proc_routes_usage.low_ino);
	proc_unregister(proc_cplant, proc_routes.low_ino);
	proc_unregister(proc_cplant, proc_myrpkt.low_ino);
	proc_unregister(proc_cplant, proc_remote.low_ino);
	proc_unregister(proc_cplant, proc_rtscts.low_ino);
#endif /* LINUX24 */
    #endif /* USE_PROC_FS */

    pkt_free_rcv_pages();
    pkt_free_snd_pages();

    kfree(realhstshmem);
    if (dmatst_area)   {
	kfree(dmatst_area);
    }

    /* Tell the Portals 3.0 module, that we're out */
    p3unregister_dev(p3slot);
    printk("RTS-CTS packet module removed\n");

    /*** ----------------------------------------------------------------- ***/

#ifndef RTSCTS_OVER_ETHERNET
    dev = &thisMYRI;
    stop_myri(dev);

    /* Stop the MCP and hold the board in reset */
    printk("Stopping the MCP and hold the board in reset\n");
    lanai_reset_unit(0, LANAI_ON);
#endif

    unregister_chrdev(RTSCTS_MAJOR, "rtscts");

}  /* end of cleanup_module() */
