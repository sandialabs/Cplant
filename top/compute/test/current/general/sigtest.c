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
** $Id: sigtest.c,v 1.1 2000/08/14 22:40:21 lafisk Exp $
**
** We want a test that ignores SIGTERM, and one that terminates on
** SIGTERM.
**
** Arguments:
**
**    -ignore    ignores SIGTERM
**
**    default is to terminate on SIGTERM
*/
#include "stdio.h"
#include <signal.h>

static void
spin()
{
    while (1);
}
void
terminate(int sig)
{
    exit(0);
}

int main(int argc, char *argv[])
{
int ignore=0;

    if (argc > 1){
	if ( (argv[1][0] == '-') && (argv[1][1] == 'i')){
	    ignore=1;
	}
    }
    if (ignore){
        signal(SIGTERM, SIG_IGN);
    }
    else{
        signal(SIGTERM, terminate);
    }

    spin();
    
    return 0;
}
