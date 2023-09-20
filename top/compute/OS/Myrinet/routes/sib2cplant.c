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
/* Generate a cplant-map file for siberia -- actually, this prints to stdout,
   so redirect to cplant-map */

#include <stdio.h>
#include <string.h>

extern int sw2su[];

void gen_host_name_siberia(char *label, int sw, int port);

/*
*******************************************************************************
**  List of node names 
*******************************************************************************
*/
static char name[100];
static int numNames=0;
static int get_node_names(void);

int main(void) 
{
  get_node_names();
  return 0;
}

static int
get_node_names(void)
{
int sw, pnid, port;

	numNames = 640;
        for (pnid= 0; pnid < numNames; pnid++)   {
	    sw= pnid / 8;
	    port= pnid % 8;

            gen_host_name_siberia(name, sw, port);
            printf("%s\n", name);
        }
        return 0;
}
