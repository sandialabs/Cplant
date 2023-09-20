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
** $Id: tvdsvr.h,v 1.3.4.1 2002/04/23 14:47:50 galagun Exp $
*/

/* definitions for TotalView debug server requests */

#ifndef TVDSVR_H
#define TVDSVR_H

#define TVDSVR_LOC "/cplant/toolworks/totalview.5.cplant/bin/tvdsvr"

#define TVDSVR_REQUEST_YOD_EXEC  0
#define TVDSVR_REQUEST_PCT_EXEC  1
#define TVDSVR_REQUEST_YOD_KILL  2
#define TVDSVR_REQUEST_PCT_KILL  3

#define TVDSVR_ACK    1
#define TVDSVR_NACK   2

#endif

