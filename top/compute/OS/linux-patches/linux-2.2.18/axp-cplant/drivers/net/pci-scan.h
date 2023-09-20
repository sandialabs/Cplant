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
#ifndef _PCI_SCAN_H
#define _PCI_SCAN_H
/*
  version 1.02 $Version:$ $Date: 2001/12/11 19:32:38 $
   Copyright 1999-2001 Donald Becker / Scyld Computing Corporation
   This software is part of the Linux kernel.  It may be used and
   distributed according to the terms of the GNU Public License,
   incorporated herein by reference.
*/

/*
  These are the structures in the table that drives the PCI probe routines.
  Note the matching code uses a bitmask: more specific table entries should
  be placed before "catch-all" entries.

  The table must be zero terminated.
*/
enum pci_id_flags_bits {
	/* Set PCI command register bits before calling probe1(). */
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	/* Read and map the single following PCI BAR. */
	PCI_ADDR0=0<<4, PCI_ADDR1=1<<4, PCI_ADDR2=2<<4, PCI_ADDR3=3<<4,
	PCI_ADDR_64BITS=0x100, PCI_NO_ACPI_WAKE=0x200, PCI_NO_MIN_LATENCY=0x400,
	PCI_UNUSED_IRQ=0x800,
};

struct pci_id_info {
	const char *name;
	struct match_info {
		int	pci, pci_mask, subsystem, subsystem_mask;
		int revision, revision_mask; 				/* Only 8 bits. */
	} id;
	enum pci_id_flags_bits pci_flags;
	int io_size;				/* Needed for I/O region check or ioremap(). */
	int drv_flags;				/* Driver use, intended as capability flags. */
};

enum drv_id_flags {
	PCI_HOTSWAP=1, /* Leave module loaded for Cardbus-like chips. */
};
enum drv_pwr_action {
	DRV_NOOP,			/* No action. */
	DRV_ATTACH,			/* The driver may expect power ops. */
	DRV_SUSPEND,		/* Machine suspending, next event RESUME or DETACH. */
	DRV_RESUME,			/* Resume from previous SUSPEND  */
	DRV_DETACH,			/* Card will-be/is gone. Valid from SUSPEND! */
	DRV_PWR_WakeOn,		/* Put device in e.g. Wake-On-LAN mode. */
	DRV_PWR_DOWN,		/* Go to lowest power mode. */
	DRV_PWR_UP,			/* Go to normal power mode. */
};

struct drv_id_info {
	const char *name;			/* Single-word driver name. */
	int flags;
	int pci_class;				/* Typically PCI_CLASS_NETWORK_ETHERNET<<8. */
	struct pci_id_info *pci_dev_tbl;
	void *(*probe1)(struct pci_dev *pdev, void *dev_ptr,
					long ioaddr, int irq, int table_idx, int fnd_cnt);
	/* Optional, called for suspend, resume and detach. */
	int (*pwr_event)(void *dev, int event);
	/* Internal values. */
	struct drv_id_info *next;
	void *cb_ops;
};

/*  PCI scan and activate.
	Scan PCI-like hardware, calling probe1(..,dev,..) on devices that match.
	Returns -ENODEV, a negative number, if no cards are found. */

extern int pci_drv_register(struct drv_id_info *drv_id, void *initial_device);
extern void pci_drv_unregister(struct drv_id_info *drv_id);


/*  ACPI routines.
	Wake (change to ACPI D0 state) or set the ACPI power level of a sleeping
	ACPI device.  Returns the old power state.  */

int acpi_wake(struct pci_dev *pdev);
enum  acpi_pwr_state {ACPI_D0, ACPI_D1, ACPI_D2, ACPI_D3};
int acpi_set_pwr_state(struct pci_dev *pdev, enum acpi_pwr_state state);


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
#endif
