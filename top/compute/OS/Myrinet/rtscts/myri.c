/* probe.c a Myrinet PCI driver for linux */
/* $Id: myri.c,v 1.75 2002/02/09 00:38:41 pumatst Exp $ */
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

#include <linux/kernel.h>
#include <linux/if_arp.h>		/* For ARPHRD_ETHER */
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/delay.h>

#include "hstshmem.h"
#include "Pkt_module.h"
#include "Pkt_recv.h"
#include "Pkt_send.h"
#include "Pkt_proc.h"			/* For PRINT_LINE_LEN */
#include "../../portals/p3mod/devices.h" /* For P3 (un)register functions */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/* support old naming sillyness */
#if LINUX_VERSION_CODE < 0x020100
#define iounmap vfree
#endif

/*
** Begin Patch Part 1/2
** patch to support shared IRQ - Stephane Amarger - Canon CRF - Nov. 15, 1996
** inspired by Donald Becker 3c59x driver
** Here, only the call to request_irq will be affected by the value of the
** flag
**                      flag = 0 if SA_SHIRQ is not defined
**                      flag = SA_SHIRQ which is of course defined
*/

#include <asm/signal.h>	/* contains the definition of irq flags */

#ifdef SA_SHIRQ
    static unsigned int IRQ_FLAGS = SA_SHIRQ;
#else
    static unsigned int IRQ_FLAGS = 0;
#endif

/* End Patch Part 1/2 */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/in.h>
#include "lanai_device.h"
#include "myri.h"
#include "printf.h"
#include "MyrinetPCI.h"
#include "myriInterface.h"

/* #define PRINTF(x) printk */


#include "Pkt_handler.h"
#include "RTSCTS_recv.h"	/* For register_IPrecv(), unregister_IPrecv() */
#include <sys/defines.h>
#ifdef USE_PROC_FS
    #include <linux/proc_fs.h>
    #include "Pkt_proc.h"
    #include "RTSCTS_proc.h"
#endif /* USE_PROC_FS */


/*
** Local Variables
*/
static int myri_boards_found = 0;
static int myri_mem_problem = 0;
static int num_myris = 0;
static int max_boards = 16;
static int io = 16;
static int p3slot;

/*
** Global and external Variables
*/
int _intlvl= 0;
int shared_interrupts = 0;
struct NETDEV thisMYRI1[7];
int mcp_type[]= {L0, L0, L0, L0, L0, L0, L0, L0};

extern hstshmem_t *hstshmem;
extern int mlanai_count;
extern unsigned int XXdriver_clockval[16];	/* in myriPut.c */
extern struct mlanai_info mlanai_private[MLANAI_MAX];
extern struct NETDEV thisMYRI;

#ifdef USE_PROC_FS
extern struct proc_dir_entry *proc_cplant;
extern struct proc_dir_entry proc_versions;
extern struct proc_dir_entry proc_rcvbuf;
extern struct proc_dir_entry proc_sndbuf;
extern struct proc_dir_entry proc_proto_debug;
extern struct proc_dir_entry proc_rtscts;
extern struct proc_dir_entry proc_routes_usage;
extern struct proc_dir_entry proc_routes;
extern struct proc_dir_entry proc_myrpkt;
#endif


/*
** External functions
*/
struct MYRINET_BOARD * myrinet_init_pointers_linux(int unit,
	    unsigned int board_base, unsigned char revision);


/*
** Public Functions
*/
int myri_allocate_irq(struct NETDEV * dev, u_short dev_num, u_short pb);

static int myri_hw_init(struct NETDEV * dev, int dev_num, int iobase,
	int num_units);
static int myri_pci_probe(struct NETDEV * dev, short iobase);
static struct NETDEV *myri_alloc_device(struct NETDEV * dev,
	unsigned int iobase, int num_units);
static void myri_interrupt(int irq, void *dev_id, struct pt_regs * regs);

int start_myri(struct NETDEV *dev);
void stop_myri(struct NETDEV *dev);
int myri_probe(struct NETDEV * dev);

