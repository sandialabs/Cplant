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
**   $Id: yod_log.c,v 1.11.2.1 2002/06/28 17:05:17 jrstear Exp $
**
**   log user activity to a log file when yod is done
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "puma.h"
#include "config.h"
#include "yod_data.h"
 
#define USERLOG_OK  0
#define USERLOG_LOCKFILE 1
#define USERLOG_TIMEOUT  2
#define USERLOG_NOFILE   3
#define USERLOG_ERROR    4
#define USERLOG_NESTED_LOCK 5

static int getLock(const char *fname, double trySeconds);
extern double dclock(void);

#define LOGBUFSIZE 1024

static char tmpbuf[LOGBUFSIZE],
            log_error_msg[LOGBUFSIZE],
            log_status_msg[LOGBUFSIZE],
            logbuf[LOGBUFSIZE];

static char mylock[MAXPATHLEN],
            rlock[MAXPATHLEN],
            uniqname[64];

static int  lockFile=0;
static char log_done=0;
static char newline = '\n';

#define TRYLOCK() \
    if (!lockFile && mylock[0] && rlock[0]) \
       { link(rlock, mylock); lockFile=1; }

#define FREELOCK() \
    if (lockFile && mylock[0]) \
       { unlink(mylock); lockFile = 0;    }

static void
add_to_log_msg(char *msg, char *add)
{
int len;

    if ((len = strlen(msg))){
	strncat(msg, ", ", LOGBUFSIZE -1 -len);
	len = strlen(msg);
    }
    strncat(msg, add, LOGBUFSIZE - 1 - len);
}

void
add_to_log_status(const char *fmt, ...)
{
va_list ap;

    va_start(ap, fmt);
    vsnprintf(tmpbuf, LOGBUFSIZE-1, fmt, ap);
    va_end(ap);

    add_to_log_msg(log_status_msg, tmpbuf);
}

void
add_to_log_error(const char *fmt, ...)
{
va_list ap;

    va_start(ap, fmt);
    vsnprintf(tmpbuf, LOGBUFSIZE-1, fmt, ap);
    va_end(ap);

    add_to_log_msg(log_error_msg, tmpbuf);
}

