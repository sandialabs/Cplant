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

#include <stdio.h>
#include <signal.h>
#include "puma.h"
#include "rpc_msgs.h"

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

/*
** $Id: nodekill.c,v 1.4 2001/04/01 01:30:59 pumatst Exp $
**
**    This is not really an IO operation, but it uses the IO
**    server to dispatch the kill request to the other nodes.
*/

void
nodekill(int node, int pid ATTR_UNUSED, int sig)
{
    hostCmd_t   cmd;

    /*
    ** only action possible right now is to send a SIGKILL to all nodes,
    ** and this is implemented through the host right now.
    */
    if(node != -1) {
        fprintf(stderr,"nodespec other than -1 unsupported in nodekill()");
        exit(-1);
    } else {
        if( sig != SIGKILL) {
            fprintf(stderr,
                "only signal currently supported in nodekill() is SIGKILL");
            exit(-1);
         }
    }
    if (  _host_rpc(&cmd, NULL,
        CMD_MASS_MURDER, 
	(int)_yod_cmd_nid, 
	(int)_yod_cmd_pid, 
	(int)_yod_cmd_ptl, 
	NULL, 0, NULL, 0) != 0 ) {

        fprintf(stderr,"nodekill() failed\n");
        return;
    } else {
        while(1); /* twiddle your thumbs until death */
    }
}