int
myri_probe(struct NETDEV * dev)
{

int tmp = num_myris;
int iobase = dev->base_addr;
int status = -ENODEV;

    PRINTF(0) ("myri_probe called dev=%p  num_myris= %d  iobase = 0x%x\n",
	dev, num_myris, iobase);

    status = myri_pci_probe(dev, iobase);

    if (status < 0) {
	/* what to do about cleanup?? */
	/* I think we're OK */
    }
    if ((tmp == num_myris) && (iobase != 0)) {
	PRINTF(0) ("%s: myri_probe() cannot find device at 0x%04x.\n",
	    dev->name, iobase);
    }

    /*
    ** Walk the device list to check that at least one device initialised OK
    */
    for (; (dev->priv == NULL) && (dev->next != NULL); dev = dev->next);

    if (dev->priv)
	status = 0;

    return status;
}

/******************************************************************************/

static int
myri_hw_init(struct NETDEV * dev, int dev_num, int iobase, int num_units)
{

int status = 0;
struct myri_private *mp;
void *free0 = NULL;


    dev->base_addr = iobase;

    if (dev) {
	PRINTF(0) ("myri_hw_init(%p): %s at 0x%x (PCI device %d)  unit=%d\n",
	    dev, dev->name, iobase, dev_num, num_units);
    } else {
	PRINTF(0) ("myri_hw_init(%p): at 0x%x (PCI device %d)  unit=%d\n",
	    dev, iobase, dev_num, num_units);
    }

    if (!dev->priv) {
	mp= dev->priv= (void *)kmalloc(sizeof(struct myri_private), GFP_KERNEL);

	/* save the original kmalloc address */
	free0 = dev->priv;

	PRINTF(0) ("(1) allocated myri_private dev->priv = %p\n",
	    dev->priv);
	if (!dev->priv) {
	    PRINTF(0) ("(1) dev->priv is NULL\n");
	    myri_mem_problem++;
	    return (-ENOMEM);
	}
    } else {
	PRINTF(0) ("(1) myri_private dev->priv already allocated = %p\n",
	    dev->priv);
	mp = dev->priv;
    }

    memset(mp, 0, sizeof(struct myri_private));

    if (free0) {
	mp->free0 = free0;
    }
    mp->dev = dev;
    mp->unit = num_units;

    /* create storage space for the device name */
    sprintf(mlanai_private[num_units].devname, "myrPtl%d", num_units);
#ifdef LINUX24
    strcpy(dev->name, mlanai_private[num_units].devname);
#else
    dev->name = mlanai_private[num_units].devname;
    //sprintf(dev->name, "myrPtl%d", num_units);
#endif

    status = 0;

    if (!status) {
	/* The MYRI-specific entries in the device structure. */
	dev->hard_start_xmit= NULL;
	dev->open= NULL;
	dev->stop= NULL;
	dev->hard_header= NULL;
	dev->rebuild_header= NULL;
	dev->get_stats= NULL;
	dev->do_ioctl= NULL;
	dev->change_mtu= NULL;
	dev->set_config= NULL;

	dev->mem_start = 0;
	sprintf(mp->adapter_name, "%s (%s)", "Myrinet", dev->name);

	dev->mtu                = MYRPKT_MTU - sizeof(struct ethhdr);
#ifndef LINUX24
	dev->tbusy              = 0;
#endif
	dev->hard_header_len    = ETH_HLEN;             /* 14 */
	dev->addr_len           = ETH_ALEN;             /* 6  */
	dev->tx_queue_len       = MAX_SND_PKT_ENTRIES;
	dev->type               = ARPHRD_ETHER;
	dev->flags              = dev->flags & ~(IFF_MULTICAST | IFF_BROADCAST);
    }
    return status;
}

/******************************************************************************/
/*
** The MYRI interrupt handler.
*/
static void
myri_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{

struct NETDEV *dev_orig;
struct myri_private *lp;
unsigned long __flags;

    #ifdef SA_SHIRQ
	dev_orig= (struct NETDEV *)dev_id;
    #else
	dev_orig= (struct NETDEV *)(irq2dev_map[irq]);
    #endif /* SA_SHIRQ */

    if (!dev_orig) {
	printk("myri_interrupt(): irq %d for null device.\n", irq);
	return;
    }
    lp = (struct myri_private *) dev_orig->priv;
 

    save_flags(__flags);
    cli();
#ifndef LINUX24
    if (dev_orig->interrupt) {
	restore_flags(__flags);
	printk("%s: Ree-entering interrupt handler!\n", dev_orig->name);
	return;
    }
    dev_orig->interrupt= 1;
#endif
    myrpkt_handle_interrupt(lp->unit, dev_orig, mcp_type[lp->unit]);
    restore_flags(__flags);
#ifndef LINUX24
    dev_orig->interrupt= 0;
#endif

}

