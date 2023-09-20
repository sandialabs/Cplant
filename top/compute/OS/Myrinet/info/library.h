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
** $Id: library.h,v 1.2 2001/04/26 20:39:02 rolf Exp $
**
** This is a simple library that provides common functions to deal with
** rtscts info requests.
*/

#ifndef INFO_LIBRARY_H
#define INFO_LIBRARY_H

#include "RTSCTS_info.h"	/* For seg_info_t */

    int info_init(char *pname, int verbose);
    int info_get_data(char *pname, int fd, int verbose, int clear, int node,
	    seg_info_t segment, void *buf);

#endif INFO_LIBRARY_H
