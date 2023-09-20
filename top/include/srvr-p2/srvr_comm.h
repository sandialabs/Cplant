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
** $Id: srvr_comm.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COMM_H
#define SRVR_COMM_H

#include "puma_errno.h"
#include "portal.h"
#include "libtrap.h"

#define SRVR_USR_DATA_LEN   128

typedef struct {
    /*
    ** alpha Linux: we want size to be 8 byte multiple
    **                      gcc/alpha alignments
    */
    INT32 msg_type;              /* 4 */

    int ret_ptl;        /* 4 */
    UINT32       req_len;        /* 8 */
    CHAMELEON mbits;          /* 8 */

    CHAR  user_data[SRVR_USR_DATA_LEN];

}data_xfer_msg;

#define MKCHAMELEON(cham, ul) {                  \
    cham.ints.i0 = (ul >> 32);                   \
    cham.ints.i1 = (ul & 0x00000000ffffffff);    \
}
#define UNMKCHAMELEON(ul, cham) { \
    ul = ((unsigned long)cham.ints.i1 | ((unsigned long)cham.ints.i0 << 32)); \
}

#ifndef PTL_DESC
#define PTL_DESC(ptl)     (_my_pcb->portal_table2[ptl])
#endif
 
#define IND_MD(ptl)   PTL_DESC(ptl).mem_desc.ind
#define SING_MD(ptl)  PTL_DESC(ptl).mem_desc.single

#define MATCH_MD(ptl) PTL_DESC(ptl).mem_desc.match
#define MLE(ptl, mle) MATCH_MD(ptl).u_lst[mle]

#define NEXT_MLE(ptl,mle)  (MLE(ptl,mle).next)
#define PREV_MLE(ptl,mle)  (MLE(ptl,mle).pad16[0])

#define DEACTIVATE_MLE(ptl, mle)   (MLE(ptl,mle).ctl_bits |= MCH_NOT_ACTIVE)
#define ACTIVATE_MLE(ptl, mle) (MLE(ptl,mle).ctl_bits &= ~MCH_NOT_ACTIVE)

#define MLE_INACTIVE(ptl, mle)  (MLE(ptl, mle).ctl_bits & MCH_NOT_ACTIVE)
#define MLE_ACTIVE(ptl, mle)    (~(MLE_INACTIVE(ptl,mle))

#define BLOCKING     (1)
#define NONBLOCKING  (0)

#define SRVR_INVAL_NID (-1)
#define SRVR_INVAL_PID (0)
#define SRVR_INVAL_PTL (0xff)

#define MINUSERPTL  0
#define MAXUSERPTL  63

extern INT32 SrvrDbg;

extern int server_library_init();
extern void server_library_done();

#include "srvr_comm_data.h"
#include "srvr_comm_ctl.h"
#include "srvr_comm_get.h"
#include "srvr_comm_put.h"
#include "srvr_coll_comm.h"

#endif
