/* MyrinetPCI.h */

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
 * 325B N. Santa Anita Ave.                                              *
 * Arcadia, CA 91024                                                     *
 * 818 821-5555                                                          *
 * http://www.myri.com                                                   *
 *************************************************************************/

/*************************************************************************
 The Myrinet PCI 1.1 board occupies 2 separate regions of memory if you
 count the PCI configuration space.
 The configuration space is accessed differently than the attached device.

 All of the non-configuration space is allocated as a contiguous block
 divided as follows. Note: The M2-PCI32-xxxxx have 256KBytes of SRAM memory.
 We've allocated 512KB in the address space to accomodate more in
 the future.
 The pointer to this region of space is found in the configuration
 space at BASE ADDRESS REGISTER #0.

 Byte offset   What                    Length
 -----------   -------------           ------
  0x00000000   LANai SRAM Memory       128*1024 Bytes (128KBytes)
  0x00020000   Padding (unused)        384*1024 Bytes (384KBytes)
  0x00080000   EEPROM                  256*1024 Bytes (256KBytes)
  0x000C0000   LANai3.x Registers      128*1024 Bytes (128KBytes)
  0x000E0000   Control Space (FPGA)    128*1024 Bytes (128KBytes)
  0x000E0040   Configuration Space                               
  0x00100000   End of Board Memory                     total = 1MByte


 L4.x - 512KBytes of SRAM (generally only 256k populated)
 Byte offset   What                    Length
 -----------   -------------           ------
  0x00000000   LANai SRAM Memory       512*1024 Bytes (512KBytes)
  0x00080000   EEPROM                  256*1024 Bytes (256KBytes)
  0x000C0000   LANai3.x Registers      128*1024 Bytes (128KBytes)
  0x000E0000   Control Space (FPGA)    128*1024 Bytes (128KBytes)
  0x000E0040   Configuration Space                                
  0x00100000   End of Board Memory                     total = 1MByte



 L4.x - 1024KBytes of SRAM
 EEPROM->board_type will be MYRINET_BOARDTYPE_1MEG
 Byte offset   What                    Length
 -----------   -------------           ------
  0x00000000   unused                  512*1024 Bytes (512KBytes)
  0x00080000   EEPROM                  256*1024 Bytes (256KBytes)
  0x000C0000   LANai3.x Registers      128*1024 Bytes (128KBytes)
  0x000E0000   Control Space (FPGA)    128*1024 Bytes (128KBytes)
  0x000E0040   Configuration Space                               
  0x00100000   LANai SRAM             1024*1024 Bytes (1MByte)
  0x00200000   End of Board Memory                    total = 2MByte


 L5.x new-style boards (PCI config Revision == 2)
Total = 16Meg
Byte offset from begining of the 16-meg space:

0x00000000 - memory space, 8-meg
0x00800000 - configuration register, 16 words
0x00800040 - control register, 1 word
0x00800100 - bank of pointers, 16 words
0x00804000 - lanai special register, 16K bytes
0x00880000 - eeprom, 4-meg
0x00A00000 - eeprom, 4-meg (this works on actel and quicklogic boards)
0x00808000 - doorbell region  - 8meg minus 32K. 4K bytes per door bell.
0x01000000 - end 

*************************************************************************/

#define MYRINET_PCI_VENDOR_ID	(0x10e8)
#define MYRINET_PCI_VENDOR_ID_2	(0x14c1)
#define MYRINET_PCI_DEVICE_ID	(0x8043)

struct MYRINET_BOARD {
        unsigned int lanai_memory[(512*1024)/sizeof(int)];
        unsigned int lanai_eeprom[(256*1024)/sizeof(int)];
        volatile unsigned int lanai_registers[(128*1024)/sizeof(int)];
        volatile unsigned short lanai_control[(128*1024)/sizeof(short)];
        unsigned int lanai_memory2[(1024*1024)/sizeof(int)];
};


/* defines for the control register (unsigned int *) LANAI_CONTROL[0] */
#define BIT(n)          ((unsigned int) 1 << n)

#if 0 && (defined(sparc_solaris) || defined(sparc_special))
#define LANAI_RESET_OFF    0x00000080
#define LANAI_RESET_ON     0x00000040
#define LANAI_INT_ENABLE   0x00000020
#define LANAI_INT_DISABLE  0x00000010
#define LANAI_WAKE_ON      0x00000000
#define FPGA_READ_BURST	   0
#else
#define LANAI_RESET_OFF    BIT(31)
#define LANAI_RESET_ON     BIT(30)
#define LANAI_INT_ENABLE   BIT(29)
#define LANAI_INT_DISABLE  BIT(28)
#define LANAI_WAKE_ON      BIT(27)
#define SERIAL3		   BIT(3)
#define FPGA_READ_BURST	   BIT(1)
#define SERIAL0		   BIT(0)
#endif

