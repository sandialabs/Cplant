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
** $Id
** Statistics about the P3 module.
*/

#include <linux/kernel.h>	/* For sprintf() */
#include <linux/string.h>	/* For memset() */
#include <asm/system.h>		/* For cli(), sti() */
#include "stat.h"

#define MIN_VALUE		(999999999L)

nalstat_t nalstat;

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3_stat_init(void)
{

    memset((void *)&nalstat, 0, sizeof(nalstat_t));
    nalstat.FwdArgLenMin= MIN_VALUE;
    nalstat.FwdRetLenMin= MIN_VALUE;

}  /* end of p3_stat_init() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3_stat_proc(char **pb_ptr)
{

char *pb;
int i;


    pb= *pb_ptr;

    pb += sprintf(pb, "Network Abstraction Layer (NAL) statistics for P3 "
	    "module:\n");
    pb += sprintf(pb, "ptlNIInit()\n");
    pb += sprintf(pb, "    Successful                           %12ld\n",
	    nalstat.NIInitOK);
    pb += sprintf(pb, "    Failed                               %12ld\n",
	    nalstat.NIInitBAD);
    pb += sprintf(pb, "ptlNIFini()\n");
    pb += sprintf(pb, "    Total calls                          %12ld\n",
	    nalstat.NIFini);
    pb += sprintf(pb, "    Still open                           %12ld\n",
	    nalstat.NIInitOK - nalstat.NIFini);
    pb += sprintf(pb, "ioctl()\n");
    pb += sprintf(pb, "    Successful                           %12ld\n",
	    nalstat.ioctlOK);
    pb += sprintf(pb, "    Access to icotl args failed          %12ld\n",
	    nalstat.ioctlBad);
    pb += sprintf(pb, "    Mlock_all calls                      %12ld\n",
	    nalstat.ioctlMlockall);
    pb += sprintf(pb, "    Unknown ioctl calls                  %12ld\n",
	    nalstat.ioctlUnknown);
    pb += sprintf(pb, "nal->forward\n");
    pb += sprintf(pb, "    Successful                           %12ld\n",
	    nalstat.FwdOK);
    pb += sprintf(pb, "    Access to usr lvl myrnal_fwd failed  %12ld\n",
	    nalstat.FwdBAD1);
    pb += sprintf(pb, "    Args to fwd too long                 %12ld\n",
	    nalstat.FwdBAD2);
    pb += sprintf(pb, "    Return arg too long                  %12ld\n",
	    nalstat.FwdBAD3);
    pb += sprintf(pb, "    Access to fwd args failed            %12ld\n",
	    nalstat.FwdBAD4);
    pb += sprintf(pb, "    Access to return args failed         %12ld\n",
	    nalstat.FwdBAD5);
    pb += sprintf(pb, "    Disptach cmd out of range            %12ld\n",
	    nalstat.FwdBAD6);
    pb += sprintf(pb, "    Can't get index for spid             %12ld\n",
	    nalstat.FwdBAD7);
    pb += sprintf(pb, "    Writing to return args failed        %12ld\n",
	    nalstat.FwdBAD8);
    pb += sprintf(pb, "    Total fwd argument list length       %12ld\n",
	    nalstat.FwdArgLen);
    if (nalstat.FwdOK > 0)   {
	pb += sprintf(pb, "    Average fwd argument list length     %12ld\n",
		nalstat.FwdArgLen / nalstat.FwdOK);
    } else   {
	pb += sprintf(pb, "    Average fwd argument list length     %12ld\n",
		nalstat.FwdArgLen);
    }
    pb += sprintf(pb, "    Max fwd argument list length         %12ld\n",
	    nalstat.FwdArgLenMax);
    pb += sprintf(pb, "    Min fwd argument list length         %12ld\n",
	    nalstat.FwdArgLenMin);
    if (nalstat.FwdOK > 0)   {
	pb += sprintf(pb, "    Average fwd return argument list len %12ld\n",
		nalstat.FwdRetLen / nalstat.FwdOK);
    } else   {
	pb += sprintf(pb, "    Average fwd return argument list len %12ld\n",
		nalstat.FwdRetLen);
    }
    pb += sprintf(pb, "    Max fwd return argument list length  %12ld\n",
	    nalstat.FwdRetLenMax);
    pb += sprintf(pb, "    Min fwd return argument list length  %12ld\n",
	    nalstat.FwdRetLenMin);
    pb += sprintf(pb, "Dispatch\n");
    for (i= 0; i < LIB_MAX_DISPATCH; i++)   {
	pb += sprintf(pb, "    %-20s                 %12ld\n",
		dispatch_name(i), nalstat.dispatch[i]);
    }

    *pb_ptr= pb;

}  /* end of p3_stat_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
