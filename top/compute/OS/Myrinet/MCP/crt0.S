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
! $Id: crt0.S,v 1.4 2000/11/06 21:46:55 rolf Exp $
! Startup code for the MCP

#include "MCPSHMEM_ADDR.h"

.text
.global start
start:
	bt real_start
	nop
	nop
	nop

! We want mcpshmem to be at a well known location: put it at address 16.
! If this location changes you must adjust MCPSHMEM_ADDR in util/common.c
! and in rtscts/Pkt_handler.c
!
	.globl _mcpshmem
	.align 4
	.set _mcpshmem, MCPSHMEM_ADDR

real_start:
	ld     [_mcp_stack],%sp	! setup stack pointer
	ld     [_mcp_stack],%fp	! setup frame pointer

	! call main() in lanai3/lanai7-compatible way
        mov     1f,%rca
        bt      _main
        st      %rca,[--%sp]
        nop

	! trap in infinite loop if main ever returns
1:
        bt      .
        nop
        nop                     ! for lanai7 compatibility

