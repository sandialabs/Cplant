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
** $Id: init.h,v 1.7 2002/02/14 18:38:12 jbogden Exp $
** Initialize the MCP
*/

#ifndef INIT_H
#define INIT_H


void flash(int num, int delay);
void MCP_init(int mcp_type);
void self_test(void);
void fault(int num,unsigned int param1,unsigned int param2) __attribute__ ((noreturn));
void warning(int num, unsigned int lastrcv, unsigned int lastsnd);

#endif /* INIT_H */
