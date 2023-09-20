/* LinuxDevice.c */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#if __SLACKWARE
  #include <linux/time.h>
#endif
#include <sys/mman.h>
#include <asm/page.h>

#include "lanai_device.h"
#include "myriInterface.h"

#define MAP_DEVICE	1
#define LANAI_DEVICE_SIZE ((3*PAGE_SIZE) + BINFO[u_num].lanai_memory_size)

/* this is how the Linux kernel implementation maps the board */
/* Note: the LANai memory might be 512kB or more - don't use fixed pointers */
struct MYRINET_BOARD_MAPPED {
	volatile unsigned int lanai_eeprom[PAGE_SIZE / sizeof(int)];
	volatile unsigned short lanai_control[PAGE_SIZE / sizeof(short)];
	volatile unsigned int lanai_registers[PAGE_SIZE / sizeof(int)];
	volatile unsigned int lanai_memory[(256 * 1024) / sizeof(int)];
	unsigned int copy_block[0];
};


unsigned int LANAI_fd[16] =
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int FD[16] =
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

struct board_info BINFO[16];


static fd_set mosmask;
static int moswidth;
static int u_num;

/* copy the first lanai's eeprom as indicated */
int
get_lanai_eeprom_linux(struct MYRINET_EEPROM* prom, int unit)
{
	int rv, fd;
	char buf[30];
	char host_name[80];
	struct MYRINET_EEPROM *EEPROM = NULL;
	struct MYRINET_BOARD_MAPPED *mb_map = NULL;
	int map_len_bytes = 0;

	FD_ZERO(&mosmask);
	moswidth = 1;

	sprintf(buf, "/dev/mlanai%x", unit);
	if ((fd = open(buf, O_RDONLY)) < 0) {
	  gethostname(host_name, 80);
	  fprintf(stderr, "get_lanai_eeprom: Can't open lanai device %d on %s "
	      "for reading????\n", unit, host_name);
	  return 0;
	}

	FD_SET(fd, &mosmask);
	if (fd >= moswidth) {
	  moswidth = fd + 1;
        }

	bzero(&BINFO[unit], sizeof(struct board_info));

	if ((rv = ioctl(fd, MLANAI_GET_INFO, &BINFO[unit])) < 0) {
	  perror("ioctl failed: ");
	  return 0;
	}

	if (!BINFO[unit].lanai_memory_size) {
	    BINFO[unit].lanai_memory_size = 256 * 1024;
	}

	LANAI_fd[unit] = fd;
	map_len_bytes = LANAI_DEVICE_SIZE;

	if (((BINFO[unit].lanai_control & 0xF) == 2) ||
	       ((BINFO[unit].lanai_control & 0xF) == 3)) {
	    /*
	    ** this is an L5 (revision==2) board
	    ** or an L7 (revision == 2|3) board
	    */
	    map_len_bytes = 16 * 1024 * 1024;
	}

	mb_map = (struct MYRINET_BOARD_MAPPED *) mmap(NULL, map_len_bytes,
	      PROT_READ, MAP_SHARED, LANAI_fd[unit], (off_t) 0);

	if (mb_map == MAP_FAILED) {
	  perror("mmap ");
	  printf("failed to mmap() Myrinet board space\n");
	  return 0;
	}

	printf("%s:%d   BINFO[unit].lanai_control is 0x%08x\n", __FILE__,
	    __LINE__, (unsigned int)BINFO[unit].lanai_control);
	if (((BINFO[unit].lanai_control & 0xF) == 2) ||
		((BINFO[unit].lanai_control & 0xF) == 3)) {
	    /*
	    ** this is an L5 (revision==2) board
	    ** or an L7 (revision == 2|3) board
	    */
	    char *base = (char *) mb_map;
	    EEPROM = (struct MYRINET_EEPROM *) (base + 0x00A00000);

	    memcpy(prom, EEPROM, sizeof(struct MYRINET_EEPROM));
	    LANAI_EEPROM[unit] = (struct MYRINET_EEPROM *) prom;

	    if (match_board_id((char *)&(EEPROM->lanai_board_id)))   {
		/* OK, no more copy */
	    } else {
		/* funny quicklogic board */
		printf("%s: %s() We don't support funny quicklogic boards\n",
		    __FILE__, "get_lanai_eeprom_linux");
	    }
	} else {
	    EEPROM = (struct MYRINET_EEPROM *) mb_map->lanai_eeprom;

	    printf("%s:%d   CPU Version 0x%08x\n", __FILE__, __LINE__,
		EEPROM->lanai_cpu_version);
	    if ((ntohs(EEPROM->lanai_cpu_version) >= (short)0x0200) &&
		    (ntohs(EEPROM->lanai_cpu_version) <= (short)0x08ff)) {
		    /* OK */
	    } else {
		printf("LANAI_EEPROM[%d] looks bad??\n", unit);
		printf("EEPROM values = \n");
		printf("\tclockval = %08lx\n", ntohl(EEPROM->lanai_clockval));
		printf("\tcpu      = %04x\n", ntohs(EEPROM->lanai_cpu_version));
		printf("\tid       = %02x:%02x:%02x:%02x:%02x:%02x\n",
		   EEPROM->lanai_board_id[0], EEPROM->lanai_board_id[1],
		   EEPROM->lanai_board_id[2], EEPROM->lanai_board_id[3],
		   EEPROM->lanai_board_id[4], EEPROM->lanai_board_id[5]);
		printf("\tsram     = %ld Bytes  (%d)\n",
		   ntohl(EEPROM->lanai_sram_size),
		   (int) lanai_memory_size(unit));
		printf("\tdelay    = 0x%04x\n",ntohs(EEPROM->delay_line_value));
		printf("\tboardtype= 0x%04x\n", ntohs(EEPROM->board_type));
		printf("\tbus_type = 0x%04x\n", ntohs(EEPROM->bus_type));

		EEPROM->lanai_cpu_version = htons(0x0400);
		EEPROM->lanai_sram_size = htonl(128 * 1024);
		EEPROM->lanai_clockval = htonl(0x11371137);
		EEPROM->lanai_board_id[0] = 0;
		EEPROM->lanai_board_id[1] = 0x60;
		EEPROM->lanai_board_id[2] = 0xDD;
		EEPROM->lanai_board_id[3] = 0x80;
		EEPROM->lanai_board_id[4] = 0;
		EEPROM->lanai_board_id[5] = 0;
		strcpy(EEPROM->fpga_version, "PCI Test Fixture");
	    }
	}


	if (!EEPROM) {
	  gethostname(host_name, 80);
	  fprintf(stderr, "get_lanai_eeprom_linux() Can't open lanai device "
	      "on %s.\n", host_name);
	  return 0;
	}

        bcopy(EEPROM, prom, sizeof(struct MYRINET_EEPROM));
	return 1;
}


