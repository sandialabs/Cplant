/*
** myriPut.c
** a subset of myriInterface.c for reasons I'd rather not go into...
*/

/*************************************************************************
 *                                                                       *
 * Myricom Myrinet Software                                              *
 *                                                                       *
 * Copyright (c) 1994, 1995, 1996 by Myricom, Inc.                       *
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
 * Arcadia, CA 91006                                                     *
 * 818 821-5555                                                          *
 * http://www.myri.com                                                   *
 *************************************************************************/

#ifdef dec_osf1
    #include <sys/machine/endian.h>
#endif
#ifndef __KERNEL__
    #include <stdio.h>
    #define PRINTF(fmt, args...)	fprintf(stderr, fmt, ## args)
#else
    #include <linux/kernel.h>
    #include <linux/string.h>		/* For memcpy() */
    #define PRINTF(fmt, args...)	printk(fmt, ## args) 
#endif /* __KERNEL__ */
#include "../util/arch_asm.h"		/* For mb() */
#include "lanai_device.h"
#include "MyrinetPCI.h"
#include "myriInterface.h"

#define ZEROS16	{0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0}

volatile unsigned short *BOARD[16] = ZEROS16;
volatile unsigned short *LANAI[16] = ZEROS16;	/* ushort pointer to base of
                                                   LANai memory */
volatile unsigned int *LANAI3[16] = ZEROS16;	/* uint pointer to base of
                                                   LANai memory */
struct LANAI_REG *LANAI_SPECIAL[16] = ZEROS16;	/* pointer to base of LANai
                                                   special regs */
struct LANAI7_REG *LANAI5_SPECIAL[16] = ZEROS16;/* pointer to base of LANai
                                                   special regs */
volatile unsigned short *LANAI_CONTROL[16] = ZEROS16;	/* pointer to board
                                                           control space */
struct MYRINET_EEPROM *LANAI_EEPROM[16] = ZEROS16;	/* pointer to EEPROM
                                                           info */
struct MYRINET_EEPROM *LANAI_EEPROM_PTR[16] = ZEROS16;  /* boardpointer to
                                                           EEPROM info */
struct MYRINET_EEPROM LANAI_EEPROM_COPY[16];	/* copy of EEPROM info */

volatile unsigned int *UBLOCK[16] = ZEROS16;	/* pio pointer to dma-block */
volatile unsigned int *DBLOCK[16] = ZEROS16;	/* dma pointer to dma-block
                                                   used by LANai */


volatile unsigned int *LANAI_BOARD[16] = ZEROS16;

unsigned int XXdriver_clockval[16] = ZEROS16;

typedef enum {false, true} boolean;


/******************************************************************************/
/*
** I can't figure out where this function gets called. It has to be defined
** here, though, for the MCP load to work. If you take it out, everything
** will compile and link, but it wont run!?!?!?
*/
void
usleep(unsigned usec)
{
    unsigned i;
    for (i = 0; i < usec; i++) {
    }
}

int
mydelay(int loop)
{
        int rv = 0, i;
        for (i = 0; i < loop; i++) {
                rv += i;
        }
        return (rv);
}

#ifdef MODULE

/******************************************************************************/

static void
set_clock_chip_delay(volatile unsigned int *p, int dly)
{
    int i;

    *p = 0;
    mb();                /* start with enable off, clock off */
    *p = 0x4;
    mb();                /* turn on enable */

    for (i = 8; i--;) {
        if ((1 << i) & dly) {
            *p = 0x5;
            mb();
            *p = 0x7;
            mb();
            *p = 0x5;
            mb();
        } else {
            *p = 0x4;
            mb();
            *p = 0x6;
            mb();
            *p = 0x4;
            mb();
        }
    }

    *p = 0;
    mb();                /* turn off enable */
}

/******************************************************************************/

