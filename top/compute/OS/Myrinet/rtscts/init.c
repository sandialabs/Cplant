/* init.c */
/* $Id: init.c,v 1.12.4.1 2002/05/22 21:40:03 jbogden Exp $ */

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

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/byteorder.h>		/* defines htons() etc. */

#include "lanai_device.h"
#include "MyrinetPCI.h"
#include "printf.h"

/* #define PRINTF(x) printk */

#define PCI_OFFSET_LCA		(IDENT_ADDR + 0x00300000000UL)
#define PCI_OFFSET_CIA		(IDENT_ADDR + 0x08600000000UL)
#define PCI_OFFSET_PYXIS	(IDENT_ADDR + 0x08800000000UL)
#define PCI_OFFSET_MCPCIA	(IDENT_ADDR + 0x0f900000000UL)
#define PCI_OFFSET_TSUNAMI	(IDENT_ADDR + 0x10000000000UL)

#define TRY_CHIPSET(PCI_OFFSET)     \
{                                   \
struct MYRINET_EEPROM *eeprom;      \
unsigned long eeprom_base;          \
struct MYRINET_BOARD *board= NULL;  \
                                    \
                                    \
    if (revision == 1)   {                                              \
        eeprom_base= base + (512UL * 1024UL);                           \
        eeprom= (struct MYRINET_EEPROM *)(eeprom_base | PCI_OFFSET);    \
    } else if ((revision == 2) || (revision == 3))   {                  \
        board= (struct MYRINET_BOARD *)(base | PCI_OFFSET);             \
        eeprom= (struct MYRINET_EEPROM *) ((char *)board + 0x00A00000); \
    } else   {                                                          \
        board= (struct MYRINET_BOARD *)(base | PCI_OFFSET);             \
        eeprom= (struct MYRINET_EEPROM *) &(board->lanai_eeprom[0]);    \
    }                                                                   \
    PRINTF(0) ("find_offset() Trying board %p, eeprom %p\n"             \
    "              offset 0x%0lx\n", board, eeprom, PCI_OFFSET);        \
    if (match_board_id((char *)&(eeprom->lanai_board_id[0])))   {       \
        printk("%s:%d Using Alpha %s (0x%08lx)\n", __FILE__, __LINE__,  \
                #PCI_OFFSET, PCI_OFFSET);                               \
        return (PCI_OFFSET);                                            \
    }                                                                   \
}


/* [x] [x] [x] [x] [x] [x] [x] [x] [x] [] [x] [x] [x] [x] [x] [x] [x] [x] [x] */

static unsigned long
find_offset(unsigned long base, unsigned char revision)
{



    PRINTF(0) ("find_offset(base 0x%016lx, rev %d)\n", base, revision);

    if ((revision != 0) && (revision != 1) && (revision != 2) &&
	    (revision != 3))   {
	printk("find_offset() Can't handle rev %d board\n", revision);
	return 0;
    }

    #ifdef intel_linux
	return 0;
    #elif  dec_linux
	/*
	** For the DEC Alpha chipsets we try different offsets until
	** we find one that works.
	*/
	TRY_CHIPSET(PCI_OFFSET_PYXIS);
	TRY_CHIPSET(PCI_OFFSET_TSUNAMI);
	TRY_CHIPSET(PCI_OFFSET_LCA);
	TRY_CHIPSET(PCI_OFFSET_CIA);
	TRY_CHIPSET(PCI_OFFSET_MCPCIA);

	printk("find_offset() Unknown PCI chipset\n");
	return 0L;
    #else
	#warning "find_offset() Unsupported Architecture"
    #endif

}  /* end of find_offset() */

/* [x] [x] [x] [x] [x] [x] [x] [x] [x] [] [x] [x] [x] [x] [x] [x] [x] [x] [x] */

struct MYRINET_BOARD *
myrinet_init_pointers_linux(int unit, unsigned int board_base,
    unsigned char revision)
{

struct MYRINET_BOARD *board;
struct MYRINET_EEPROM *eeprom;
unsigned int board_size = 1 * 1024 * 1024;
unsigned long offset;
unsigned long eeprom_base;
unsigned long board_base2 = (unsigned long) board_base;


    PRINTF(0) ("myrinet_init_pointers_linux(0x%x) unit=%d  rev=%d\n",
       board_base, unit, revision);


    offset= find_offset(board_base, revision);
    PRINTF(0) ("myrinet_init_pointers_linux() offset is 0x%016lx\n", offset);
    #if dec_linux
	if (offset == 0)   {
	    printk("myrinet_init_pointers_linux() invalid offset 0x0L\n");
	    return NULL;
	}
    #endif

    /*
    ** Linux requires high addresses to be re-mapped to lower ones
    ** But we need to find out how big the board is first, map only
    ** the EEPROM for now.
    */
    if (revision == 1) {
	eeprom_base= (unsigned long) board_base + (unsigned long) (512 * 1024);
	#ifdef intel_linux
	    eeprom= (struct MYRINET_EEPROM *)ioremap(eeprom_base, 4096UL);
	#elif dec_linux
	    eeprom= (struct MYRINET_EEPROM *)(eeprom_base | offset);
	#else
	    eeprom= (struct MYRINET_EEPROM *)(NULL);
	#endif

	if (eeprom == NULL) {
	    PRINTF(0) ("**** ioremap of eeprom space failed! ************* \n");
	    return NULL;
	}

	if (ntohs(eeprom->board_type) == MYRINET_BOARDTYPE_1MEG_SRAM) {
	    board_size = 2 * 1024 * 1024;
	    PRINTF(0) ("myri_init_ptrs_linux[%d]: this is a 1Meg SRAM board\n",
	       unit);
	} else {
	    PRINTF(0) ("myri_init_ptrs_linux[%d]: this board has normal "
		"layout\n", unit);
	}

	#ifdef intel_linux
	    iounmap(eeprom);
	#endif

	PRINTF(0) ("board_base = 0x%lx\neeprombase = 0x%lx\n",
	   (unsigned long) board_base2, (unsigned long) eeprom_base);
    }

    if ((revision == 3) || (revision == 2) || (revision == 0)) {
	board_size = (16 * 1024 * 1024);
    }


    #ifdef intel_linux
	board= (struct MYRINET_BOARD *)ioremap(board_base2, board_size);
    #elif dec_linux
	board= (struct MYRINET_BOARD *)(board_base2 | offset);
    #else
	board= (struct MYRINET_BOARD *)(NULL);
    #endif
    PRINTF(0) ("myri[%d]: DEC Alpha using 0x%lx as pci_offset\n", unit, offset);
    PRINTF(0) ("myri[%d]: board is %p\n", unit, board);
    PRINTF(0) ("myri[%d]: NEW k. virtual addr  (%p)  size = 0x%x\n", 
	unit, board, board_size);
    BOARD[unit] = (volatile unsigned short *) board;

    if (board == NULL) {
	PRINTF(0) ("**** ioremap failed! ************* \n");
	return NULL;
    }

    /* set the pointers, delay line and lanai clockval */
    PRINTF(0)("myri[%d]: calling myrinet_init_pointers\n",unit);
    myrinet_init_pointers(unit, board, revision);
    PRINTF(0) ("myri[%d]: returned from myrinet_init_pointers\n",unit);


    PRINTF(0) ("LANAI_EEPROM[%d]  = %p\n", unit, LANAI_EEPROM[unit]);
    PRINTF(0) ("LANAI_SPECIAL[%d] = %p\n", unit, LANAI_SPECIAL[unit]);
    PRINTF(0) ("LANAI_CONTROL[%d] = %p\n", unit, LANAI_CONTROL[unit]);
    PRINTF(0) ("LANAI3[%d]        = %p\n", unit, LANAI3[unit]);

    if (!LANAI_EEPROM[unit]) {
	printk("myri[%d]: ***** EEPROM is NULL???\n", unit);
    }

    PRINTF(0) ("\tclockval = %08x\n",
	(unsigned int) ntohl(LANAI_EEPROM[unit]->lanai_clockval));
    PRINTF(0) ("\tcpu      = %04x\n",
	(unsigned short) ntohs(LANAI_EEPROM[unit]->lanai_cpu_version));
    PRINTF(0) ("\tid       = %02x:%02x:%02x:%02x:%02x:%02x\n",
	LANAI_EEPROM[unit]->lanai_board_id[0],
	LANAI_EEPROM[unit]->lanai_board_id[1],
	LANAI_EEPROM[unit]->lanai_board_id[2],
	LANAI_EEPROM[unit]->lanai_board_id[3],
	LANAI_EEPROM[unit]->lanai_board_id[4],
	LANAI_EEPROM[unit]->lanai_board_id[5]);
    PRINTF(0) ("\tsram     = %ld Bytes  (%d)\n",
	ntohl(LANAI_EEPROM[unit]->lanai_sram_size),
	(int) lanai_memory_size(unit));
    PRINTF(0) ("\tdelay    = 0x%04x\n",
	ntohs(LANAI_EEPROM[unit]->delay_line_value));
    PRINTF(0) ("\tboardtype= 0x%04x\n", ntohs(LANAI_EEPROM[unit]->board_type));
    PRINTF(0) ("\tbus_type = 0x%04x\n", ntohs(LANAI_EEPROM[unit]->bus_type));
    PRINTF(0) ("\tserial no= %ld\n", ntohl(LANAI_EEPROM[unit]->serial_number));
    PRINTF(0) ("\tfpga vers= %s\n", LANAI_EEPROM[unit]->fpga_version);
    PRINTF(0) ("\tmore vers= %s\n", LANAI_EEPROM[unit]->more_version);
    PRINTF(0) ("\tboard label = %s\n", LANAI_EEPROM[unit]->board_label);
    PRINTF(0) ("\tproduct code= 0x%x\n", LANAI_EEPROM[unit]->product_code);
    PRINTF(0) ("myrinet_init_pointers_linux(unit=%d) returning\n", unit);

    return (board);

}  /* end of myrinet_init_pointers_linux() */

/* [x] [x] [x] [x] [x] [x] [x] [x] [x] [] [x] [x] [x] [x] [x] [x] [x] [x] [x] */