/*************************************************

	void	-no parameters.

	returns 0 on error.
**************************************************/
int
open_lanai_device_linux(void)
{
	int rv, fd, i;
	char buf[30];
	char host_name[80];
	struct MYRINET_EEPROM *EEPROM = NULL;
/*
   struct MYRINET_BOARD *board = NULL;
 */
	struct MYRINET_BOARD_MAPPED *mb_map = NULL;
	int map_len_bytes = 0;

	FD_ZERO(&mosmask);
	moswidth = 1;
	u_num = 0;

	for (i = 0; i < 16; i++) {
		sprintf(buf, "/dev/mlanai%x", i);
		if ((fd = open(buf, O_RDWR)) < 0) {
			continue;
		}

		FD_SET(fd, &mosmask);
		if (fd >= moswidth)
			moswidth = fd + 1;

		bzero(&BINFO[u_num], sizeof(struct board_info));

		if ((rv = ioctl(fd, MLANAI_GET_INFO, &BINFO[u_num])) < 0) {
			perror("ioctl failed: ");
			return (u_num);
		}

		if (!BINFO[u_num].lanai_memory_size) {
			BINFO[u_num].lanai_memory_size = 256 * 1024;
		}

		LANAI_fd[u_num] = fd;

		map_len_bytes = LANAI_DEVICE_SIZE;

                if (((BINFO[u_num].lanai_control & 0xF) == 2) ||
			((BINFO[u_num].lanai_control & 0xF) == 3)) {
		    /*
		    ** this is an L5 (revision==2) board
		    ** or an L7 (revision == 2|3) board
		    */
		    map_len_bytes = 16 * 1024 * 1024;
                }

		mb_map = (struct MYRINET_BOARD_MAPPED *)
			mmap(NULL, map_len_bytes,
				 PROT_READ | PROT_WRITE,
				 MAP_SHARED,
				 LANAI_fd[u_num],
				 (off_t) 0);

		if (mb_map == MAP_FAILED) {
			perror("mmap ");
			printf("failed to mmap() Myrinet board space\n");
			return (0);
		}

		if (((BINFO[u_num].lanai_control & 0xF) == 2) ||
			((BINFO[u_num].lanai_control & 0xF) == 3)) {
		    /*
		    ** this is an L5 (revision==2) board
		    ** or an L7 (revision == 2|3) board
		    */
		    char *base = (char *) mb_map;
		    LANAI_BOARD[u_num] = (volatile unsigned int *) base;
		    LANAI3[u_num] = (void *)           (base + 0x00000000);
		    LANAI[u_num] = (void *)            (base + 0x00000000);
		    EEPROM = (struct MYRINET_EEPROM *) (base + 0x00A00000);
		    LANAI_SPECIAL[u_num] = (void *)    (base + 0x00804000);
		    LANAI5_SPECIAL[u_num] = (void *)   (base + 0x00804000);
		    LANAI_CONTROL[u_num] = (void *)    (base + 0x00800040);

		    UBLOCK[u_num] = (void *) (base + (16 * 1024 * 1024));

		    {
			char *prom = malloc(sizeof(struct MYRINET_EEPROM));

#ifdef NOTOUCH
			/* can't touch the EEPROM */
#else
			memcpy(prom, EEPROM, sizeof(struct MYRINET_EEPROM));
			LANAI_EEPROM[u_num] = (struct MYRINET_EEPROM *) prom;

			if (match_board_id((char *)&(EEPROM->lanai_board_id))) {
			    /* OK, no more copy */
			} else {
			    /* funny quicklogic board */
			    printf("%s: %s() We don't support funny quicklogic "
				"boards\n", __FILE__,"open_lanai_device_linux");
			}
#endif /* NOTOUCH */
		    }
		} else {
		    LANAI_BOARD[u_num] = (volatile unsigned int *)
			mb_map->lanai_eeprom;

		    EEPROM = (struct MYRINET_EEPROM *) mb_map->lanai_eeprom;

		    LANAI_CONTROL[u_num] = mb_map->lanai_control;
		    LANAI_SPECIAL[u_num] = (struct LANAI_REG *)
			mb_map->lanai_registers;
		    LANAI5_SPECIAL[u_num] = (struct LANAI7_REG *)
			mb_map->lanai_registers;
		    LANAI3[u_num] = mb_map->lanai_memory;
		    LANAI[u_num] = (volatile unsigned short *) LANAI3[u_num];
		    UBLOCK[u_num] = (unsigned int *) (((unsigned char *)
			LANAI[u_num]) + BINFO[u_num].lanai_memory_size);

		    if (EEPROM && !LANAI_EEPROM[u_num]) {
			/*
			 * copy the EEPROM to regular memory to speed up
			 * accesses
			 */
#ifndef NO_COPY_EEPROM
			char *prom = malloc(sizeof(struct MYRINET_EEPROM));

			memcpy(prom, EEPROM, sizeof(struct MYRINET_EEPROM));
			LANAI_EEPROM[u_num] = (struct MYRINET_EEPROM *) prom;
#else
			LANAI_EEPROM[u_num] = EEPROM;
#endif							/* NO_COPY_EEPROM */

			printf("%s:%d   CPU Version 0x%08x\n", __FILE__,
			    __LINE__, EEPROM->lanai_cpu_version);
			if (
			(ntohs(LANAI_EEPROM[u_num]->lanai_cpu_version) >= (short)0x0200) &&
			(ntohs(LANAI_EEPROM[u_num]->lanai_cpu_version) <= (short)0x08ff)) {
				/* OK */
			} else {

			    printf("LANAI_EEPROM[%d] looks bad??\n", u_num);

			    printf("EEPROM values = \n");
			    printf("\tclockval = %08lx\n", ntohl(LANAI_EEPROM[u_num]->lanai_clockval));
			    printf("\tcpu      = %04x\n", ntohs(LANAI_EEPROM[u_num]->lanai_cpu_version));
			    printf("\tid       = %02x:%02x:%02x:%02x:%02x:%02x\n",
				       LANAI_EEPROM[u_num]->lanai_board_id[0],
				       LANAI_EEPROM[u_num]->lanai_board_id[1],
				       LANAI_EEPROM[u_num]->lanai_board_id[2],
				       LANAI_EEPROM[u_num]->lanai_board_id[3],
				       LANAI_EEPROM[u_num]->lanai_board_id[4],
				       LANAI_EEPROM[u_num]->lanai_board_id[5]);
			    printf("\tsram     = %ld Bytes  (%d)\n",
				       ntohl(LANAI_EEPROM[u_num]->lanai_sram_size),
				       (int) lanai_memory_size(u_num));
			    printf("\tdelay    = 0x%04x\n", ntohs(LANAI_EEPROM[u_num]->delay_line_value));
			    printf("\tboardtype= 0x%04x\n", ntohs(LANAI_EEPROM[u_num]->board_type));
			    printf("\tbus_type = 0x%04x\n", ntohs(LANAI_EEPROM[u_num]->bus_type));



			    LANAI_EEPROM[u_num]->lanai_cpu_version = htons(0x0400);
			    LANAI_EEPROM[u_num]->lanai_sram_size = htonl(128 * 1024);
			    LANAI_EEPROM[u_num]->lanai_clockval = htonl(0x11371137);
			    LANAI_EEPROM[u_num]->lanai_board_id[0] = 0;
			    LANAI_EEPROM[u_num]->lanai_board_id[1] = 0x60;
			    LANAI_EEPROM[u_num]->lanai_board_id[2] = 0xDD;
			    LANAI_EEPROM[u_num]->lanai_board_id[3] = 0x80;
			    LANAI_EEPROM[u_num]->lanai_board_id[4] = 0;
			    LANAI_EEPROM[u_num]->lanai_board_id[5] = 0;
			    strcpy(LANAI_EEPROM[u_num]->fpga_version, "PCI Test Fixture");
			}
		    }


		    if (LANAI_EEPROM[u_num] == NULL) {
			printf("failed to map EEPROM space\n");
			return (0);
		    }

		}

		u_num++;
	}

	if (!u_num) {
	    gethostname(host_name, 80);
	    fprintf(stderr, "open_lanai_device_linux() Can't open lanai device "
		"on %s.\n", host_name);
	    return 0;
	}
	return (u_num);
}


int
open_lanai_device(void)
{
	return open_lanai_device_linux();
}

/* end of LinuxDevice.c */
