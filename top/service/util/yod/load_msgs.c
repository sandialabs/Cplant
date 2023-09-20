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
** $Id: load_msgs.c,v 1.34 2002/03/07 00:24:30 ktpedre Exp $
*/
#include <stdio.h>
#include <string.h>
#ifdef __linux__
#  if __i386__
#    include <signal.h>
#  else
#    if 0
#    define _SIGNAL_H        /* signum.h requires _SIGNAL_H defined */
#    include <signum.h>
#    undef _SIGNAL_H    
#    else
#    include <signal.h>
#    endif
#  endif
#endif

#ifdef __osf__
#include <signal.h>
#endif

#include "appload_msg.h"
#include "bebopd.h"
#include "config.h"

const char *pct_to_yod_strings[PCT_TO_YOD_LAST_MSG-PCT_TO_YOD_FIRST_MSG+1]=
{
"local pid message",
"app ready message",
"process done message",
"all done acknowledgement",
"launch failure message",
"executable will fit in RAM disk",
"executable will NOT fit in RAM disk",
"application portal ID",
"OK to load",
"try load again later",
"pct rejects load"
};

const char *launch_err_strings[LAST_LAUNCH_ERR-FIRST_LAUNCH_ERR+1]=
{
"no error",
"miscellaneous error",
"error requiring PCT to terminate, notify system administration",
"problem in load data from yod",
"pct can't generate temp file for user code",
"bad job ID",
"failure in fork of new process",
"failure in exec of new process",
"failure in wait for process termination",
"failure to create pipe for gdb",
"failure to fork pct for gdb",
"failure to locate gdb binary file, bad path?",
"failure to exec gdb binary file, bad permission?",
"failure to fork pct for gdbwrap",
"failure to locate gdbwrap binary file, bad path?",
"failure to exec gdbwrap binary file, bad permission?",
"error talking to app process",
"application process detected error before user code",
"application process terminated before user code",
"pct group communication failure",
"failure to reply to yod's request for pid map",
"executable path not found, or file not executable",
"error in protocol between yod and PCT",
"error in protocol between main PCT and app host",
"error in protocol between app host and app process",
"error in setting user, group or process group IDs before exec",
"executable file corrupted on the way to compute node",
"portals layer returned an error code"
};

const char *yod_to_pct_strings[YOD_TO_PCT_LAST_MSG-YOD_TO_PCT_FIRST_MSG+1]=
{
"initial load message",
"auxiliary load data",
"program name",
"program arguments",
"user environment",
"node number list",
"executable program",
"global pid list",
"go main message",
"app all done message",
"first abort load message",
"second abort load message",
"message to send user signal to app process",
"reset pct message",
"pid map request",
"executable program path",
"get app process stack trace",
"portal ids",
"group ids",
"strace options",
"yodless completion request",
"request to load",
"cancel request to load"
};

/*
** status contained in pct update to bebopd
*/
const char *pct_status_strings[LAST_PCT_STATUS+1]=
{
"no pct status",
"pct free",
"pct allocated",
"pct busy",
"pct down",
"no report from pct",
"pct trouble" 
};

/*
** PCT's child status bit map, map must fit in a UINT16
*/
const char *child_status_strings[20]=
{
"no status set",
"new job",
"job started",
"app proc requested pid map",
"app proc sent pid map",
"app proc requested nid map",
"app proc sent nid map",
"app proc requested fileio data",
"app proc sent fileio data",
"app proc ready to go to main",
"app proc sent to main",
"app process done", 
"app proc sent NACK in startup",
"pct sent SIGTERM to app proc",
"pct sent SIGKILL to app proc",
"app proc send the portal ID",
"app proc sent the fyod map",
"started nice kill of whole job"
};

/*
** Linux/AXP signal numbers/names
*/

#ifdef LINUX_PORTALS

#ifdef __linux__
const char *signal_names[_NSIG]= {
#endif
#ifdef __osf__
const char *signal_names[NSIG]= {
#endif
"NO_SIGNAL",
"SIGHUP",
"SIGINT",
"SIGQUIT",
"SIGILL", 
"SIGTRAP",
"SIGABRT",
"SIGEMT", 
"SIGFPE",
"SIGKILL",
"SIGBUS",
"SIGSEGV",
"SIGSYS",
"SIGPIPE",
"SIGALRM",
"SIGTERM",
"SIGURG",
"SIGSTOP",
"SIGTSTP",
"SIGCONT",
"SIGCHLD",
"SIGTTIN",
"SIGTTOU",
"SIGIO",  
"SIGXCPU",
"SIGXFSZ",
"SIGVTALRM",
"SIGPROF",
"SIGWINCH",
"SIGINFO",
"SIGUSR1",
"SIGUSR2"};

#endif

#if 0
void
print_node_name(int phys_nid, FILE *fp, int width)
{
int pd1, pd2, len, i, havenm, suNum;
char *nodeName;

    nodeName = phys2name(phys_nid);
    havenm = 1;

    if ( (suNum < 0) || (!nodeName)){
	len  =  7;
	havenm = 0;
    }
    else{
        len = 7 + strlen(nodeName);
    }

    pd1 = pd2 = 0;

    if (width){
	if (width > len){
	    pd1 = (width - len) / 2;
	    pd2 = width - pd1 - len;
	} 
    }
    for (i=0; i<pd1; i++) fprintf(fp, " ");
    if (havenm){
	fprintf(fp, "SU-%03d %s",suNum,nodeName);
    } 
    else{
	fprintf(fp, "unknown");
    }
    for (i=0; i<pd2; i++) fprintf(fp, " ");
}
#endif
