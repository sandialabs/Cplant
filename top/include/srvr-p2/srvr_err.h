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
** $Id: srvr_err.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/
#ifndef SRVR_ERRH
#define SRVR_ERRH

#include <syslog.h>
#include <errno.h>

/*
** errno: set by system
** CPerrno: set by Cplant servers & utilities, defined in startup.c
*/
extern int CPerrno;
extern const char *CPstrerror(int n);

#define CLEAR_ERR  (errno = CPerrno = 0)
 
/*
** Every process wishing to log errors must call log_open with
** it's name.
*/
void log_open(const char *myidentity);
void log_reopen(const char *myidentity);

/*
** By default, log messages go to /var/log/cplant.  
**
**     log_to_stderr(1)  send messages to stderr too
**     log_to_stderr(0)  don't messages to stderr
**     log_to_file(1)    send messages to file
**     log_to_file(0)    don't messages to file
*/
void log_to_file(int on);
void log_to_stderr(int on);

/*
** The next two calls assume a system error occured.  They display 
** the message passed in, they display the errno message, and they 
** display the CPerrno message if CPerrno is non-zero.  
**
** log_warning() returns and log_error() exits.
*/
void log_warning(const char *fmt, ...);
void log_error(const char *fmt, ...);

/*
** The next two calls ignore errno.  They display the message passed
** in.  If CPerrno is non-zero, they display the CPerrno message,
** and then zero CPerrno.  
**
** log_msg returns and log_quit exits.
*/
void log_msg(const char *fmt, ...);
void log_quit(const char *fmt, ...);

#endif
