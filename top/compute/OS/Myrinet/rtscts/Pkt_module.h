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
** $Id: Pkt_module.h,v 1.8 2001/08/22 23:07:01 pumatst Exp $
*/

#ifndef PKT_MODULE_H
#define PKT_MODULE_H

/*
** For now, we make sure to never allocate more than a single page.
** It seems the 2.0.x kernels have problems when we go beyond a page, and
** the PYXIS chipset on our Miata's can't cross a page boundary anyway.
*/
/*
** Leave a few bytes at the end of the page, in case we have to round
** up the receive length.
*/
#define FUDGE		(32)
#define MYRPKT_MTU	(PAGE_SIZE - FUDGE)
/* for ethernet    #define MYRPKT_MTU	(1500 - FUDGE) */
/* for gm on x86 lanai4  #define MYRPKT_MTU	(3752 - FUDGE) */
/* for gm on x86 lanai7  #define MYRPKT_MTU	(PAGE_SIZE - FUDGE) */
/* for gm on alpha lanai4 #define MYRPKT_MTU	(7848 - FUDGE) */
/* for gm on alpha lanai7 #define MYRPKT_MTU	(PAGE_SIZE - FUDGE) */

#endif /* PKT_MODULE_H */