/******************************************************************************/

static int
init_pci_card(struct NETDEV *dev, struct pci_dev *pcidev)
{

u_char irq;
u_char revision;
u_char pb, dev_num;
u_short status;
int rv = 0, changed = 0;
u_int iobase;
struct myri_private *mp;
struct MYRINET_BOARD *mb = NULL;


    pb = pcidev->bus->number;
    dev_num = pcidev->devfn;
    myri_boards_found++;
    PRINTF(0) ("Myrinet PCI board found  bus(%d)  device(%d) found\n",
	pb, dev_num);
    PRINTF(0) ("config regs follow:");

    if (PRINT_LEVEL >= 0) {
	int i;
	unsigned char data;

	for (i = 0; i < 64; i++) {
	    pcibios_read_config_byte(pb, dev_num, i, &data);
	    if ((i % 4) == 0) {
		    PRINTF(0) ("\n%02x: ", i);
	    }
	    if (data) {
		    PRINTF(0) ("%02x ", data);
	    } else {
		    PRINTF(0) ("   ");
	    }
	}
	PRINTF(0) ("\n");
    }

    pcibios_read_config_dword(pb, dev_num, PCI_BASE_ADDRESS_0, &iobase);

    /* Clear prefetch and other bits */
    iobase &= ~0xFF;
    irq = pcidev->irq;

    /*
     * Enable  Memory Access and Bus
     * Mastering
     */
    pcibios_read_config_word(pb, dev_num, PCI_COMMAND, &status);

    if (!(status & PCI_COMMAND_MEMORY)) {
	PRINTF(0) ("myri[%d]: Memory Access Enable bit NOT set\n",
	    num_myris);
	status |= PCI_COMMAND_MEMORY;
	changed++;
    }
    if (!(status & PCI_COMMAND_MASTER)) {
	PRINTF(0) ("myri[%d]: Master DMA Enable bit NOT set\n",
	    num_myris);
	status |= PCI_COMMAND_MASTER;
	changed++;
    }
    if (!(status & PCI_COMMAND_INVALIDATE)) {
	PRINTF(0) ("myri[%d]: Mem Write & Invalidate bit NOT set\n",
	    num_myris);
	status |= PCI_COMMAND_INVALIDATE;
	changed++;
    }
    if (changed) {
	PRINTF(0) ("myri[%d]: Setting PCI command reg = 0x%x\n",
	    num_myris, status);
	pcibios_write_config_word(pb, dev_num, PCI_COMMAND, status);
	pcibios_read_config_word(pb, dev_num, PCI_COMMAND, &status);
    }


    /* get the revision number */
    pcibios_read_config_byte(pb, dev_num, PCI_REVISION_ID, &revision);
    PRINTF(0) ("myri board revision = %d\n", revision);


    if ((dev = myri_alloc_device(dev, iobase, num_myris)) != NULL) {
	dev->irq = irq;
	dev->base_addr = iobase;

	if ((rv = myri_hw_init(dev, dev_num, iobase, num_myris)) == 0) {
	    PRINTF(0) ("(1) incrementing num_myris=%d\n", num_myris);
	    num_myris++;
	} else {
	    PRINTF(0) ("myri_hw_init returned bad status = %d -- "
		"exiting \n", rv);
	    return rv;
	}
    } else {
	PRINTF(0) ("myri_alloc_device returned FAIL?\n");
	myri_mem_problem++;
	return -ENXIO;
    }

    mp = (struct myri_private *) dev->priv;
    mb = (struct MYRINET_BOARD *) (unsigned long) iobase;

    mp->mb = mb;
    mp->unit = num_myris - 1;
    mp->revision = revision;


    /* turn off DMA_MASTER while setting clockval */
    pcibios_read_config_word(pb, dev_num, PCI_COMMAND, &status);

    status &= ~PCI_COMMAND_MASTER;

    PRINTF(0) ("myri[%d]: Setting PCI command reg = 0x%x  Turn OFF "
	"dma_master\n", mp->unit, status);
    pcibios_write_config_word(pb, dev_num, PCI_COMMAND, status);

    PRINTF(0) ("Calling myrinet_init_pointers_linux unit = %d & %d\n",
	mp->unit, num_myris - 1);

    mp->mb_remap = myrinet_init_pointers_linux(mp->unit, iobase, revision);
    if (!mp->mb_remap) {
	printk("Myrinet init pointers failed\n");
	return -ENXIO;
    }

    PRINTF(0) ("myri[%d]: clockval used == 0x%08x\n", mp->unit,
	XXdriver_clockval[mp->unit]);

    /* turn on DMA_MASTER after setting clockval */
    status |= PCI_COMMAND_MASTER;

    PRINTF(0) ("myri[%d]: Setting PCI command reg = 0x%x  Turn ON "
	"dma_master\n", num_myris, status);
    pcibios_write_config_word(pb, dev_num, PCI_COMMAND, status);


    if (revision == 1) {
#ifndef LINUX24
	dev->interrupt = 1;
#endif
	mcp_type[mp->unit]= L4;
	setEIMR(mp->unit, L4, 0);
	lanai_reset_unit(mp->unit, LANAI_ON);

	PRINTF(0) ("myri[%d]: Requesting IRQ = %d\n", mp->unit, dev->irq);

	lanai_interrupt_unit(mp->unit, LANAI_OFF);
	setEIMR(mp->unit, L4, 0);
	/* now clear the bit if set */
	setISR(mp->unit, L4, HOST_SIG_BIT);

	if (myri_allocate_irq(dev, dev_num, pb)) {
	    /* OK */
	} else {
	    lanai_interrupt_unit(mp->unit, LANAI_OFF);
	    setEIMR(mp->unit, L4, 0);
	    /* now clear the bit if set */
	    setISR(mp->unit, L4, HOST_SIG_BIT);
	}
#ifndef LINUX24
	dev->interrupt = 0;
#endif

    } else   {
#ifndef LINUX24
	dev->interrupt = 1;
#endif
	mcp_type[mp->unit]= L7;
	setEIMR(mp->unit, L7, 0);
	lanai_reset_unit(mp->unit, LANAI_ON);

	PRINTF(0) ("myri[%d]: Requesting IRQ = %d\n", mp->unit, dev->irq);

	lanai_interrupt_unit(mp->unit, LANAI_OFF);
	setEIMR(mp->unit, L7, 0);
	/* now clear the bit if set */
	setISR(mp->unit, L7, HOST_SIG_BIT);

	if (myri_allocate_irq(dev, dev_num, pb)) {
	    /* OK */
	} else {
	    lanai_interrupt_unit(mp->unit, LANAI_OFF);
	    setEIMR(mp->unit, L7, 0);
	    /* now clear the bit if set */
	    setISR(mp->unit, L7, HOST_SIG_BIT);
	}

#ifndef LINUX24
	dev->interrupt = 0;
#endif
    } 

    /*
     * initialize the stuff for the
     * mmap() driver
     */
    mlanai_init(mp);
    PRINTF(0) ("Calling register_netdev for %s\n", dev->name);
    register_netdev(dev);

    return 0;

}  /* end of init_pci_card() */

