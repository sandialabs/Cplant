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
** $Id: library.c,v 1.4 2001/04/26 20:39:02 rolf Exp $
**
** This is a simple library that provides common functions to deal with
** rtscts info requests.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "../util/timing.h"	/* For start() stop() */
#include "RTSCTS_ioctl.h"
#include "RTSCTS_info.h"	/* For get_info_t */
#include "library.h"


#define DEVICE_NAME	"/dev/rtscts"

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
info_init(char *pname, int verbose)
{

int fd;


    fd= open(DEVICE_NAME, O_RDWR);
    if ((fd < 0) && (verbose > 1))   {
	fprintf(stderr, "%s: Could not open %s\n", pname, DEVICE_NAME);
	perror("");
    }

    return fd;

}  /* end of info_init() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
info_get_data(char *pname, int fd, int verbose, int clear, int node,
    seg_info_t segment, void *buf)
{

get_info_req_t get_info;
int cnt, rc;


    /* request info */
    get_info.node= node;
    get_info.seg= segment;
    get_info.buf= buf;
    get_info.clear= clear;
    rc= ioctl(fd, RTS_REQ_INFO, &get_info);
    if (rc != 0)    {
	if (verbose > 1)   {
	    fprintf(stderr,"%s: info_get_data() ioctl(fd %d, req %d, ...) "
		"failed\n", pname, fd, RTS_GET_INFO);
	}
	return -1;
    }

    /* Wait for data */
    start();
    cnt= 0;
    do   {
	rc= ioctl(fd, RTS_GET_INFO, buf);
	cnt++;
	if ((cnt > 10) && (stop() > 0.15))   {
	    if (verbose > 1)   {
		fprintf(stderr, "%s: info_get_data() timed out for node %d\n",
		    pname, node);
	    }
	    return -2;
	}
    } while (rc < 0);

    return rc;

}  /* end of info_get_data() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
