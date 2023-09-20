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
** $Id: srvr_comm_put.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COMM_PUT_H
#define SRVR_COMM_PUT_H

#include "puma.h"
#include "portal.h"
#include "srvr_comm_ctl.h"

INT32 srvr_comm_put_buffer(VOID *buf, INT32 len, int myptl);

INT32 srvr_comm_put_req(INT32 msg_type,  int myptl, INT32 mle,
                  UINT16 nid, UINT16 pid, UINT16 ptl,
                  CHAR *user_data, INT32 user_data_len);


INT32 srvr_comm_put_reply(control_msg_handle *mh, VOID *recv_buf, 
                          INT32 recv_len);
INT32 srvr_comm_put_reply_partial(control_msg_handle *mh,
	       VOID *recv_buf, INT32 buf_len, INT32 offset);



#endif
