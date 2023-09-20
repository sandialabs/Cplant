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
 * $Id: msgid.c,v 1.1.2.2 2002/05/22 20:01:22 jbogden Exp $
 *
 * A simple utility to parse out the fields in the RTSCTS msgIDs.
*/

#include <stdlib.h>
#include <stdio.h>
#include "RTSCTS_protocol.h"

int main(int argc, char **argv)
{

    unsigned int msgid;
    unsigned int nodeid;
    unsigned int seqnum;

    if (argc != 2)
    {
        printf("USAGE: msgid <MSGID>\n");
    }
    else
    {
        msgid = strtoul(argv[1],NULL,16);

        nodeid = (msgid >> msgID_SHIFT) - 1;
        seqnum = (msgID_MASK & msgid);
        printf("msgid=0x%08x  nodeid = %d (0x%04x)\n",msgid,nodeid,nodeid);
        printf("seq num = %d (0x%08x)\n",seqnum,seqnum);
    }
    
    return 1;
}