/******************************************************************************/

/*
** PCI bus I/O device probe
*/
static int
myri_pci_probe(struct NETDEV *dev, short ioaddr)
{

struct pci_dev *pcidev = NULL;
int rc = 0;


    PRINTF(0) ("myri_pci_probe(dev = %p ioaddr = %04x) called\n", dev, ioaddr);

    if (pcibios_present()) {

	/* Look for AMCC Myrinet PCI cards */
	while ((pcidev= pci_find_device(MYRINET_PCI_VENDOR_ID,
			    MYRINET_PCI_DEVICE_ID, pcidev)))   {
	    PRINTF(0) ("Found AMCC Myrinet PCI card: vendor 0x%04x, "
		"device 0x%04x\n", pcidev->vendor, pcidev->device);
	    rc= init_pci_card(dev, pcidev);
	}

	/* Look for Myrinet PCI cards with the new vendor ID */
	while ((pcidev= pci_find_device(MYRINET_PCI_VENDOR_ID_2,
			    MYRINET_PCI_DEVICE_ID, pcidev)))   {
	    PRINTF(0) ("Found new style Myrinet PCI card: vendor 0x%04x, "
		"device 0x%04x\n", pcidev->vendor, pcidev->device);
	    rc= init_pci_card(dev, pcidev);
	}
    }

    return rc;
}
/******************************************************************************/

