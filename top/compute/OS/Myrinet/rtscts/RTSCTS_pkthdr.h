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
** $Id: RTSCTS_pkthdr.h,v 1.2 2002/02/26 23:15:52 jbogden Exp $
*/

#ifndef PKTHDR_H
#define PKTHDR_H

typedef struct   {
    unsigned int version;	/* Make sure we all speak the same protocol */
    short type;
    unsigned short src_nid;	/* Where this packet came from */
    unsigned int msgID;		/* Each msg is tagged with a unique ID*/
    int len;                    /* Length of data + pkthdr */

    short len1;
    short len2;

    short type2;		/* second type copy */
    unsigned short src_nid2;	/* second src_nid copy */
    unsigned int msgID2;	/* second msgID copy */

    unsigned int seq;		/* sequence # for each msg; RTS = 0 */
    unsigned int info;		/* pkt type specific info; e.g. len */
    unsigned int info2;		/* more packet type specific info */
    unsigned int msgNum;	/* Message number between two nodes */

    short type3;		/* third type copy */
    unsigned short src_nid3;	/* third src_nid copy */
    unsigned int msgID3;	/* third msgID copy */
    
    unsigned int pad1;      /* pad to 64-bit boundary */
} pkthdr_t;

#endif /* PKTHDR_H */
