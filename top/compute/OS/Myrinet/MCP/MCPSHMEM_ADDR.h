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
** $Id: MCPSHMEM_ADDR.h,v 1.2 2002/02/06 00:58:46 pumatst Exp $
**
** Both the MCP and the host need to know where mcpshmem is. The C
** compiler places it for us, so we never really know where to look
** for it. We reserve a 4-byte spot at a well known address in LANai
** memory to hold a 32-bit pointer to the beginning of mcpshmem.
** During initialization, the MCP grabs the actuall address of mcpshmem
** and stores it in that well known location. The host can then go
** there and look it up.
** MCPSHMEM_ADDR defines where the well known lcoation should be.
** (See crt0.S to see how it is reserved.)
*/

#ifndef MCPSHMEM_ADDR_H
#define MCPSHMEM_ADDR_H
    #define MCPSHMEM_ADDR	(16)
#endif /* MCPSHMEM_ADDR_H */