/*
** Allocate the device by pointing to the next available space in the
** device structure. Should one not be available, it is created.
*/
static struct NETDEV *
myri_alloc_device(struct NETDEV * dev, unsigned int iobase, int num_units)
{

    PRINTF(5)("myri_alloc_device called: dev= %p iobase= 0x%x num_units= %d\n",
	dev, iobase, num_units);

    /*
    ** Check the device structures for an end of list or unused device
    */

    if ((dev == &thisMYRI) && (!dev->base_addr)) {
	PRINTF(0) ("myri_alloc_device: myri0 device allocated statically, "
	    "returning\n");
	return dev;
    }
    if (dev->next) {
	PRINTF(0) ("myri_alloc_device: dev->next = %p  not NULL!\n",
	    dev->next);
	return NULL;
    }
    dev->next = (struct NETDEV *) kmalloc(sizeof(struct NETDEV), GFP_KERNEL);
    if (dev->next) {
	PRINTF(0) ("myri_alloc_device: allocated a new DEV structure = %p\n",
	    dev->next);
	memset(dev->next, 0, sizeof(struct NETDEV));
    }
    dev = dev->next;

    if (dev == NULL) {
	PRINTF(0) ("myri_alloc_device:  myri%d: Device not initialised, "
	    "not enough memory\n", num_units);
	return NULL;
    }

    /*
    ** If the memory was allocated, point to the new memory area * and
    ** initialize it (name, I/O address, next device (NULL) and *
    ** initialisation probe routine).
    */
    PRINTF(5) ("myri_alloc_device:  initialize the memory\n");

    dev->next = NULL;

    /* create storage space for the device name */
    sprintf(mlanai_private[num_units].devname, "myrPtl%d", num_units);
#ifdef LINUX24
    strcpy(dev->name, mlanai_private[num_units].devname);
#else
    dev->name = mlanai_private[num_units].devname;
    //sprintf(dev->name, "myrPtl%d", num_units);	/* New device name */
#endif

    dev->base_addr = iobase;
    dev->next = NULL;
    dev->init = NULL;

    PRINTF(0)("myri_alloc_device: completed new device allocation newdev= %p\n",
	dev);

    return dev;
}

/******************************************************************************/

int
myri_allocate_irq(struct NETDEV * dev, u_short dev_num, u_short pb)
{
struct myri_private *mp = (struct myri_private *) dev->priv;

    /* try without sharing */
    if (request_irq(dev->irq, (void *)myri_interrupt, 0,
	    mp->adapter_name, dev)) {
	PRINTF(0) ("      Requested IRQ%d is already used\n", dev->irq);
	PRINTF(0) ("      Will try to get it via Sharing\n");
    } else {
	PRINTF(0) ("      Requested IRQ%d correctly allocated\n", dev->irq);
	return (1);
    }

/*
** Begin Patch Part 2/2
** patch to support shared IRQ -- Stephane Amarger - Canon CRF - Nov. 15, 1996
** inspired by Donald Becker 3c59x driver
*/
    if (request_irq(dev->irq, (void *) myri_interrupt, IRQ_FLAGS,
	    mp->adapter_name, dev)) {
	PRINTF(0) ("      No IRQ was correctly allocated\n");
	return (0);
    } else {
	PRINTF(0) ("      Requested IRQ%d correctly allocated\n", dev->irq);
	shared_interrupts++;
	return (1);
    }
/* End Patch Part 2/2 */
}

/******************************************************************************/
/*
** We probe when we insert the module. However, ifconfig wants to see
** (and call) a valid function in the device structure. So, here is one ;-)
*/
static int
dummy_probe(struct NETDEV *dev)
{
    return 0;
}
/******************************************************************************/
/***********  MODULE   MODULE    MODULE    MODULE    MODULE *******************/

struct NETDEV thisMYRI =
{
  next: NULL,
  init: dummy_probe,
};

/******************************************************************************/

