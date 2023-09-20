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
/* $Id: yod_log.h,v 1.2 2001/11/02 17:25:30 lafisk Exp $ */

#ifndef YOD_LOGH
#define YOD_LOGH

/*
** When yod is done it logs user activity to a user log
** file.  It will add a parenthetical status message,
** and a parenthethical error message if need be.  If
** there's an error message, yod will automatically
** display the list of nodes the app ran on.
*/

/*
** add text to the status message that goes to the log file
*/
void add_to_log_status(const char *fmt, ...);
/*
** add text to the error message that goes to the log file
*/
void add_to_log_error(const char *fmt, ...);
/*
** Write the message to the user log.  yod will only write
** once to the log, subsequent calls to log_user will be
** ignored.
*/
void log_user(unsigned int euid, time_t start, time_t end,
	 int nnodes, char *ndlist, char *fname);

#endif

