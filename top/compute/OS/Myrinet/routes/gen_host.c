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
** $Id: gen_host.c,v 1.4 2001/04/06 23:02:27 jsotto Exp $
** This is the function that converts a sw and port number to a host
** name. This file is machine specific for Siberia. Link it with
** pnid2hname.c to get a little tool that converts physical node ID's
** to host names.
*/
#include "gen_host.h"

/* +------------------------------------------------------------------------+ */
/* |   Siberia                                                              | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */
/*
** Each switch is in some SU. Unfortunately, in Siberia the correspondence
** is not as simple as sw = su / 2. Therefore, the table.
** Racks (switches) 6, 15, 22, 31, 62, and 71 are I/O racks and have only
** two nodes attached to them. They are all collected in SU 37.
*/
int sw2su[] = {
    0, 0, 1, 1, 2, 2, 37, 3, 3, 4, 4, 5, 5, 6, 6, 37, 7, 7, 8, 8, 9, 9, 37, 10,
    10, 11, 11, 12, 12, 13, 13, 37, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19,
    19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28,
    37, 29, 29, 30, 30, 31, 31, 32, 32, 37, 33, 33, 34, 34, 35, 35, 36, 36, 999
};

/*
** Given a switch number and a port number, generate a host name
*/
void
gen_host_name_siberia(char *label, int sw, int port)
{

    sprintf(label, "ERROR switch %d, port %d", sw, port);

    switch (sw)   {
	case 6:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 0, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 0, sw2su[sw]);
	    } else   {
		sprintf(label, "Happy-%d.Snowwhite", port + 0);
	    }
	    break;
	case 15:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 2, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 2, sw2su[sw]);
	    } else   {
		sprintf(label, "Grumpy-%d.Snowwhite", port + 0);
	    }
	    break;
	case 22:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 4, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 4, sw2su[sw]);
	    } else   {
		sprintf(label, "Bashful-%d.Snowwhite", port + 0);
	    }
	    break;
	case 31:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 6, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 6, sw2su[sw]);
	    } else   {
		sprintf(label, "Doepy-%d.Snowwhite", port + 0);
	    }
	    break;
	case 62:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 8, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 8, sw2su[sw]);
	    } else   {
		sprintf(label, "Sleepy-%d.Snowwhite", port + 0);
	    }
	    break;
	case 71:
	    if (port == 0)   {
		sprintf(label, "c-%d.SU-%d", port + 10, sw2su[sw]);
	    } else if (port == 1)   {
		sprintf(label, "c-%d.SU-%d", port + 10, sw2su[sw]);
	    } else   {
		sprintf(label, "Sneezy-%d.Snowwhite", port + 0);
	    }
	    break;
	default:
	    /* The rules for all non-I/O nodes */
	    if ((sw2su[sw] != sw2su[sw + 1])  && (sw2su[sw] != 37))  {
		/* We must be in the second rack of this SU */
		sprintf(label, "c-%d.SU-%d", port + 8, sw2su[sw]);
	    } else   {
		/* We must be in the first rack of this SU */
		sprintf(label, "c-%d.SU-%d", port, sw2su[sw]);
	    }
    }

}  /* end of gen_host_name_siberia() */


/*
** Given a switch number, generate a switch name
*/
void
gen_switch_name_siberia(char *label, int sw)
{

    if ((sw2su[sw] != sw2su[sw + 1])  && (sw2su[sw] != 37))  {
	/* This switch must be m-1 */
	sprintf(label, "m-1.SU-%d", sw2su[sw]);
    } else   {
	/* This switch must be m-0 */
	sprintf(label, "m-0.SU-%d", sw2su[sw]);
    }

}  /* end of gen_switch_name_siberia() */


/* +------------------------------------------------------------------------+ */
/* |   Iceberg                                                              | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */
void
gen_switch_name_iceberg(char *label, int sw)
{
    sprintf(label, "m-0.SU-%d", sw);
}  /* end of gen_switch_name_iceberg() */


void
gen_host_name_iceberg(char *label, int pnid)
{
   sprintf(label, "c-%d.SU-%d", pnid % 8, pnid / 8);
}  /* end of gen_host_name_iceberg() */


/* +------------------------------------------------------------------------+ */
/* |   Iceberg2                                                             | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */
void
gen_switch_name_iceberg2(char *label, int sw)
{
    sprintf(label, "m-0.SU-%d", sw);
}  /* end of gen_switch_name_iceberg2() */


void
gen_host_name_iceberg2(char *label, int pnid)
{
   sprintf(label, "c-%d.SU-%d", pnid % 8, pnid / 8);
}  /* end of gen_host_name_iceberg2() */


/* +------------------------------------------------------------------------+ */
/* |   Alaska                                                               | */
/* |                                                                        | */
/* +------------------------------------------------------------------------+ */
void
gen_host_name_alaska(char *label, int pnid)
{
   sprintf(label, "c-%d.SU-%d", pnid % 16, pnid / 16);
}  /* end of gen_host_name_alaska() */
