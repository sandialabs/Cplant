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
#ifndef WRAP_H
#define WRAP_H

#include "sys_limits.h"
#include "appload_msg.h"
#include "rpc_msgs.h"
#include "pct_start.h"
#include "srvr_comm.h"

extern double dclock(void);

extern const char *routine_name;
extern const char *routine_where;
extern const char *routine_name0;
extern const char *routine_where0;
extern const char *routine_name1;
extern const char *routine_where1;
#define LOCATION(a,b) {\
routine_name0=routine_name1;routine_where0=routine_where1; \
routine_name1=routine_name;routine_where1=routine_where; \
routine_name=a;routine_where=b;}

#endif /* WRAP_H */
