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
/* $Id: ptRXTX.c,v 1.6 2001/10/23 22:30:46 jsotto Exp $ */

/* ptRXTX module, used to: 
 *      transfer portals package to sk_buff queue in the kernel, 
 *      receive sk_buff from kernel network, pass it up to portals
 *      V1: Current only printks, no real connections to portals
 */

#define EXPORT_SYMTAB                    /* for EXPORT_SYMBOL */

/* Needed for all kernel modules */
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/init.h>                  /* module initialization */
#include <linux/major.h>                 
#include <linux/fs.h>                  
#include </usr/include/sys/syscall.h>    /* temporary for syscalls */

#include <linux/netdevice.h>           /*for dev_base*/
#include <asm/uaccess.h>               /* copy_from_user */
#include <asm/system.h>                /* mb() */

#include "ptRXTX.h"

#ifdef LINUX24
#define NETDEV net_device
#define DEVGET dev_get_by_name
#else
#define NETDEV device
#define DEVGET dev_get
#endif

static int (*ptl_recv)(unsigned long page, struct NETDEV *dev);
static int ptRXTX_ioctl(struct inode *inodeP, struct file *fileP, 
                        unsigned int cmd, unsigned long arg);

struct NETDEV *ptRXTX_dev;
char macAddr[NODES][ETH_ALEN];

static int rx_fn_registered= 0;

static iface_t interface;

struct file_operations ptRXTX_fops = {

  ioctl: ptRXTX_ioctl,
};

static mac_addr_t mac_addr;

static int ptRXTX_ioctl(struct inode *inodeP, struct file *fileP, 
                        unsigned int cmd, unsigned long arg)
{
    int retval = 0, rc, nid;
#ifdef DEBUG_LOG
    int minor = MINOR(inodeP->i_rdev); 
    int major = MAJOR(inodeP->i_rdev); 

    PRINTF(0) ("ptRXTX_ioctl: (major,minor) = (%d,%d)\n", major, minor);
    PRINTF(0) ("ptRXTX_ioctl: caller's pid= %d\n", current->pid);
    PRINTF(0) ("ptRXTX_ioctl: action = %d\n", cmd);
#endif /* DEBUG_LOG */

    switch(cmd) {
	case PTRXTX_SET_MAC_ADDR:
          /* arg is the address in user-space of a
             struct mac_addr_t which contains a node
             id and a mac address */
          if ( arg ) {
            rc = copy_from_user((void*) &mac_addr, (void*)arg, 
                                                    sizeof(mac_addr_t)); 
            if (rc < 0) {
              printk("ptRXTX SET_MAC_ADDR ioctl: copy_from_user(mac_addr) failed\n");
              retval = -1;
            }
            else {
              nid = mac_addr.nid;
              macAddr[nid][0] = mac_addr.byte[0];
              macAddr[nid][1] = mac_addr.byte[1];
              macAddr[nid][2] = mac_addr.byte[2];
              macAddr[nid][3] = mac_addr.byte[3];
              macAddr[nid][4] = mac_addr.byte[4];
              macAddr[nid][5] = mac_addr.byte[5];
            }
          }
          else {
            printk("ptRXTX ioctl: null address in SET_MAC_ADDR\n");
            retval = -1;
          }
        break;
 
	case PTRXTX_GET_MAC_ADDR:
          /* arg is the address in user-space of a
             struct mac_addr_t which contains a node
             id -- we insert the corresponding mac address */
          if ( arg ) {
            rc = copy_from_user((void*) &mac_addr, (void*)arg, 
                                                    sizeof(mac_addr_t)); 
            if (rc < 0) {
              printk("ptRXTX GET_MAC_ADDR ioctl: copy_from_user(mac_addr) failed\n");
              retval = -1;
            }
            else {
              nid = mac_addr.nid;
              mac_addr.byte[0] = macAddr[nid][0];
              mac_addr.byte[1] = macAddr[nid][1];
              mac_addr.byte[2] = macAddr[nid][2];
              mac_addr.byte[3] = macAddr[nid][3];
              mac_addr.byte[4] = macAddr[nid][4];
              mac_addr.byte[5] = macAddr[nid][5];
            }
            rc = copy_to_user((void*) arg, (void*) &mac_addr, 
                                                    sizeof(mac_addr_t)); 
            if (rc < 0) {
              printk("ptRXTX GET_MAC_ADDR ioctl: copy_to_user(mac_addr) failed\n");
              retval = -1;
            } 
          }
          else {
            printk("ptRXTX ioctl: null address in SET_MAC_ADDR\n");
            retval = -1;
          }
          return retval;
        break;

	case PTRXTX_SET_IFACE:
           if ( !arg ) {
             retval = -1;
             printk("ptrxtx_ioctl: SET_IFACE -- bad arg\n");
             break;
           }
           rc = copy_from_user((void*)&interface, (void*)arg, sizeof(iface_t));
           if ( rc < 0 ) {
             retval = -1;
             printk("ptrxtx_ioctl: SET_IFACE -- bad copy from user\n");
           }
           ptRXTX_dev = DEVGET(interface.name);
        break;

	case PTRXTX_GET_IFACE:
           if ( !arg ) {
             retval = -1;
             printk("ptrxtx_ioctl: GET_IFACE -- bad arg\n");
             break;
           }
           rc = copy_to_user((void*)arg, (void*)&interface, sizeof(iface_t));
           if ( rc < 0 ) {
             retval = -1;
             printk("ptrxtx_ioctl: GET_IFACE -- bad copy to user\n");
           }
        break;

        default:
          printk("ptRXTX ioctl: default case -- unknown IOCTL\n");
          retval = -1;
        break;
    }
    return retval;
}

/*
 *      Main portals Receive routine.
 */ 
int portals_rcv(struct sk_buff *skb, struct NETDEV *dev, struct packet_type *pt)
{
  int rc;

  if ( !rx_fn_registered ) {
    return -1;
  }

  if((skb->len) == 46) 
    __skb_trim(skb, 44);             /* BAD BAD */
  
  rc = ptl_recv((unsigned long)skb->data, skb->dev);

  mb();          /* redundant? */

  kfree_skb(skb);
 
  return rc;
}


/*
 *      protocol layer initialiser
 */

static struct packet_type portals_packet_type =
{
  __constant_htons(PORTALS_PROT_ID),
  NULL,   /* All devices */
  portals_rcv,         /* forward declaration */
  (void*)1,
  NULL,
};

void eth_reg_rcv(int (*rcver)(unsigned long page, struct NETDEV *dev))
{
  ptl_recv = rcver;
  rx_fn_registered = 1;
}

EXPORT_SYMBOL(eth_reg_rcv);
EXPORT_SYMBOL(ptRXTX_dev);
EXPORT_SYMBOL(macAddr);

int init_module(void)
{
  dev_add_pack(&portals_packet_type);

  interface.name[0] = 0;

  if (register_chrdev(PTRXTX_MAJOR, "ptRXTX", &ptRXTX_fops)) {
    printk("ptRXTX init_modulue: unable to get major dev num = %d\n",
	    PTRXTX_MAJOR);
    return -EIO;
  }

  return 0;
}

void cleanup_module(void) 
{
  dev_remove_pack(&portals_packet_type);
  unregister_chrdev(PTRXTX_MAJOR, "ptRXTX");
  printk("Bye from ptRXTX module\n");
}