/*
** myrinet_init_pointers()
**
** This routine should be called to initialize the data structures
** to allow the rest of the function to operate. These global
** data structures are also used by the myriSpi and myriApi
** libraries.
**
** It also sets the clock_value in the LANai chip.
**
** If you need to support the myriAPI library, you need to define
** UBLOCK[] and DBLOCK[]. These are pointers to a block of memory
** that can be used for DMA. UBLOCK is a CPU-access pointer and
** DBLOCK is a pointer used for DMA. On some architectures
** (in the kernel) these addresses are different.
*/
int
myrinet_init_pointers(int unit, void *board_base, unsigned char revision)
{

struct MYRINET_BOARD *board = (struct MYRINET_BOARD *) board_base;


    if ((unit < 0) || (unit >= 16)) {
	return (0);
    }

    if (!board_base) {
	return (0);
    }

    /* Clear prefetch and other bits */
    #if defined(dec_osf1) || defined(dec_linux)
	board = (struct MYRINET_BOARD *) ((unsigned long)board & ~(0xFFUL));
    #else
	board = (struct MYRINET_BOARD *) ((unsigned long)board & ~0xFF);
    #endif

    BOARD[unit] = (void *)board;
    LANAI_BOARD[unit] = (void *)board;
    LANAI3[unit] = board->lanai_memory;
    LANAI[unit] = (unsigned short *) board->lanai_memory;

    LANAI_EEPROM[unit] = (struct MYRINET_EEPROM *) &board->lanai_eeprom[0];
    LANAI_EEPROM_PTR[unit] = (struct MYRINET_EEPROM *) &board->lanai_eeprom[0];
    LANAI_SPECIAL[unit] = (struct LANAI_REG *) &board->lanai_registers[0];
    LANAI_CONTROL[unit] = &board->lanai_control[0];

    /*PRINTF("%s:%4d    revision is %d\n", __FILE__, __LINE__, revision);*/
    if (revision == 1)   {
	if (ntohs(LANAI_EEPROM[unit]->board_type) ==
		MYRINET_BOARDTYPE_1MEG_SRAM) {
	    /* this is the "newer" memory layout, LANai SRAM comes at the end */
	    LANAI3[unit] = (unsigned int *)
			((unsigned long)LANAI3[unit] + (unsigned long)(1024*1024));
	    LANAI[unit] = (unsigned short *) LANAI3[unit];
	}
    }

    if ((revision == 2) || (revision == 3)) {
	char *base = (char *) board_base;

	BOARD[unit] = (void *) base;
	LANAI_BOARD[unit] = (void *) base;
	LANAI3[unit] = (void *) (base + 0x00000000);
	LANAI[unit] = (void *) (base + 0x00000000);
	LANAI_EEPROM_PTR[unit] = (void *) (base + 0x00A00000);
	LANAI_SPECIAL[unit] = (void *) (base + 0x00804000);
	LANAI5_SPECIAL[unit] = (void *) (base + 0x00804000);
	LANAI_CONTROL[unit] = (void *) (base + 0x00800040);

	/*
	PRINTF("%s:%4d    LANAI_EEPROM_PTR is %p\n", __FILE__, __LINE__,
	    LANAI_EEPROM_PTR[unit]);
	*/

#ifdef USER_LIBRARY
#warning "Compiling with USER_LIBRARY defined!"
	LANAI_EEPROM[unit] = (void *) (&LANAI_EEPROM_COPY[unit]);
	if (match_board_id((char *)&(LANAI_EEPROM_PTR[unit]->lanai_board_id))) {
	    memcpy((char *) LANAI_EEPROM[unit], (char *) LANAI_EEPROM_PTR[unit],
		    sizeof(struct MYRINET_EEPROM));
	} else {
	    PRINTF("*** This is probably a board we're not supporting\n");
	}
#endif /* USER_LIBRARY */

#if !defined(USER_LIBRARY)
	/* function is a no-op if the board is not LANai5-based */
	lanai_breset_unit(unit, LANAI_OFF);

	/* give the LANAI a clock_val that keeps it from eating power */
	LANAI5_SPECIAL[unit]->CLOCK = htonl(0x80);
	mydelay(100);

	LANAI_EEPROM[unit] = (void *) (&LANAI_EEPROM_COPY[unit]);
	if (match_board_id((char *)&(LANAI_EEPROM_PTR[unit]->lanai_board_id))) {
	    memcpy((void *) LANAI_EEPROM[unit], (void *) LANAI_EEPROM_PTR[unit],
		    sizeof(struct MYRINET_EEPROM));
	} else {
	    PRINTF("*** This is probably a board we're not supporting\n");
	}
#endif	/* NOTOUCH */
    }
    /*
       Set some values once into the board at powerup.

       The delay_func_line_value is used to initialize a digital
       delay_func line on the PCI board. This gets the clock to the
       LANai working properly.

       The lanai_clockval is written into a special register in the
       LANai chip to set up the phase relationships of the internal
       LANai clocks.

       These should both be set before accessing the LANai itself, and
       should only be set one at power-up.
    */


    if (revision == 1) {
	unsigned int delay;

	delay = (unsigned int) ntohs(LANAI_EEPROM[unit]->delay_line_value);

	if (delay >= 0xFFFF) {
	    /*
	     * take care of boards that do NOT have the EEPROM
	     * info
	     */
	    delay = 0xE;
	}

	set_clock_chip_delay((volatile unsigned int *) &LANAI_CONTROL[unit][0],
	    delay);
    }


    lanai_ereset_unit(unit, LANAI_ON);
    /* set the LANai clock_val */
    /*PRINTF("%s:%4d    CPU Version 0x%08x\n", __FILE__, __LINE__,
	LANAI_EEPROM[unit]->lanai_cpu_version);*/
    if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
	    (unsigned short) 0x0400) {
	mb();
	/* eeprom and LANai are big endian */
	if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
		(unsigned short) 0x0500) {
	    unsigned int clockval;

	    clockval= ntohl(LANAI_EEPROM[unit]->lanai_clockval);
	    /*PRINTF("%s:%4d    Using eeprom clock val of 0x%08x\n", __FILE__,
		__LINE__, clockval);*/
	    clockval= clockval & (unsigned int)0xFFFFFF8F;
	    clockval= clockval | 0xA0; /* 2.0 X PCI */
	    /* PRINTF("*** FIX ME *** calculate correct clock value!!!\n"); */
	    /*PRINTF("%s:%4d    With multiplier           0x%08x\n", __FILE__,
		__LINE__, clockval);*/

	    LANAI5_SPECIAL[unit]->CLOCK= htonl(clockval);
	    XXdriver_clockval[unit]= clockval;
	} else {
	    LANAI_SPECIAL[unit]->clock_val = LANAI_EEPROM[unit]->lanai_clockval;
	    XXdriver_clockval[unit]=
		ntohl(LANAI_EEPROM[unit]->lanai_clockval);
/*
	    if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) ==
		    (unsigned short)0x0403) {
		LANAI_SPECIAL[unit]->clock_val = htonl(0x90469046);
		XXdriver_clockval[unit]=
		    ntohl(LANAI_EEPROM[unit]->lanai_clockval);
	    }
*/
	}
	mb();
    } else   {
	PRINTF("myrinet_init_pointers() NOT Setting LANai clock!!!\n");
    }
    lanai_ereset_unit(unit, LANAI_OFF);
    lanai_reset_unit(unit, LANAI_ON);

    return (1);

}

