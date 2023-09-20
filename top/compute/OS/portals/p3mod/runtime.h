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
** $Id: runtime.h,v 1.8 2001/08/22 16:51:31 pumatst Exp $
** Portals 3.0 module file that provides runtime services to the P3 module
** Right now these are functions that call other functions in the old
** Portals 2.0 module. Eventually, there should be a seperate Cplant module
** that deals with process allocation, memory mapping, etc. When that
** comes along, all we have to change in the P3 module is contained in
** this file.
*/

#ifndef P3_RUNTIME_H
#define P3_RUNTIME_H

int checktask(void);
int getpid(void);
int getnum_nodes(void);
int getgid(void);
int getnid(void);
int getrank(void);
int getTskIndex(long spid);
void *getTsk(int ppid);
int p3rank2pnid(int nid);
int p3rank2ppid(int nid);
int memcpy2user(void *task, void *to, void *from, size_t len);

#endif /* P3_RUNTIME_H */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
