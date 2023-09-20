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
** $Id: ts_srvr.h,v 1.6 2001/04/01 23:53:11 pumatst Exp $
*/
#ifndef TS_H
#define TS_H


#include "srvr_err.h"
#include "srvr_comm.h"
#include "puma.h"
#include "ppid.h"

#define GID_TS_SRVR    99

#include <p30/errno.h>
#define DONE_SRVRLIB_TEST server_library_done()

#define INIT_SRVRLIB_TEST { \
   log_to_file(0);          \
   log_to_stderr(1);        \
   if (___proc_type == SERV_TYPE){                  \
       _my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_TS_SRVR, "ts_srvr"); \
       if ( _my_ppid == 0 ){                        \
           log_error("Can not register_ppid");      \
       }                                            \
       if (server_library_init()){                  \
	   log_error("Can't init server library");  \
       }                                            \
   }                                                \
}

#endif