void
log_user(unsigned int euid, time_t start, time_t end, 
	 int nnodes, char *ndlist, char *fname)
{
struct passwd *pw;
int fd, rc, hr, min, sec, len, elapsed;
char stime[30], etime[30];
struct tm *tdata;
const char *lname;

    if (log_done) return;

    lname = log_file_name();

    if (!lname){
        log_done = 1;
        return;
    }
    
    pw = getpwuid(euid);

    elapsed = (int)difftime(end, start);

    hr =  elapsed / 3600;

    min = ((elapsed % 3600) / 60);

    sec = elapsed % 60;

    tdata = localtime(&start);

    strftime(stime, 29, "%m/%d/%Y %H:%M:%S", tdata);

    tdata = localtime(&end);

    strftime(etime, 29, "%m/%d/%Y %H:%M:%S", tdata);

    snprintf(logbuf, LOGBUFSIZE-1, "%s %s %02d:%02d:%02d %04d %s %s",
         stime, etime,
         hr, min, sec, nnodes,
         ((pw && pw->pw_name) ? pw->pw_name : "user unknown"),
         fname);

    if (log_status_msg[0]){

	add_to_log_msg(logbuf,log_status_msg);
    }

    if (ndlist){
        add_to_log_msg(logbuf,ndlist);
    }

    if (log_error_msg[0]){  /* display node list if there were errors */

	add_to_log_msg(logbuf,log_error_msg);

    }
    len = strlen(logbuf);

#ifdef __i386__
    rc = getLock(lname,  0.0);
#else
    rc = getLock(lname, 30.0);
#endif

    if (rc != USERLOG_OK){

	if (rc == USERLOG_TIMEOUT){

            /*
	    ** Can't get user log file.  For now write to a user log
	    ** file owned by me with name appended by my user ID.
	    */
	    snprintf(tmpbuf, LOGBUFSIZE-1, "%s.%d", lname, euid);

	    rc = getLock(tmpbuf, 30.0);
            if (rc != USERLOG_OK){
	        yoderrmsg("Unable to obtain user log file %s\n",tmpbuf);
            }
	    else{
		yodmsg(
		"Please notify system adminstration of user log problem.\n");
		lname = tmpbuf;
	    }
	     
	}
	else if (rc == USERLOG_NESTED_LOCK){
	    /*
	    ** Maybe yod took a signal while writing log file.
	    ** Let's remove lock and return.
	    */
	    FREELOCK();
	}
	else if (rc == USERLOG_NOFILE){
	    yoderrmsg("User log file is missing!!\n");
	    yoderrmsg("Please tell system administration to create it.\n");
	}
	else if (rc == USERLOG_LOCKFILE){
	    yoderrmsg("Unable to create lock file to lock user log.\n");
	    yoderrmsg("Please notify system administration.\n");
	}
	else{
            yoderrmsg("Unable to obtain user log file %s\n",lname);
	}
    }
    if (rc != USERLOG_OK){
	log_done = 1;
        return;
    }

    fd = open(lname, O_WRONLY|O_CREAT);

    if (fd < 0){
        yoderrmsg("Unable to open user log file to write\n");
    }
    else{
 
        rc = lseek(fd, 0, SEEK_END);
 
        if (rc < 0){
            yoderrmsg("Unable to seek to end of user log file\n");
        }
        else{
            rc = write(fd, (const void *)logbuf, len);

	    if (rc == len) {
                rc = write(fd, (const void *)&newline, 1);
            }
 
            if (rc < 0){
                yoderrmsg("Unable to write to user log file\n");
            }
#ifdef _POSIX_SYNCHRONIZED_IO
            else{
	        rc = fdatasync(fd);

	        if (rc < 0){
                    yoderrmsg("Unable to flush to the user log file\n");
	        }
	    }
#endif
        }
    }
 
    if (fd >= 0){
        close(fd);
        /* make sure the log is perm 666 */
        chmod(lname, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    }

    FREELOCK();

    log_msg(logbuf); /* send a copy to syslog */

    log_done = 1;
}
/*
** Get an exclusive lock on a file in a directory on a remote file
** system.  Linux man pages warn flock() won't work due to NFS 
** peculiarities.
**
** Arguments:
        where       the file to get exclusive access to
        trySeconds  how many seconds we wait for the lock
**
** Warning: getLock/freeLock can't be nested.  You have to free a
**   lock on a file before obtaining the next one.  
*/
static int
getLock(const char *where, double trySeconds)
{
int fd, rc, status, backoff;
char *c;
double inittime;
struct stat sbuf;

    if (lockFile){ /* simple getLock only allows one lock at a time */
        return USERLOG_NESTED_LOCK;   
    }

    if (uniqname[0] == 0){
        snprintf(uniqname,63,".nid%04d.%d.LOCK",_my_pnid, _my_pid);
    }
    /*
    ** File we wish exclusive access to
    */
    c = realpath(where, rlock);

    if (c == NULL){
	/*
	** File to lock doesn't exist.  See if I can open it to write.
	*/
	if (errno == ENOENT){  
            fd = open(where, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

            if (fd < 0){
                return USERLOG_NOFILE;
            }
            close(fd);
	}
	else{
           return USERLOG_ERROR;
        }
    }

    /*
    ** Root lock file - all users create link with this, create
    ** it if it's not already out there.
    */
    strncat(rlock, ".LOCK", MAXPATHLEN-1);
    rc = stat(rlock, &sbuf);

    if (rc < 0){
        fd = open(rlock, O_RDONLY | O_CREAT, S_IRUSR|S_IRGRP|S_IROTH);

        if (fd < 0){
            return USERLOG_LOCKFILE;
        }
        close(fd);
    }

    /*
    ** My lock file - has unique name, create a link with root lock
    */
    strcpy(mylock, rlock);
    strncat(mylock, uniqname, MAXPATHLEN-1-strlen(uniqname));

    /*
    ** If mylock already exists, get rid of it.
    */
    if (!stat(mylock, &sbuf)){
        unlink(mylock);
    }

    /*
    ** Stat it.  If "links" is 2, I'm the only one with such
    ** a link.  I have the lock.  If it's higher, someone's
    ** fighting me for it.  Delete my personal lock and try
    ** again.  (It's possible none of the "fighters" get 2.
    ** Having some back off a bit increases the chance of
    ** someone getting the lock.)
    */
    inittime=dclock();

    status = USERLOG_OK;

    backoff = _my_pid % 3;

    while (1){

        TRYLOCK();

        rc = stat(mylock, &sbuf);

        if (rc < 0){
	    FREELOCK();
            status = USERLOG_ERROR;
            break;
        }

        if (sbuf.st_nlink == 2){   /* I got it! */
            break;
        }
        if ((dclock() - inittime) > trySeconds){
	    FREELOCK();
            status = USERLOG_TIMEOUT;
            break;
        }
	FREELOCK();

	sleep(backoff); /* works better if some back off */ 
    }

    return status;

}
