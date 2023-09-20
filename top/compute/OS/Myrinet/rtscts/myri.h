/* myri.h */
/* $Id: myri.h,v 1.8 2001/08/22 16:29:08 pumatst Exp $ */
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

/*
    Copyright 1994 Digital Equipment Corporation.

    This software may be used and distributed according to  the terms of the
    GNU Public License, incorporated herein by reference.

    The author may    be  reached as davies@wanton.lkg.dec.com  or   Digital
    Equipment Corporation, 550 King Street, Littleton MA 01460.

    =========================================================================
*/

/*
** Cut to size and adapted for Puma portals by Sandia National Laboratories
*/



#ifndef MYRI_H
#define MYRI_H

#define MLANAI_MAJOR	38
#define MLANAI_MAX	16

#ifdef LINUX24
#define NETSTAT net_device_stats
#define NETDEV  net_device
#else
#define NETSTAT enet_statistics
#define NETDEV  device
#endif


struct myri_private {
	char adapter_name[80];		/* Full Adapter name */
	struct NETSTAT stats;   	/* Public stats */

	void *free0;
	int unit;			/* the board number (minor dev num) */
	struct MYRINET_BOARD *mb;
	struct MYRINET_BOARD *mb_remap;

	unsigned int myrinet_address[2];
	int curr_send_index;
	int LANai_needs_buffers;	/* >0 indicates missed AddRecv */
	struct NETDEV *dev;
	struct NETDEV *next_m;
	int sram_size;			/* used in case EEPROM is mangled */
	int reset_counter;
	int revision;
};


struct mlanai_info {
	struct myri_private *myriP;
	int unit;
	char devname[16];		/* device name */
};

extern struct mlanai_info mlanai_private[];

extern int mlanai_init(struct myri_private * myriP);
extern int mlanai_uninit(struct myri_private * myriP);

#endif /* MYRI_H */
