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
#ifndef CGDB_H
#define CGDB_H

#define CGDB_REQUEST_GDB     0
#define CGDB_REQUEST_BT      1
#define CGDB_REQUEST_PROXY   2

#define GDBWRAP_CGDB_SEND_BT              0
#define GDBWRAP_CGDB_NO_GDB               1
#define GDBWRAP_CGDB_CANT_SIGNAL_GDB      2
#define GDBWRAP_CGDB_GET_BT_FROM_GDB      3
#define GDBWRAP_CGDB_GDB_DONE             4
#define GDBWRAP_CGDB_GDB_START_FAILED     5

#define PCT_CGDB_START_OK                 0
#define PCT_CGDB_START_FAILED             1

#endif
