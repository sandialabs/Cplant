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
**   $Id: yod2_log.c,v 1.3 2001/11/24 23:33:37 lafisk Exp $
**
** a copy of yod_log.c that can manage the log record for 
** more than one job
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
#include "yod2.h"
#include "job.h"
 
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
            ndlist[LOGBUFSIZE],
            logbuf[LOGBUFSIZE];

static char mylock[MAXPATHLEN],
            rlock[MAXPATHLEN],
            uniqname[64];

static int  lockFile=0;
static char newline = '\n';

#define TRYLOCK() \
    if (!lockFile && mylock[0] && rlock[0]) \
       { link(rlock, mylock); lockFile=1; }

#define FREELOCK() \
    if (lockFile && mylock[0]) \
       { unlink(mylock); lockFile = 0;    }

void
add_to_log_string(char **str, const char *fmt, ...)
{
va_list ap;
int len1, len2;
char *c;

    va_start(ap, fmt);
    vsnprintf(tmpbuf, LOGBUFSIZE-1, fmt, ap);
    va_end(ap);

    len1 = (*str ? strlen(*str) : 0);

    len2 = strlen(tmpbuf);

    *str = (char *)realloc(*str, len1 + len2 + 3);

    if (!*str) return;

    c = *str + len1;

    if (len1){
        sprintf(c, ", %s",tmpbuf);
    }
    else{
        strcpy(c, tmpbuf);
    }
}

void
log_user(jobTree *job)
{
struct passwd *pw;
int fd, rc, hr, min, sec, len, elapsed;
int len1, len2, len3, len4;
char stime[30], etime[30];
struct tm *tdata;
const char *lname;
unsigned int euid; 
time_t start, end;
int nprocs; 
char *fname;

    if (job->log_done) return;

    lname = log_file_name();

    if (!lname){
        job->log_done = 1;
        return;
    }

    euid   = geteuid();
    start  = job->startRun.tm;
    end    = job->endTime.tm;
    nprocs = job->nprocs;
    fname  = jobInfo_fileName(job->handle);

    if (start == 0){
        start = job->startLoad.tm;  /* load never completed */
        end = start;
    }
    else if (end == 0){
        end = time(NULL);
    }

    write_node_list(job->nodeList, job->nprocs, ndlist, LOGBUFSIZE);
    
    pw = getpwuid(euid);

    elapsed = (int)difftime(end, start);

    hr =  elapsed / 3600;

    min = ((elapsed % 3600) / 60);

    sec = elapsed % 60;

    tdata = localtime(&start);

    strftime(stime, 29, "%m/%d/%Y %H:%M:%S", tdata);

    tdata = localtime(&end);

    strftime(etime, 29, "%m/%d/%Y %H:%M:%S", tdata);

    snprintf(logbuf, LOGBUFSIZE-1, "%s %s %02d:%02d:%02d %04d %s %s ",
         stime, etime,
         hr, min, sec, nprocs,
         ((pw && pw->pw_name) ? pw->pw_name : "user unknown"),
         fname);

    if (job->parent){
        sprintf(tmpbuf,"(spawned by #%d)",job->parent->job_id);
    }
    else{
        tmpbuf[0] = 0;
    }

    len1 = (job->log_status ? strlen(job->log_status) : 0);
    len2 = (ndlist[0] ? strlen(ndlist) : 0);
    len3 = (job->log_error ? strlen(job->log_error) : 0);
    len4 = (tmpbuf[0] ? strlen(tmpbuf) : 0);

    len = strlen(logbuf);

    if (len1 && (len + len1 + 2 < LOGBUFSIZE)){
         strcat(logbuf, ", ");
         strcat(logbuf, job->log_status);
         len = strlen(logbuf);
    }
    if (len2 && (len + len2 + 2 < LOGBUFSIZE)){
         strcat(logbuf, ", ");
         strcat(logbuf, ndlist);
         len = strlen(logbuf);
    }
    if (len3 && (len + len3 + 2 < LOGBUFSIZE)){
         strcat(logbuf, ", ");
         strcat(logbuf, job->log_error);
         len = strlen(logbuf);
    }
    if (len4 && (len + len4 + 2 < LOGBUFSIZE)){
         strcat(logbuf, ", ");
         strcat(logbuf, tmpbuf);
         len = strlen(logbuf);
    }

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
	job->log_done = 1;
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
    }

    FREELOCK();

    job->log_done = 1;
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
