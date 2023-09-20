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
** $Id: RTSCTS_route.h,v 1.11 2001/08/22 23:07:01 pumatst Exp $
*/
#ifndef RTSCTS_ROUTE_H
#define RTSCTS_ROUTE_H

#include "MCPshmem.h"		/* For MAX_NUM_ROUTES */

extern int pINgFO[MAX_NUM_ROUTES];

    typedef enum {ROUTE_UNTESTED, ROUTE_NO_ANSWER, ROUTE_BAD_DNID,
         ROUTE_NOT_SET, ROUTE_OK, ROUTE_CONFUSING_REPLY} route_status_t;

    void handlePING(int pnid);
    void handlePINGA(int pnid);
    void handlePINGR(int pnid, char* buf, int len);
    void handleROUTE_REQ_REPLY(int rte_nid0, route_status_t status);
    void handleRouteStat(unsigned int msgID, unsigned short src_nid,
		    unsigned short expected_pnid);
    void handleRouteAck(unsigned int msgID, unsigned short src_nid,
		    unsigned short expected_pnid);
    int sendRouteStat(unsigned short int dst_nid, int src_msgID);
    void sendRouteAck(unsigned short dst_nid, unsigned short expected_pnid);

    extern route_status_t route_status[MAX_NUM_ROUTES];
    extern route_status_t route_request;
    extern unsigned long route_used[MAX_NUM_ROUTES];
    extern int rte_nids[2];

#endif /* RTSCTS_ROUTE_H */