/* 
    LANai 5-based boards

    31   remove lanai reset                     | CHANGED
    30   assert lanai reset                     | CHANGED
    29   remove lanai ebus reset                | CHANGED
    28   assert lanai ebus reset                | CHANGED
    27   remove board reset                     | CHANGED
    26   assert board reset                     | CHANGED
    25   turn on interrupt enable
    24   turn off interrupt enable

    23   dma trigger 3  ( write only )
    22   dma trigger 2  ( write only )
    21   dma trigger 1  ( write only )
    20   dma trigger 0  ( write only )
    19
    18
    17
    16

    15
    14   read-only correct RST/REQ64 sequence detected
    13   micro-controller 2 serial data in, read only
    12   micro-controller 1 serial data in, read only
    11   micro-controller 2 serial data out
    10   micro-controller 1 serial data out
    09   micro-controller serial data clock (shared)
    08

    07   force 64 bit mode
    06   force 32 bit mode
    05
    04
    03   enable multi-queue interrupt on write
    02   queue size select 2
    01   queue size select 1
    00   queue size select 0

*/
#define LANAI5_RESET_OFF	BIT(31)
#define LANAI5_RESET_ON		BIT(30)
#define LANAI5_ERESET_OFF	BIT(29)
#define LANAI5_ERESET_ON	BIT(28)

#define LANAI5_BRESET_OFF	BIT(27)
#define LANAI5_BRESET_ON	BIT(26)
#define LANAI5_INT_ENABLE	BIT(25)
#define LANAI5_INT_DISABLE	BIT(24)

#define LANAI5_DMA_3		BIT(23)
#define LANAI5_DMA_2		BIT(22)
#define LANAI5_DMA_1		BIT(21)
#define LANAI5_DMA_0		BIT(20)


#define LANAI5_64BIT_RST_OK BIT(14)

#define LANAI5_UC2_IN       BIT(13)
#define LANAI5_UC1_IN       BIT(12)
#define LANAI5_UC2_OUT      BIT(11)
#define LANAI5_UC1_OUT      BIT(10)
#define LANAI5_UC_CLOCK     BIT(9)

#define LANAI5_FORCE_64BIT_MODE BIT(7)
#define LANAI5_FORCE_32BIT_MODE BIT(6)

#define LANAI5_WRITE_INT_ENABLE BIT(3)
#define LANAI5_QUEUE_SIZE_2 BIT(2)
#define LANAI5_QUEUE_SIZE_1 BIT(1)
#define LANAI5_QUEUE_SIZE_0 BIT(0)


/*
(in the control register)
bit 4: 1 if LANai's LED is on
bit 5: 1 if green LED of tricolor LED is on
bit 6: 1 if red LED of tricolor LED is on
*/

/*
   A write of a 1 into a particular bit activates the  operation. Several bits can
   be written at one time.  
  
   to hold the LANai in reset:  lanai_control = LANAI_RESET_ON;
   to release the LANai reset:  lanai_control = LANAI_RESET_OFF;
  
   to enable host interrupts from the LANai:  lanai_control = LANAI_INT_ENABLE;
   [and you need to set bits in the LANAI_SPECIAL[unit].EIMR]
   To see if a particular LANai asserted interrupt, you must look at 
   	LANAI_SPECIAL[unit].ISR

   to disable host interrupts from the LANai: lanai_control = LANAI_INT_DISABLE;
  
   to turn on the wake pin on the LANai:  lanai_control = LANAI_WAKE_ON;
  
   The value read for each set/reset pair is the last effective write
   value.  For example, if the previous write to (LANAI_RESET_OFF,LANAI_RESET_ON)
   is (0,1), then a read will return (0,1).  The illegal write value of (1,1)
   will produce a result that is undefined.

 */


struct PCIconfig_space {
	unsigned short vendor;
	unsigned short device;

	unsigned short command;
	unsigned short status;

	unsigned char rev_id;
	unsigned char class_code[3];

	unsigned char cache_line_size;
	unsigned char latency_timer;
	unsigned char header_type;
	unsigned char bist;

	unsigned int base_address[6];

	unsigned int reserved[5];

#ifdef __alpha
  /*
   * Digital Unix has various #define's that conflict
   * with the following structure members.
   */
    unsigned char myri_int_line;
    unsigned char myri_int_pin;
    unsigned char myri_min_gnt;
    unsigned char myri_max_lat;
#else
    unsigned char int_line;
    unsigned char int_pin;
    unsigned char min_gnt;
    unsigned char max_lat;
#endif
};
	
/* end of MyrinetPCI.h */
