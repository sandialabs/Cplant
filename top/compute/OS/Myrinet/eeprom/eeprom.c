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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lanai_device.h"
#include "myriInterface.h"

int get_lanai_eeprom_linux(struct MYRINET_EEPROM* prom, int unit);

int
main(int argc, char *argv[])
{

int status;
int unit;
unsigned int max_lanai_speed;
struct MYRINET_EEPROM the_eeprom;
struct MYRINET_EEPROM *EEPROM = &the_eeprom;


    if (argc == 1)   {
	unit= 0;
    } else if (argc == 2)   {
	unit= strtol(argv[1], NULL, 10);
    } else   {
	fprintf(stderr, "Usage: %s [unit]\n", argv[0]);
	return -1;
    }

    /* copy the lanai eeprom to the_eeprom */
    if (!(status = get_lanai_eeprom_linux(EEPROM, unit))) {
	printf("%s: ERROR -- could not get lanai eeprom\n", argv[0]);
	return -1;
    }

    printf("LANai EEPROM values for unit %d\n", unit);
    printf("\tclockval     = 0x%08lx\n", ntohl(EEPROM->lanai_clockval));
    printf("\tcpu          = 0x%04x: LANai %d.%d\n",
	ntohs(EEPROM->lanai_cpu_version),
	(ntohs(EEPROM->lanai_cpu_version) & 0xff00) >> 8,
	ntohs(EEPROM->lanai_cpu_version) & 0xff);
    printf("\tid           = %02x:%02x:%02x:%02x:%02x:%02x\n",
	EEPROM->lanai_board_id[0], EEPROM->lanai_board_id[1],
	EEPROM->lanai_board_id[2], EEPROM->lanai_board_id[3],
	EEPROM->lanai_board_id[4], EEPROM->lanai_board_id[5]);
    printf("\tsram         = %ld bytes (%ldkB)\n",
	ntohl(EEPROM->lanai_sram_size), ntohl(EEPROM->lanai_sram_size) / 1024);
    printf("\tdelay        = 0x%04x\n", ntohs(EEPROM->delay_line_value));
    printf("\tboardtype    = 0x%04x: ", ntohs(EEPROM->board_type));
    switch (ntohs(EEPROM->board_type))   {
	case MYRINET_BOARDTYPE_1MEG_SRAM: printf("1 meg SRAM board\n"); break;
	case MYRINET_BOARDTYPE_FPGA: printf("FPGA board\n"); break;
	case MYRINET_BOARDTYPE_L5: printf("LANai 5 board\n"); break;
	case MYRINET_BOARDTYPE_NONE: printf("none\n"); break;
	default: printf("unknown\n");
    }

    printf("\tbus_type     = 0x%04x: ", ntohs(EEPROM->bus_type));
    switch (ntohs(EEPROM->bus_type))   {
	case MYRINET_BUS_SBUS: printf("SBUS\n"); break;
	case MYRINET_BUS_PCI: printf("PCI bus\n"); break;
	case MYRINET_BUS_GSC: printf("GSC\n"); break;
	case MYRINET_BUS_FPGA: printf("FPGA\n"); break;
	case MYRINET_BUS_NONE: printf("none\n"); break;
	default: printf("Unknown\n");
    }

    printf("\tserial no    = %ld\n", ntohl(EEPROM->serial_number));
    printf("\tfpga vers    = %s\n", EEPROM->fpga_version);
    printf("\tmore vers    = %s\n", EEPROM->more_version);
    if (isprint(EEPROM->board_label[0]) && isprint(EEPROM->board_label[1]))   {
	printf("\tboard label  = %s\n", EEPROM->board_label);
    } else   {
	printf("\tboard label  = \n");
    }
    printf("\tproduct code = 0x%04x\n", ntohs(EEPROM->product_code));
    max_lanai_speed= ntohs(EEPROM->max_lanai_speed);
    if ((max_lanai_speed == 0) || (max_lanai_speed == (unsigned short)0xFFFF)) {
	max_lanai_speed = 67;
    }
    printf("\tmax speed    = 0x%04x (%d MHz)\n", ntohs(EEPROM->max_lanai_speed),
	max_lanai_speed);

    return 0;
}
