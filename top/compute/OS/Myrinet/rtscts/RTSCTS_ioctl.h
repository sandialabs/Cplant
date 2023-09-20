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
** $Id: RTSCTS_ioctl.h,v 1.15 2002/03/14 00:54:04 jbogden Exp $
*/

#ifndef RTS_IOCTL
#define RTS_IOCTL

#define RTSCTS_MAJOR 61

/* Leave RTS_DO_TEST_ROUTE_W_SIZE last so that utils like cplant_lan_check
 * that probe for the existence of the RTS_DO_TEST_ROUTE_W_SIZE command
 * will correctly be backward compatible with older rtscts modules. This
 * works because the RTS_DO_TEST_ROUTE_W_SIZE command will be invalid
 * (to high of a value) on older modules.
*/
enum {RTS_SET_TEST_ROUTE_LEN, RTS_SET_REVERSE_ROUTE, 
      RTS_DO_TEST_ROUTE, RTS_DO_TEST_ROUTE_W_ACK, RTS_DO_TEST_ROUTE_W_REV,
      RTS_DO_PING, RTS_SET_PING, RTS_GET_PING,
      RTS_ROUTE_STAT, RTS_ROUTE_CHECK, 
      RTS_SET_ROUTE_ID, RTS_GET_ROUTE,
      RTS_ROUTE_REQ, RTS_ROUTE_REQ_RESULT, RTS_REQ_INFO, RTS_GET_INFO,
      RTS_CACHE_REQ, RTS_CACHE_RETRIEVE_DATA,
      RTS_REQ_P3_PING, RTS_GET_P3_PING,
      RTS_DO_TEST_ROUTE_W_SIZE};

/* Generic structure to pass more data via an ioctl() call to RTSCTS */
typedef struct {
    int type;
    int param1;
    char route[48];
    int buflen;
    void *buf;
} rts_ioctl_arg_t;

#endif
