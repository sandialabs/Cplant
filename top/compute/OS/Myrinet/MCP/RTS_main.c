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
** $Id: RTS_main.c,v 1.3 1999/05/20 18:01:45 rolf Exp $
** Start of the Pakcet Myrinet Control Program (MCP)
*/

#include "init.h"
#include "Pkt_statem.h"
#include "MCPshmem.h"

/******************************************************************************/

int
main(void)
{

    MCP_init(MCP_TYPE_RTSCTS);
    pkt_state_machine();

    /* We never come back from the state machine. Make cc happy */
    return 0;

}  /* end of main() */
