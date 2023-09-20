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
** $Id: bebopd_log.c,v 1.6 2001/08/06 06:32:02 lafisk Exp $
**
** bebopd logs machine usage in the same manner as Tflops
*/
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "puma.h"
#include "config.h"
#include "srvr_err.h"

static char fullname[MAXPATHLEN];
static char logdir[MAXPATHLEN];

static int newdate(struct tm *ltm);

char *
myLogFileName()
{
struct tm *tm;
time_t t1;
char *c, *pname;

    t1 = time(NULL);

    tm = localtime(&t1);

    if (!newdate(tm)){
	return fullname;
    }

    log_msg("myLogFileName: time to set or reset the log file name\n");

    pname = (char *)log_file_name();  /* name of yod's user log file */

    if (pname){
	strcpy(fullname, pname);  /* keep directory name only */

	c = strrchr(fullname, '/');
	if (c) {
	   *(c+1) = 0;
	}
	else{
	    strcpy(fullname,"./");
	}
    }
    else{
        strcpy(fullname,"/tmp");
    }

    /*
    ** write tflops style log in the same directory
    ** in which cplant style user log is written
    */

    c = realpath(fullname, logdir);

    if (!c){
        log_warning("bebopd log file path error (%s -> %s)",fullname,logdir);
        fullname[0] = 0;
	return NULL;
    }

    sprintf(fullname, "%s/run.log.%02d-%02d-%02d", logdir,
              tm->tm_mon+1, tm->tm_mday, tm->tm_year % 100);

    log_msg("             %s\n",fullname);

    return fullname;
}

static int formeryr=0, 
	   formermon=-1, 
	   formerday=0;

/*
** TRUE - current date has changed since last call to newdate()
** FALSE - current date is the same
*/
static int
newdate(struct tm *olddate)
{
int status;

    status = FALSE;

    if ((olddate->tm_mday != formerday) ||
	(olddate->tm_mon != formermon)  ||
        (olddate->tm_year != formeryr)    ){

	status = TRUE;
	formerday=olddate->tm_mday;
	formermon=olddate->tm_mon;
	formeryr=olddate->tm_year;
    }
    
    return status;
}
int
write_log_record(char *s)
{
char *logfname;
FILE *fp;
int len, rc;

    logfname = myLogFileName();

    if (!logfname) return 0;

    fp = fopen(logfname, "a");

    if (!fp){
	log_warning("log_user - fopen %s (%d)",logfname,strlen(logfname));
	return (-1);
    }

    len = strlen(s);

    if (s[len-1] != '\n'){
	rc = fprintf(fp, "%s\n",s);

	if (rc != (len+1)){
	    log_warning("log_user - fprintf");
	    fclose(fp);
	    return (-1);
	}
    }
    else{
	rc = fwrite(s, len, 1, fp);

	if (rc != 1){
	    log_warning("log_user - fwrite");
	    fclose(fp);
	    return (-1);
	}
    }

    fflush(fp);

    fclose(fp);

    return 0;
}
