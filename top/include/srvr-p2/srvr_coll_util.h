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
/* $Id: srvr_coll_util.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_COLL_UTIL_H
#define SRVR_COLL_UTIL_H
 
extern int *dsrvr_ballot;
extern int dsrvr_invalVote;

#define MAX_FANOUT_DEGREE                8
#define TREE_PARENT(rank,degree)        ( (int)((rank) - 1) / degree)
#define TREE_CHILD(rank,degree,which)   ( ((rank) * degree) + 1 + which)

#define DSRVR_VOTE_VERIFY(rank)  \
        (dsrvr_ballot && (rank >= 0) && (rank < dsrvrMaxMembers))
 
#define DSRVR_VOTE_VALUE(rank)  \
  (DSRVR_VOTE_VERIFY(rank) ? (dsrvr_ballot)[((rank)*2)+1] : dsrvr_invalVote)
 
int dsrvr_vote(int voteVal, double tmout, int type, int *list, int listLen);
int dsrvr_system_synchronization(int fanout_degree, int msgtype, 
                                 double timeout);

#endif

