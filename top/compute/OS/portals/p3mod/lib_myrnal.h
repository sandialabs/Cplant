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
** $Id: lib_myrnal.h,v 1.4 2001/08/22 16:51:31 pumatst Exp $
*/

#ifndef LIB_MYRNAL_H
#define LIB_MYRNAL_H

#include <p30/lib-nal.h>	/* For nal_cb_t */

int open_lib_myrnal(void);
void close_lib_myrnal(void);
void myrnal_up(void);
void myrnal_down(void);

#endif /* LIB_MYRNAL_H */