int start_myri(struct NETDEV *dev) 
{
struct myri_private *mp;
int index, rv;

    thisMYRI.base_addr = (unsigned long) NULL;
    thisMYRI.irq = 10;

#ifdef LINUX24
    strcpy("myrPkt0", mlanai_private[0].devname);
#else
    thisMYRI.name = mlanai_private[0].devname;
#endif
    strcpy("myrPkt0", thisMYRI.name);

    if (io != 16) {
	max_boards = io;
	PRINTF(0) ("Myrinet: will attach a maximum of %d boards (io=0x%x)\n",
	    max_boards, max_boards);
    }

    if ((rv = myri_probe(&thisMYRI)) != 0) {
	if (myri_boards_found && myri_mem_problem) {
	    PRINTF(0)("** Myrinet driver had problems allocating memory (1)\n");
	    PRINTF(0)("** No boards will be useable - %d board(s) were found\n",
		myri_boards_found);
	} else {
	    PRINTF(0)("\n** No Myrinet boards found\n");
	}

	#ifdef DO_TIMEOUT_PROTOCOL
	    remove_rtscts_timeout();
	#endif /* DO_TIMEOUT_PROTOCOL */
	p3unregister_dev(p3slot);

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
            remove_proc_entry("rtscts", proc_cplant);
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
	    proc_unregister(proc_cplant, proc_rtscts.low_ino);
#endif
	#endif /* USE_PROC_FS */

	pkt_free_rcv_pages();
	pkt_free_snd_pages();

	kfree(hstshmem);

	return -EIO;
    }

    if (mlanai_count == 0) {
	if (myri_boards_found && myri_mem_problem) {
	    PRINTF(0)("** Myrinet driver had problems allocating memory (2)\n");
	    PRINTF(0)("** No boards will be useable - %d board(s) were found\n",
		myri_boards_found);
	} else {
	    PRINTF(0) ("\n** No Myrinet boards found (mlanai_count==0)\n");
	}

	#ifdef DO_TIMEOUT_PROTOCOL
	    remove_rtscts_timeout();
	#endif /* DO_TIMEOUT_PROTOCOL */
	p3unregister_dev(p3slot);

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
            remove_proc_entry("rtscts", proc_cplant);
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
	    proc_unregister(proc_cplant, proc_rtscts.low_ino);
#endif
	#endif /* USE_PROC_FS */

	pkt_free_rcv_pages();
	pkt_free_snd_pages();

	kfree(hstshmem);

	return -EIO;
    }

    if (myri_mem_problem) {
	PRINTF(0) ("\n** Myrinet driver had problems allocating memory\n");
	PRINTF(0) ("** Some boards may not appear to be functional\n");
    } else   {
	PRINTF(0) ("RTS-CTS init_module returns OK  board_count = %d\n",
	    mlanai_count);
    }


    index = 0;
    for (dev = &thisMYRI; dev; dev = dev->next) {
	if (dev->name) {
	    mp = dev->priv;
	    PRINTF(0) ("%d:  unit = %d  %s %s\n",
		index, mp->unit, dev->name, mp->adapter_name);
	    index++;
	}
    }
    return 0;
}

void stop_myri(struct NETDEV *dev) 
{
    struct NETDEV *devnext;
    struct myri_private *lp;
    int keep_loaded=0;

    for (; dev; dev = dev->next) {
	if (!strncmp(dev->name, "myrPtl", 6)) {
	    if (dev->flags & IFF_UP) {
		PRINTF(0) ("%s: device busy, remove delayed\n", dev->name);
		keep_loaded++;
	    }
	}
    }

    if (keep_loaded) {
	PRINTF(0) ("Myri devices busy, remove delayed\n");
    } else {
	for (dev = &thisMYRI; dev;) {
	    devnext = dev->next;
	    if (!strncmp(dev->name, "myrPtl", 6)) {
		lp = dev->priv;

		if (lp) {
		    PRINTF(0)("%s: Freeing Myrinet unit = %d  IRQ = %d  "
			" myri_private = %p\n", dev->name, lp->unit, dev->irq,
			lp->free0);
		    #ifdef intel_linux
			if (BOARD[lp->unit]) {
			    iounmap(BOARD[lp->unit]);
			}
		    #endif

		    free_irq(dev->irq, dev);
		    PRINTF(0) ("Freeing IRQ %d for dev %s\n", dev->irq,
			dev->name);
		    kfree(lp->free0);
		    dev->priv = NULL;
		    mlanai_uninit(lp);
		}

		unregister_netdev(dev);
		if (dev != &thisMYRI) {
		    PRINTF(0) ("Myrinet Freeing device (%p)\n", dev);
		    kfree(dev);
		}
	    }			/* myri dev */
	    dev = devnext;
	}			/* for */
    }				/* else */
}
