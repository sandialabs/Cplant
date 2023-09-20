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
** $Id: devices.h,v 1.18 2002/02/14 23:33:29 jbogden Exp $
** Portals 3.0 module file that interfaces with lower level device
** drivers such as the rtscts or the pkt module.
*/

#ifndef P3_DEVICES_H
#define P3_DEVICES_H

#include <p30/lib-types.h>	/* For ptl_hdr_t */
#include <p30/lib-nal.h>	/* For nal_cb_t */

#define MAX_P3DEV	(8)	/* Maximum of 8 devices can attach to us */
#define MAX_P3DEV_NAME	(128)	/* Max device name length */

typedef struct   {
    int (* send_fun)(nal_cb_t *nal, void *buf, size_t len, int dnid,
	    ptl_hdr_t *hdr, lib_msg_t *cookie, unsigned int msgID, 
            unsigned int msgNum, unsigned int opt);
    int (* recv_fun)(nal_cb_t *nal, void *private, void *buf, size_t mlen,
	    size_t rlen, lib_msg_t *cookie);
    int (* dist_fun)(nal_cb_t *nal, int nid, unsigned long *dist );
    void (*exit_fun)(void *task);
    int in_use;
    char name[MAX_P3DEV_NAME];
} p3dev_t;

extern p3dev_t p3dev[];

void devices_init(void);
int p3register_dev(char *dev_name, int (*send_fun)(nal_cb_t *nal, void *buf, 
	    size_t len, int dnid, ptl_hdr_t *hdr, lib_msg_t *cookie,
	    unsigned int msgID, unsigned int msgNum, unsigned int opt),
	    int (*recv_fun)(nal_cb_t *nal, void *private, void *buf,
	    size_t mlen, size_t rlen, lib_msg_t *cookie),
            int (* dist_fun)(nal_cb_t *nal, int nid, unsigned long *dist),
	    void (*exit_fun)(void *task));
void p3unregister_dev(int slot);
int p3find_dev(void);
int p3send(int slot, nal_cb_t *nal, user_ptr data, size_t len, 
           int dnid, ptl_hdr_t *hdr, lib_msg_t *cookie);
int p3recv(int slot, nal_cb_t *nal, void *private, user_ptr data, 
           size_t mlen, size_t rlen, lib_msg_t *cookie);
int p3dist(int slot, nal_cb_t *nal, int nid, unsigned long *dist );
void p3exit(int slot, void *task);

#endif /* P3_DEVICES_H */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
