/* myriFiles.c */

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
 * Arcadia, CA 91024                                                     *
 * 818 821-5555                                                          *
 * http://www.myri.com                                                   *
 *************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "bfd.h"
#include "MyrinetPCI.h"		/* For LANAI5_RESET_OFF etc. */
#include "lanai_device.h"

#ifdef dec_osf1
#define long int
#endif

static int bfd_initialized = 0;

#define bfd_perror(x)	printf("bfd_error: %s\n",x)


static inline
unsigned int
ulong_max(const unsigned long a, const unsigned long b)
{
    return (a > b ? a : b);
}

static inline
unsigned int
ulong_min(const unsigned long a, const unsigned long b)
{
    return (a < b ? a : b);
}

/*
** Load the data from a section with addresses in the range [buffer_offset,
** buffer_offset+buffer_size) into buffer. Return true on success, false on
** failure.
** 
** We do this rather than load directly into the lanai so the function can also
** be used by the simulator.
** 
** We do this by sections so "gendat" can be written using this function.
*/
int
lanai_read_section(bfd *abfd, asection *section, char *buffer,
    const unsigned int buffer_offset, const unsigned int buffer_size)
{
    unsigned long load_start, load_end, section_size;

    section_size = (unsigned long) bfd_section_size(abfd, section);
    if (section_size == 0)
        return true;    /* Nothing to load */

    load_start = ulong_max(section->vma, buffer_offset);
    load_end = ulong_min(section->vma + section_size,
                 buffer_offset + buffer_size);
    if (load_start >= load_end)
        return true;    /* Nothing to load. */

    /*
    fprintf(stderr, "Loading section \"%s\" at location 0x%x.\n",
        bfd_section_name(abfd, section), section->vma);
    */

    return bfd_get_section_contents(abfd, section,
		(PTR) ((unsigned char *) buffer +
		(unsigned long) (load_start - buffer_offset)),
                load_start - (unsigned long) section->vma,
		load_end - load_start);
}


/*
** Load the contents of an executable into a memory buffer representing
** buffer_size bytes of lanai memory starting at lanai address `offset'
*/
int
lanai_load_buffer_from_executable(char *buffer, const unsigned int offset,
    const unsigned int buffer_size, const char *const filename)
{
    bfd *abfd;

    int status;
    const char *section_names[4] = {".text", ".data", ".bss", NULL};
    const char **section_name;


    if (!filename) {
        fprintf(stderr, "Null filename.\n");
        return false;
    }
    if (!bfd_initialized) {
        bfd_init();
        bfd_initialized = 1;
    }
    abfd = bfd_openr(filename, NULL);
    if (!abfd) {
        bfd_perror("Error opening file.");
        return false;
    }
    if (!bfd_check_format(abfd, bfd_object)) {
        bfd_perror("Not an object file");
        status = false;
        goto close;
    }
    for (section_name = &section_names[0]; *section_name; section_name++) {
        asection *section;
        section = bfd_get_section_by_name(abfd, *section_name);
        if (!section) {
            /*
             * It is perfectly acceptable not to find a section.
             */
            status = true;
            goto close;
        }
        if (!lanai_read_section(abfd, section, buffer, offset, buffer_size)) {
            fprintf(stderr, "Could not read section named \"%s\".\n",
                *section_name);
            status = false;
            goto close;
        }
    }
    status = true;

close:
    if (!bfd_close(abfd))
        return false;

    return status;
}


/*
** allocates a buffer and loads it with the contents of a lanai file
** of some sort. returns size of buffer. caller should free buffer
*/
unsigned
lanai_new_loaded_buffer(const unsigned unit, const char *filename, char **p)
{
    unsigned int size;

    if (!p)   {
        fprintf(stderr, "lanai_new_loaded_buffer() p is NULL!\n");
        return 0;
    }

    size = lanai_memory_size(unit);

    if (!size)   {
        fprintf(stderr, "lanai_new_loaded_buffer() size is 0!\n");
        return 0;
    }

    /* get buffer */
    if (!(*p = calloc(sizeof(char), size))) {
        bfd_perror("Could not malloc() buffer.");
        return 0;
    }

    /* load file data into buffer */
    if (!lanai_load_buffer_from_executable(*p, 0, size, filename)) {
        free(*p);
        return 0;
    }

    /* return size of buffer */
    return size;
}


/*
** a load+reset less likely to crater when called over NFS over myrinet
*/
int
lanai_load_and_reset(const unsigned unit, const char *filename)
{
    char *buffer = NULL;
    unsigned size;

    if (!(unit < 64 && filename && *filename))   {
        fprintf(stderr, "lanai_load_and_reset() assert\n");
        return 0;
    }

    /* get a new loaded buffer */
    if (!(size = lanai_new_loaded_buffer(unit, filename, &buffer)))
        return 0;

    if (!buffer)   {
	fprintf(stderr, "lanai_load_and_reset() buffer is NULL\n");
	return 0;
    }

    /* freeze the lanai */
    lanai_reset_unit(unit, 1);

    /* write the buffer into the lanai. */
    lanai_put(unit, buffer, 0, size);

    /* now un-reset the lanai */
    lanai_reset_unit(unit, 0);

    /* and set interrupts on or off */
    lanai_interrupt_unit(unit, 0);

    /* set the LANai version register (only needed on L4) */
    switch (lanai_board_type(unit)) {
        case lanai_4_0:
            setVERSION(unit, L4, 0);
            break;

        case lanai_4_1:
        case lanai_4_2:
        case lanai_4_3:
        case lanai_4_4:
        case lanai_4_5:
            setVERSION(unit, L4, 3);
            break;

        case lanai_7_0:
        case lanai_7_1:
        case lanai_7_2:
            break;

        case lanai_9_0:
        case lanai_9_1:
        case lanai_9_2:
        case lanai_9_3:
            break;

        default:
            /* We don't support any other LANai types */
	    return 0;
    }

    /* free the buffer. */
    free(buffer);
    *(unsigned int *) &LANAI_CONTROL[unit][0] = (
	LANAI5_RESET_OFF | LANAI5_ERESET_OFF | LANAI5_BRESET_OFF |
	LANAI5_INT_ENABLE | LANAI5_FORCE_64BIT_MODE );

    #ifdef VERBOSE
    printf("Set control register to 0x%08x, now 0x%08x\n" ,
	LANAI5_RESET_OFF | LANAI5_ERESET_OFF | LANAI5_BRESET_OFF |
	LANAI5_INT_ENABLE | LANAI5_FORCE_64BIT_MODE,
	(*(unsigned int *) &LANAI_CONTROL[unit][0]));
    #endif /* VERBOSE */

    return 1;
}
