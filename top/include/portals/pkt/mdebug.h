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
** $Id: mdebug.h,v 1.3 1998/09/14 23:55:28 jotto Exp $
** Mark Sears debug module. Basically a bunch of counters. Made them
** visible through /proc/pktmod
*/
#ifdef MDEBUG		/* Only do this if you want the counters included */

#ifndef MDEBUG_H
#define MDEBUG_H

#ifdef MDEBUG_PLUSPLUS
/* see comment below -- this flag lives in a device 
   driver/module; it is toggled using the device's ioctl
   (keeps us out of the kernel proper) */
extern int mdebug_flag;
#endif

typedef enum   {
    PDD_NSEND= 0,	/* number of sends */
    PDD_LSEND,		/* length of sends */
    PDD_NRECV,		/* number of recvs */
    PDD_LRECV,		/* length of recvs */
    PDD_NFAILS,		/* number of failed sends */
    PDD_CANTS,		/* number of cant sends */
    PDD_NRETRAN,	/* number of retransmits */

    PDD_IGNACK,		/* ignored acks */
    PDD_NOUTSEQ,	/* number of out of sequence messages */
    PDD_NDROP1,		/* number of dropped messages (stream busy) */
    PDD_NDROP2,		/* number of dropped messages (incorrect seq num) */
    PDD_NDROP3,		/* number of dropped messages (invalid process) */
    PDD_NSTALL,		/* number of stalls */
    PDD_UNSTALL,	/* number of unstalls */
    PDD_RSTALL,		/* number of msgs rcvd in stalled state */

    PDD_NTRECV,		/* number of truly rcvd packets */
    PDD_NPUTUSR2,	/* number of calls to put_user2 (ptlPktRecvBody) */
    PDD_LPUTUSR2,	/* length of data for put_user2 */

    PDD_NTSEND,		/* number of portal layer sends */
    PDD_LTSEND,		/* length of portal layer sends */
    PDD_MTU,		/* device mtu */
    PDD_MAXSEND,	/* max send size */
    PDD_MAXNPKTS,	/* max send numPkts */
    PDD_NSENDPKTS,	/* total send packets */

    PDD_NOTDONE,	/* packet arrived when ptlPktHandler busy */
    PDD_LONGMSG,	/* packet num != numpkts  */
    PDD_WORKING,	/* num of working packets */

    PDD_AUX1,           /* auxiliary location 1 */
    PDD_AUX2,           /* auxiliary location 2 */

    PDD_LAST		/* Sentinel for the list */
} mdebug_t;

typedef struct {
    long cnt;
    char *name;
} mdebug_entry_t;


/* The array that contains the counters */
mdebug_entry_t mdebug_list[];


/* The function that gets called when someone read /proc/pktmod */
extern int pktModProc(char *buf, char **start, off_t off, int len, int unused);


/*
** The functions to update the counters
*/
static __inline__ void
mdebugSet(mdebug_t i, int j)
{
#ifdef MDEBUG_PLUSPLUS
/* add this test if you want to turn instrumentation 
   on and off selectively from a user app */
   if (mdebug_flag)
#endif
     mdebug_list[i].cnt = j;
}

static __inline__ void
mdebugAdd(mdebug_t i, int j)
{
#ifdef MDEBUG_PLUSPLUS
  if (mdebug_flag)
#endif
     mdebug_list[i].cnt += j;
}

static __inline__ void
mdebugMax(mdebug_t i, int j)
{
#ifdef MDEBUG_PLUSPLUS
  if (mdebug_flag) 
#endif
    if (j > mdebug_list[i].cnt) {
	mdebug_list[i].cnt = j;
    }
}

static __inline__ void
mdebugInc(mdebug_t i)
{
#ifdef MDEBUG_PLUSPLUS
if (mdebug_flag)
#endif
     mdebug_list[i].cnt++;
}

#endif MDEBUG_H

#endif MDEBUG
