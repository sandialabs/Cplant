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
** $Id: pct_start.h,v 1.21 2001/07/24 22:02:35 lafisk Exp $
*/

#ifndef PCT_START_H
#define PCT_START_H

#include "puma_errno.h"
/*
** definitions used by pct and child process startup
*/

#define CHILD_GO_MAIN_TOKEN   0xceed    /* proceed to main */
#define CHILD_IS_READY_TOKEN  0x6ede
#define CHILD_NACK_TOKEN      0xdead

#define CHILD_PID_MAP_REQUEST   (1)
#define CHILD_NID_MAP_REQUEST   (2)
#define CHILD_FYOD_MAP_REQUEST  (3)
#define CHILD_FILEIO_REQUEST    (4)
#define CHILD_PORTAL_ID         (5)
#define CHILD_BEBOPD_ID_REQUEST (6)
#define CHILD_FIRST_REQUEST     CHILD_PID_MAP_REQUEST
#define CHILD_LAST_REQUEST      CHILD_BEBOPD_ID_REQUEST

/* these are associated w/ the string array start_errors[] 
   in startup.c
*/

#define ERR_START_OK			0
#define ERR_START_MLOCKALL              1
#define ERR_START_SYS_CTL_PTL		2
#define ERR_START_SYNC_CTL_PTL		3
#define ERR_START_WRITE_PTL		4
#define ERR_START_READ_PTL		5
#define ERR_START_ACK_PTL		6
#define ERR_START_FILEIO		7
#define ERR_START_READY			8
#define ERR_START_GOMAIN		9
#define ERR_START_SYNC			10
#define ERR_START_PREMAIN		11
#define ERR_START_NID_MAP		12
#define ERR_START_PID_MAP		13
#define ERR_START_FYOD_MAP		14
#define ERR_START_CTL_PTL		15
#define ERR_START_DATA_PTL		16
#define ERR_START_ENVIRONMENT           17
#define ERR_START_PPID                  18
#define ERR_OPEN_CPLANT_DEVICE_FILES    19
#define ERR_START_REDIRECT              20
#define ERR_START_IO_PTL                21
#define ERR_START_P3                    22  
#define ERR_START_SRVRLIB               23
#define ERR_START_LAST_MSG       ERR_START_SRVRLIB

#define start_error_string(n) \
   TYPE_TO_STRING(0, ERR_START_LAST_MSG, n, start_errors)

#define MEM_DESC(ptl)   (_my_pcb->portal_table2[ptl].mem_desc.single)

/*
** First field of all messages sent from app to pct must be
** the local ID assigned to app process by the pct. (pct_req_id)
*/
typedef struct _child_ready{
    int ctl_ptl;
} child_ready_msg;

typedef struct _child_nack{
    int lerrno;
    int CPerrno;
    int start_log;
} child_nack_msg;

typedef struct _out_of_band{
    int int1;
    int int2;
    int int3;
    int int4;
    void *ptr1;
    void *ptr2;
} out_of_band_msg;

extern const char *start_errors[];   /* defined in startup.c */

#endif /* PCT_START_H */