#endif /* MODULE */

/******************************************************************************/
/*
** lanai_put()
**
** Copy memory from the host to the LANai SRAM.
** Useful for writing generic memory access routines
** that do not depend on idiosyncarcies of the board.
** Assumes that the pointers are 4-byte aligned and the
** length is a multiple of 4.
*/
void
lanai_put(int unit, void *source, unsigned offset, unsigned len)
{

register unsigned int  *from, *to;
int i;

#if defined(dec_linux) || defined(intel_linux)
    if (((unsigned long)source & 0x03) || (offset & 0x03) || (len & 0x03))   {
	PRINTF("lanai_put(unit %d, src %p, offset %d, len %d)\n",
	    unit, source, offset, len);
	PRINTF("    called with unaligned source, or offset or len "
	    "not multiple of 4\n");
	return;
    }

    to = (unsigned int *) ((long)LANAI[unit] + offset);
    from = (unsigned int *)source;

    for (i= 0; i < (len / sizeof(int)); i++) {
	mb();
	*to++ = *from++;
	mb();
    }
#else
    to = (unsigned char *) &LANAI3[unit][offset / 4];
    from = (unsigned char *) source;

    if (!is_lanai_safe(unit)) {
	return;
    }

    mb();
    bcopy(from, to, length);
    mb();
#endif
}


