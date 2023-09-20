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
** $Id: srvr_comm_get.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COMM_GET_H
#define SRVR_COMM_GET_H

#include "puma.h"
#include "portal.h"
#include "srvr_comm_ctl.h"

INT32 srvr_comm_get_req(CHAR *buf, INT32 len, int myptl,
                INT32 msg_type, CHAR *user_data, INT32 user_data_len,
                UINT16 nid, UINT16 pid, UINT16 ptl,
		INT32 blocking, double tmout);

VOID srvr_comm_free_handle(int ptl, INT32 mle);

INT32 srvr_comm_get_reply(control_msg_handle *handle,
                    VOID *reply_buf, INT32 reply_len);
INT32 srvr_comm_get_reply_partial(control_msg_handle *handle,
                    VOID *reply_buf, INT32 reply_len, INT32 offset);

#endif
