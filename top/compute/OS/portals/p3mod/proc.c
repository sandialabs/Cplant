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
/*
** $Id: proc.c,v 1.12 2001/08/16 16:10:24 pumatst Exp $
** Portals 3.0 module file that generates the files in /proc/cplant
*/

#include <asm/uaccess.h>
#include <sys/defines.h>			/* For MIN() */
#include "debug.h"
#include "stat.h"
#include "devices.h"
#include "proc.h"
#include "versions.h"


#define MAX_PRINT_BUF		(8 * 8 * 1024)
char print_buf[MAX_PRINT_BUF];

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
int
p3_read_debug_proc_indirect(char *buf, char **start, off_t off, int len,
                            int unused)
{
  int eof=0;
  return p3_read_debug_proc(buf, start, off, len, &eof, NULL);
}

int
p3_read_debug_proc(char *buf, char **start, off_t off, int count, 
                            int *eof, void *data)
{

int copy_len;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	p3_debug_proc(&pb, pb + MAX_PRINT_BUF);
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(count, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }


    *eof = 1;
    return copy_len;

}  /* end of p3_read_debug_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
int
p3_read_dev_proc_indirect(char *buf, char **start, off_t off, int len, 
                          int unused)
{
  int eof=0;
  return p3_read_dev_proc(buf, start, off, len, &eof, NULL);
}

int
p3_read_dev_proc(char *buf, char **start, off_t off, int count, int *eof,
                 void *data)
{
  int copy_len;
  int i;
  static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	pb += sprintf(pb, "Devices registered with the P3 module:\n");
	pb += sprintf(pb, "Slot Name                   Send func          Recv "
		"func          Exit func\n");
	for (i= 0; i < MAX_P3DEV; i++)   {
	    if (p3dev[i].in_use)   {
		pb += sprintf(pb, "%2d:  %-20s   0x%p 0x%p 0x%p\n", i,
		    p3dev[i].name, p3dev[i].send_fun, p3dev[i].recv_fun,
		    p3dev[i].exit_fun);
	    } else   {
		pb += sprintf(pb, "%2d:  %-20s\n", i, "available");
	    }
	}
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(count, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

    *eof = 1;
    return copy_len;

}  /* end of p3_read_dev_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
int
p3_read_nal_proc_indirect(char *buf, char **start, off_t off, int len, 
                          int unused)
{
   int eof=0;
   return p3_read_nal_proc(buf, start, off, len, &eof, NULL);
}

int
p3_read_nal_proc(char *buf, char **start, off_t off, int count, int *eof,
                 void *data)
{

int copy_len;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	p3_stat_proc(&pb);
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(count, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

    *eof = 1;
    return copy_len;

}  /* end of p3_read_nal_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
int
p3_read_versions_proc_indirect(char *buf, char **start, off_t off, int len, 
                               int unused)
{
   int eof=0;
   return p3_read_versions_proc(buf, start, off, len, &eof, NULL);
}

int
p3_read_versions_proc(char *buf, char **start, off_t off, int count, int *eof,
                      void *data)
{

int copy_len;
int i;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	pb += sprintf(pb, "Nbr File name            Ver     Date         "
	    "Time       User\n");
	pb += sprintf(pb, "----------------------------------------------"
	    "-----------------\n");

	i= 0;
	while (version_strings[i].file != NULL)   {
	    pb += sprintf(pb, "%-3d %-20s %-7s %-12s %-10s %-12s\n", i,
		    version_strings[i].file, version_strings[i].version,
		    version_strings[i].date, version_strings[i].time,
		    version_strings[i].user);
	    i++;
	}
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(count, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

    *eof = 1;
    return copy_len;

}  /* end of p3_read_versions_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
