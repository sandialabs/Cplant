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
** $Id: infotest.c,v 1.1 2001/07/24 22:03:20 lafisk Exp $
**
**  This runs as a cplant server (as opposed to a cplant parallel app)
**  and tests the informational functions in libpuma.a
*/

#include <stdio.h>
#include "srvr/srvr_err.h"

/*
** these belong in a header file somewhere
*/

int get_bebopd_id(int *bnid, int *bpid, int *bptl, int newLookUp);


main()
{
int i, rc, newLookUp;
int bnid, bpid, bptl;

    log_to_file(0);
    log_to_stderr(1);

    for (i=0; i<10; i++){

        newLookUp = (i & 1);

	rc = get_bebopd_id(&bnid, &bpid, &bptl, newLookUp);

	if (rc == 0){

	    printf("Bebopd ID (%s): nid %d, pid %d, ptl %d\n",
	             (newLookUp ? "new look up" : "reply from cached info"),
		     bnid, bpid, bptl);
	}
	else{
	    log_warning("get_bebopd_id");
	}
    }

}
