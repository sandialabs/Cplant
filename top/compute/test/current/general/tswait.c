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
** $Id: tswait.c,v 1.2 2000/04/13 07:24:10 jsotto Exp $
**
** A test program that occupies the node for the number of seconds
** specified on it's command line.
**
** If no number of seconds is specified, it spins forever in a loop.
*/

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc > 1){
	sleep(atoi(argv[1]));
    }
    else{
	while(1);
    }
    return 0;    
}
