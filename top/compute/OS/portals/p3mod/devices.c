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
** $Id: devices.c,v 1.17 2002/02/14 23:33:29 jbogden Exp $
** Portals 3.0 module file that interfaces with lower level device
** drivers such as the rtscts or the pkt module.
*/

#include <asm/uaccess.h>
#include <sys/defines.h>		/* For TRUE */
#include <p30/lib-types.h>	/* For ptl_hdr_t */
#include "devices.h"


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

p3dev_t p3dev[MAX_P3DEV];

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
devices_init(void)
{

int i;

    for (i= 0; i < MAX_P3DEV; i++)   {
	p3unregister_dev(i);
    }

}  /* end of devices_init() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3register_dev(char *dev_name,
	int (*send_fun)(nal_cb_t *nal, void *buf, size_t len, int dnid,
		ptl_hdr_t *hdr, lib_msg_t *cookie, unsigned int msgID,
		unsigned int msgNum, unsigned int opt),
	int (*recv_fun)(nal_cb_t *nal, void *private, void *buf, size_t mlen,
		size_t rlen, lib_msg_t *cookie),
    int (* dist_fun)(nal_cb_t *nal, int nid, unsigned long *dist),
	void (*exit_fun)(void *task)
	)
{

int i;
int slot;


    /* Find a free slot in the device table */
    slot= -1;
    for (i= 0; i < MAX_P3DEV; i++)   {
	if (!p3dev[i].in_use)   {
	    slot= i;
	    break;
	}
    }

    if (slot < 0)   {
	printk("p3register_dev() No more slots available!\n");
	return slot;
    }

    /* Initialize the device structure */
    p3dev[slot].in_use= TRUE;
    p3dev[slot].send_fun= send_fun;
    p3dev[slot].recv_fun= recv_fun;
    p3dev[slot].dist_fun= dist_fun;
    p3dev[slot].exit_fun= exit_fun;
    strncpy(p3dev[slot].name, dev_name, MAX_P3DEV_NAME);

    /* return the function pointers for receive start and end */

    return slot;

}  /* end of p3register_dev() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3unregister_dev(int slot)
{
    if ((slot < 0) || (slot >= MAX_P3DEV))   {
	printk("p3unregister() Illegal slot value %d\n", slot);
	return;
    }

    if ( !p3dev[slot].in_use)   {
	printk("p3unregister() Slot %d was not in use!\n", slot);
    }

    p3dev[slot].name[0]= '\0';
    p3dev[slot].in_use= FALSE;
    p3dev[slot].send_fun= NULL;
    p3dev[slot].recv_fun= NULL;
    p3dev[slot].dist_fun= NULL;
    p3dev[slot].exit_fun= NULL;
}

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
/*
** This function should really take an argument to find the right
** device. For now, we just pick the first one we find.
*/
int
p3find_dev(void)
{

int slot;


    for (slot= 0; slot < MAX_P3DEV; slot++)   {
	if (p3dev[slot].in_use)   {
	    return slot;
	}
    }

    return -1;

}  /* end of p3find_dev() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3send(int slot, nal_cb_t *nal, user_ptr data, size_t len, int dnid,
	ptl_hdr_t *hdr, lib_msg_t *cookie)
{
    return p3dev[slot].send_fun(nal, data, len, dnid, hdr, cookie, 0, 0, 0);
}  /* end of p3send() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3recv(int slot, nal_cb_t *nal, void *private, user_ptr data, size_t mlen,
	size_t rlen, lib_msg_t *cookie)
{
    return p3dev[slot].recv_fun(nal, private, data, mlen, rlen, cookie);

}  /* end of p3recv() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
p3dist(int slot, nal_cb_t *nal, int nid, unsigned long *dist )
{
    return p3dev[slot].dist_fun(nal, nid, dist );
}  /* end of p3dist() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3exit(int slot, void *task)
{
    p3dev[slot].exit_fun(task);
}  /* end of p3exit() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
