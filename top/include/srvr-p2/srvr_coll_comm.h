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
#ifndef DSRVRCOMMH
#define DSRVRCOMMH
 
#include "portal.h"
#include "collcore.h"
#include "vcoll.h"

int dsrvr_barrier(double tmout, int *list, int listLen);
int dsrvr_gather(char *data, int blklen, int nblks, double tmout, 
		 int type, int *list, int listLen);
int dsrvr_bcast(char *buf, int len, double tmout, 
		int type, int *list, int listLen);
int dsrvr_reduce(PUMA_OP op, char *data, int len, double tmout, 
		  int type, int *list, int listLen);
int dsrvr_comm_init(int nmembers, int maxRedBuf);

int dsrvr_failed_op(void);
char *dsrvr_who_failed(void);

void dsrvr_comm_reset(void);
int dsrvr_puma_init(int gid, CHAMELEON mbits, char *inbuf, char *outbuf,
                 int data_size, int buf_len, void *ustruct,
                 VSENDRECV_HANDLE *handle);
int dsrvr_puma_send(BOOLEAN flow_control, int nrecs, DATA_REC *recs,
                VSENDRECV_HANDLE *handle);
void dsrvr_new_coll_comm_buffer(char *buf, int len);
void dsrvr_free_coll_comm_buffer(char *buf, int len);
int dsrvr_puma_recv(PUMA_OP op, BOOLEAN flow_control, int nrecs, DATA_REC *recs,
                VSENDRECV_HANDLE *handle);
int dsrvr_puma_cleanup(VSENDRECV_HANDLE *handle);
int dsrvr_puma_no_cleanup(VSENDRECV_HANDLE *handle);
void resetPtl(int ptl);


#define DSRVR_BARRIER_PORTAL           30
#define DSRVR_GATHER_PORTAL            31
#define DSRVR_BROADCAST_PORTAL         32
#define DSRVR_REDUCE_PORTAL            33
#define DSRVR_HANDSHAKE_PORTAL         34

typedef struct{
    int ptl;
    int ping_ptl;
    char   pad[2];
    double tmout;
}dsrvrInfo;
 
#define PUMA_RECV_SEND_OK    1     /* "where" values */
#define PUMA_RECV_SINGLE_BLK 2
#define PUMA_RECV_IND_BLK    3
#define PUMA_SEND_DATA_BLIND  4 
#define PUMA_SEND_DATA        5 
#define PUMA_SEND_AWAIT_HANDSHAKE 6

#define DSRVR_DONT_PING    (MAX_PORTAL + 1)
 
#define DSRVR_OKTOGO         0x01010101
#define DSRVR_BARRIER_TYPE   0x00001111

#endif
