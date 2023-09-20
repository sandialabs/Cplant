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
#ifndef SRVR_COLL_H
#define SRVR_COLL_H

/*************************************************************
**  Errors
*************************************************************/

#define DSRVR_OK              0
#define DSRVR_ERROR           (-11) /* invalid input */
#define DSRVR_RESOURCE_ERROR  (-12) /* internal resource error */
#define DSRVR_EXTERNAL_ERROR  (-13) /* external membership error*/

enum{
 NO_TYPE,
 GATHER_TYPE,
 BCAST_TYPE,
 REDUCE_TYPE,
 BARRIER_TYPE,
 SEND_TYPE,
 NUM_OP_TYPES
} SrvrOpType;

/*
** from srvr_coll_comm.c
*/

int server_coll_init(void);
void server_coll_done(void);
int srvr_reset_coll(void);   /* done then init */

int dsrvr_barrier(int tmout, int *list, int listLen);
int dsrvr_gather(char *data, int blklen, int nblks, int tmout,
                 int type, int *list, int listLen);
int dsrvr_bcast(char *buf, int len, int tmout, int type, 
               int *list, int listLen);

extern unsigned char dsrvr_bcast_cksum;
extern char dsrvr_do_cksum;

#include "srvr_coll_fail.h"
#include "srvr_coll_membership.h"
#include "srvr_coll_util.h"

#endif
