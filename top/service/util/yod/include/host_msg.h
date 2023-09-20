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
/* $Id: host_msg.h,v 1.9 2001/02/16 05:44:34 lafisk Exp $ */

#ifndef USHOST_MSG_H
#define USHOST_MSG_H


#include "defines.h"


#define MAX_WORK_BUFS     (4)
#define WORK_BUF_SIZE     (10*1024)
 

INT32 host_writemem(INT32 buf_num, INT32 len, int portal,
	    UINT16 acl_idx, NID_TYPE dst_nid, PID_TYPE dst_pid, UINT32 offset);
INT32 host_readmem(INT32 buf_num, INT32 len, int portal,
	    UINT16 acl_idx, NID_TYPE dst_nid, PID_TYPE dst_pid, UINT32 offset);

INT32 get_work_buf(VOID);
INT32 wait_work_buf(INT32 buf_num, INT32 expected);
INT32 reset_work_buf(INT32 buf_num, INT32 erase);
INT32 release_work_buf(INT32 buf_num);

CHAR *workBufData(int buf_num);
int get_app_srvr_portal(void);

INT32
iAmYod(void);
INT32
iAmFyod(void);

#endif