/******************************************************************************/
void
lanai_reset_unit(int unit, int action)
{
    /*
     * Board control registers may need byte swapping  - they are
     * little endian.  E.g., many PowerPC boards are big endian, but
     * a little endian PCI bus.
     */

    if (LANAI_CONTROL[unit]) {
	switch (action) {

	    case LANAI_RESET_ON:
	    case LANAI_ON:
		lanai_interrupt_unit(unit, LANAI_OFF);
		mb();
		if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
			(unsigned short) 0x0500) {
		    *(unsigned int *) &LANAI_CONTROL[unit][0] = 
			(LANAI5_RESET_ON|LANAI5_ERESET_ON);
		    lanai_ereset_unit(unit, LANAI_OFF);
		} else {
		    *(unsigned int *) &LANAI_CONTROL[unit][0]= (LANAI_RESET_ON);
		}
		mb();
		return;

	    case LANAI_RESET_OFF:
	    case LANAI_OFF:
		mb();
		if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
			(unsigned short) 0x0500) {
		    lanai_ereset_unit(unit, LANAI_ON);
		    lanai_ereset_unit(unit, LANAI_OFF);
		    *(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_RESET_OFF;
		} else {
		    *(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI_RESET_OFF;
		}
		mb();
		if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
			(unsigned short) 0x0500) {
		    /*
		    LANAI5_SPECIAL[unit]->EIMR = 0;
		    */
		} else {
		    LANAI_SPECIAL[unit]->EIMR = 0;
		}
		mb();
		return;

	    default:
		/* bad operation */
		break;
	}
    } else {
	/* bad unit */
    }
}


/******************************************************************************/

void
lanai_ereset_unit(int unit, int action)
{
    /* control registers may need byte swapping, they are little endian. */

    if (LANAI_CONTROL[unit]) {
	switch (action) {
	    case LANAI5_ERESET_ON:
	    case LANAI_ON:
		mb();
		if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
			(unsigned short) 0x0500) {
		    *(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_ERESET_ON;
		} else {
		    /* no L4 equivalent */
		}
		mb();
		return;

	    case LANAI5_ERESET_OFF:
	    case LANAI_OFF:
		mb();
		if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
			(unsigned short) 0x0500) {
		    *(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_ERESET_OFF;
		    usleep(10000);
		} else {
		    /* no L4 equivalent */
		}
		mb();
		return;

	    default:
		/* bad operation */
		break;
	}
    } else {
	/* bad unit */
    }
}

