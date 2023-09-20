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
** $Id: route_util.h,v 1.2 2001/08/22 16:45:15 pumatst Exp $
** Utilities to handle routing data.
** Make sure to include #include "../MCP/MCPshmem.h" before this file for
** mcpshmem_t.
*/

#ifndef ROUTE_UTIL_H
#define ROUTE_UTIL_H

char *read_route(FILE *route_fp, int verbose);
void dnld_route(mcpshmem_t *mcpshmem, char *route, int verbose);

#endif /* ROUTE_UTIL_H */
