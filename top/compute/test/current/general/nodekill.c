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
** $Id: nodekill.c,v 1.1 2001/03/23 17:42:03 lafisk Exp $
**
** one optional argument "p".  All processes of rank "p" or
** greater will call nodekill.  If "p" is not set, all processes
** call nodekill.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

extern void nodekill(int, int, int);
extern int _my_rank;

main(int argc, char *argv[])
{
int killers;

    if (argc > 1){
        killers = atoi(argv[1]);
    }
    else{
        killers = 0;
    }
    
    sleep(10);

    if (_my_rank >= killers){
         printf("process %d is calling nodekill, bye\n",_my_rank);
         nodekill(-1, 0, SIGKILL);
    }

    printf("process %d is going to spin wait...\n",_my_rank);
    while(1);

}
