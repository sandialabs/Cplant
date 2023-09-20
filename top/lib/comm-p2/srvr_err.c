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
** $Id: srvr_err.c,v 1.1 2000/11/04 03:56:32 lafisk Exp $
**
** Error logging routines for Cplant daemons
*/

/*
** These routines are mostly taken from Appendix B of Stevens Advanced
** Programming... book.
*/
#define _GNU_SOURCE  /* to get basename() from string.h */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "srvr_err.h"

#define LOG_TO_FILE    (1)
#define LOG_TO_STDERR  (2)

/*
** set facility local7 to go to /var/log/cplant in your /etc/syslog.conf
*/
 
#define LOG_CPLANT  LOG_LOCAL7
 
#define SYS_ERRNO     (1<<1)
#define CP_ERRNO      (1<<2)

static int log_dest = 0x00000000;

static void
log_doit(int errnoflag, int priority, const char *fmt, va_list ap)
{
int errno_save;
char buf[1024];    

    errno_save = errno;

    if (!log_dest){        /* probably called before main */
        log_open(basename(getenv("_")));
        log_dest |= LOG_TO_STDERR;
    }

    vsprintf(buf, fmt, ap);

    if (errnoflag&SYS_ERRNO){       /* a system call set errno */

        sprintf(buf + strlen(buf), " [%s]", strerror(errno_save));
    }

    if (errnoflag&CP_ERRNO){       /* we set CPerrno */

        sprintf(buf + strlen(buf), " (%s)", CPstrerror(CPerrno));
    }

    strcat(buf, "\n");

    if (log_dest & LOG_TO_STDERR){
        fflush(stdout);
        fputs(buf,stderr);
	fprintf(stderr,"\n");
    }
    if (log_dest & LOG_TO_FILE){
        syslog(priority, buf);
    }

    CLEAR_ERR;

    return;
}
void
log_open(const char *myidentity)
{
    openlog(myidentity,  LOG_PID, LOG_CPLANT);
    log_dest = LOG_TO_FILE;
    return;
}
void
log_reopen(const char *myidentity)
{
    closelog();
    openlog(myidentity,  LOG_PID, LOG_CPLANT);
    return;
}
void
log_to_file(int on)
{
    if (on){
        log_dest |= LOG_TO_FILE;
    }
    else{
        log_dest &= ~(LOG_TO_FILE);
    }
    return;
}
void
log_to_stderr(int on)
{
    if (on){
        log_dest |= LOG_TO_STDERR;
    }
    else{
        log_dest &= ~(LOG_TO_STDERR);
    }
    return;
}
void
log_warning(const char *fmt, ...)
{
va_list ap;
int errno_flag;

    if (log_dest){

        errno_flag = (errno ? SYS_ERRNO : 0);

        if (CPerrno != 0){
            errno_flag |= CP_ERRNO;
        }

        va_start(ap, fmt);
        log_doit(errno_flag, LOG_WARNING, fmt, ap);
        va_end(ap);
    }

    CPerrno = 0;

    return;
}
void
log_error(const char *fmt, ...)
{
va_list ap;
int errno_flag;

    if (log_dest){

        errno_flag = (errno ? SYS_ERRNO : 0);

        if (CPerrno != 0){
            errno_flag |= CP_ERRNO;
        }

        va_start(ap, fmt);
        log_doit(errno_flag, LOG_ERR, fmt, ap);
        va_end(ap);
    }

    exit(1);
}
void
log_msg(const char *fmt, ...)
{
va_list ap;
int errno_flag;

    if (log_dest){

        if (CPerrno != 0){
            errno_flag = CP_ERRNO;
        }
        else{
            errno_flag = 0;
        }

        va_start(ap, fmt);
        log_doit(errno_flag, LOG_WARNING, fmt, ap);
        va_end(ap);
    }

    CPerrno = 0;

    return;
}
void
log_quit(const char *fmt, ...)
{
va_list ap;
int errno_flag;

    if (log_dest){

        if (CPerrno != 0){
            errno_flag = CP_ERRNO;
        }
        else{
            errno_flag = 0;
        }

        va_start(ap, fmt);
        log_doit(errno_flag, LOG_ERR, fmt, ap);
        va_end(ap);
    }

    exit(1);
}
