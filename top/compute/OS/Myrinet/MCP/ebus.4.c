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
** $Id: ebus.4.c,v 1.1 1999/10/20 23:18:30 rolf Exp $
**
** This file contains functions that deal with the EBUS side; i.e.
** functions to DMA to and from the host, that are specific to LANai 4.x
*/

#include "ebus.h"

int PCIavailable= TRUE;

/******************************************************************************/

void
ebus_dma_init(void)
{
    /* No need to set anything up on LANai 4.x */
}  /* end of ebus_dma_init() */

/******************************************************************************/