void
lanai_breset_unit(int unit, int action)
{
    /* control registers may need byte swapping, they are little endian. */

    if (LANAI_CONTROL[unit]) {
	switch (action) {
	    case LANAI5_BRESET_ON:
	    case LANAI_ON:
		mb();
		*(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_BRESET_ON;
		mb();
		return;

	    case LANAI5_BRESET_OFF:
	    case LANAI_OFF:
		mb();
		*(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_BRESET_OFF;
		mydelay(100);
		mb();
		return;

	    default:
		/* bad operation */
		break;
	    }
    } else {
	/* bad unit */
    }
}


void
lanai_interrupt_unit(int unit, int action)
{
    /* control registers may need byte swapping, they are little endian. */

    if (LANAI_CONTROL[unit]) {
        switch (action) {
	    case LANAI_INT_ENABLE:
	    case LANAI_ON:
	    mb();
	    if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
		    (unsigned short) 0x0500) {
		*(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_INT_ENABLE;
	    } else   {
		*(unsigned int *)&LANAI_CONTROL[unit][0]= LANAI_INT_ENABLE;
	    }
	    mb();
	    return;

	case LANAI_INT_DISABLE:
	case LANAI_OFF:
	    mb();
	    if (ntohs(LANAI_EEPROM[unit]->lanai_cpu_version) >=
		    (unsigned short) 0x0500) {
		*(unsigned int *) &LANAI_CONTROL[unit][0]= LANAI5_INT_DISABLE;
	    } else {
		*(unsigned int*)&LANAI_CONTROL[unit][0]= LANAI_INT_DISABLE;
	    }
	    mb();
	    return;

         default:
	     /* bad operation */
	     break;
        }
    } else {
	/* bad unit */
    }
}


/******************************************************************************/
/*
** lanai_board_type()
**
** Returns the cpu version of the board.
*/
board_type_number
lanai_board_type(unsigned int unit)
{
    if (LANAI_EEPROM[unit]) {
	return ((board_type_number)
		    ntohs(LANAI_EEPROM[unit]->lanai_cpu_version));
    } else {
	return ((board_type_number) 0);
    }
}


/******************************************************************************/
/*
** lanai_get_board_id()
**
** Grabs the 48-bit UID from the EEPROM.
** Copies it to *addr.
** Returns true if id was found, false if not.
*/
int
lanai_get_board_id(int unit, void *addr)
{

    unsigned char *src, *dst;

    if (LANAI_EEPROM[unit]) {
	src = (unsigned char *)&LANAI_EEPROM[unit]->lanai_board_id[0];
	dst = (unsigned char *) addr;

	/* 6 bytes of ID */
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	return (1);
    } else {
	return (0);
    }
}

/******************************************************************************/
/*
** Is this a Myrinet card?
*/
int
match_board_id(unsigned char *id)
{
    #if defined(__KERNEL__) && defined(VERBOSE)
	PRINTF("match_board_id()\taddr = %p\n", id);
	PRINTF("match_board_id()\tid   = %02x:%02x:%02x:%02x:%02x:%02x\n",
	   id[0], id[1], id[2], id[3], id[4], id[5]);
    #endif
    if ((id[0] == 0x00) && (id[1] == 0x60) &&
            (id[2] == 0xdd) && (id[3] == 0x7f))   {
        return 1;
    } else if ((id[0] == 0x00) && (id[1] == 0x60) && (id[2] == 0xb0))   {
        return 1;
    } else   {
        return 0;
    }
}  /* end of match_board_id() */

/******************************************************************************/

unsigned int
lanai_memory_size(const unsigned int unit)
{
    if (!LANAI_EEPROM[unit]) {
	return (128 * 1024);
    }

    if (!LANAI_EEPROM[unit]->lanai_sram_size) {
	return (128 * 1024);
    }

    /* protect against EEPROM read fail */
    if (ntohl(LANAI_EEPROM[unit]->lanai_sram_size) & 0x3ff) {
	return (0);
    }

    return (ntohl(LANAI_EEPROM[unit]->lanai_sram_size));

}

/******************************************************************************/
void
lanai_write_word(int unit, unsigned offset, unsigned int i)
{

    /* LANAI3 is an array of full-words */
    mb();
    LANAI3[unit][offset / 4] = htonl(i);
    mb();

}

/******************************************************************************/
int
lanai_clear_memory(const unsigned int unit)
{

int i;
int size;


    size = lanai_memory_size(unit);

    for (i = 0; i < size / 4; i++)   {
	lanai_write_word(unit, i * 4, 0);
    }

    return true;
}
