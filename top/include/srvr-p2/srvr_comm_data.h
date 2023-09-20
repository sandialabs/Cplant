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
** $Id: srvr_comm_data.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COMM_DATA_H
#define SRVR_COMM_DATA_H

#include "puma.h"
#include "portal.h"

/*
** two types of data buffers
*/
#define READ_BUFFER      (1)
#define WRITE_BUFFER     (2)
 
int srvr_init_data_ptl(INT32 max_num_bufs);
INT32 srvr_release_data_ptl(int ptl);
INT32 srvr_add_data_buf(VOID *buf, INT32 len, BOOLEAN type, int ptl,
             unsigned long ign_mbits, unsigned long must_mbits);
INT32 srvr_test_write_buf(int ptl, INT32 mle);
INT32 srvr_test_read_buf(int ptl, INT32 mle, INT32 count);
INT32 srvr_delete_buf(int ptl, INT32 mle);

#endif
