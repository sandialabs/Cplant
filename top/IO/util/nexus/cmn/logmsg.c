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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cmn.h"
#include "queue.h"
#include "smp.h"

IDENTIFY("$Id: logmsg.c,v 0.9 2001/07/18 18:57:26 rklundt Exp $");

struct logdes;

struct logger {
	const char *name;
	unsigned flags;
	int	(*func)(struct logger *, const char *);
	void	(*logs)(struct logdes *, int, const char *);
};

/*
 * Logger flags.
 */
#define LOGFLG_PID	0x1
#define LOGFLG_TSTAMP	0x2

static void logs_syslog(struct logdes *, int, const char *);
static int log_to_syslog(struct logger *, const char *);
static int log_to_fildes(struct logger *, const char *);
static void logs_fildes(struct logdes *, int, const char *);
static int log_to_file(struct logger *, const char *);

static struct logger loggers[] = {
	{
	  "syslog",
	  0,
	  log_to_syslog,
	  logs_syslog
	}, {
	  "fildes",
	  LOGFLG_PID|LOGFLG_TSTAMP,
	  log_to_fildes,
	  logs_fildes
	}, {
	  "file",
	  LOGFLG_PID|LOGFLG_TSTAMP,
	  log_to_file,
	  logs_fildes
	}
};

struct logdes {
	SIMPLEQ_ENTRY(logdes) link;
	enum { LDTY_SYSLOG, LDTY_FILDES } tag;
	union {
		int	u_fildes;			/* LDTY_FILDES */
	} u;
	struct logger *loggerp;
};

static mutex_t mutex = MUTEX_INITIALIZER;
static SIMPLEQ_HEAD(, logdes) log_list = SIMPLEQ_HEAD_INITIALIZER(log_list);

/*
 * Add a syslog logging mechanism to the list of logs.
 */
static int
add_log_syslog(struct logger *loggerp)
{
	struct logdes *ldp;

	ldp = m_alloc(sizeof(struct logdes));
	if (ldp == NULL)
		return -1;
	ldp->tag = LDTY_SYSLOG;
	ldp->loggerp = loggerp;
	mutex_lock(&mutex);
	SIMPLEQ_INSERT_TAIL(&log_list, ldp, link);
	mutex_unlock(&mutex);
	return 0;
}

/*
 * Parse syslog spec and enable logging to the resulting facility.
 */
static int
log_to_syslog(struct logger *loggerp, const char *spec)
{
	int	err;
	int	facility;
	static int isopen = 0;
	static mutex_t my_mutex = MUTEX_INITIALIZER;

	facility = 0;					/* compiler cookie */
	err = 0;

	/*
	 * They can only request the syslog mechanism once.
	 */
	mutex_lock(&my_mutex);
	if (isopen)
		err = EEXIST;
	isopen = 1;
	mutex_unlock(&my_mutex);
	if (err) {
		errno = err;
		return -1;
	}

	/*
	 * Parse spec to get facility.
	 */
	if (strcasecmp(spec, "daemon") == 0)
		facility = LOG_DAEMON;
	else if (strcasecmp(spec, "local") == 0) {
		if (*(spec + 5) == '0')
			facility = LOG_LOCAL0;
		else if (*(spec + 5) == '1')
			facility = LOG_LOCAL1;
		else if (*(spec + 5) == '2')
			facility = LOG_LOCAL2;
		else if (*(spec + 5) == '3')
			facility = LOG_LOCAL3;
		else if (*(spec + 5) == '4')
			facility = LOG_LOCAL4;
		else if (*(spec + 5) == '5')
			facility = LOG_LOCAL5;
		else if (*(spec + 5) == '6')
			facility = LOG_LOCAL6;
		else if (*(spec + 5) == '7')
			facility = LOG_LOCAL7;
		else
			err = EINVAL;
		if (!err && *(spec + 6) != '\0')
			err = EINVAL;
	} else
		err = EINVAL;
	if (err) {
		errno = err;
		return -1;
	}

	openlog(pgmname, LOG_NDELAY|LOG_PID, facility);
	return add_log_syslog(loggerp);
}

/*
 * Log to syslog mechanism.
 */
static void
logs_syslog(struct logdes *ldp, int prio, const char *s)
{
	size_t	len;
	char	*buf, *cp;

	if (ldp->tag != LDTY_SYSLOG)
		panic("logs_syslog: descriptor not syslog");

	/*
	 * We use our own buffer so that we can escape `%' characters.
	 * They have meaning otherwise.
	 */
	len = strlen(s);
	buf = m_alloc(len << 2);
	if (buf != NULL) {
		cp = buf;
		while (*s != '\0') {
			*cp++ = *s;
			if (*s == '%')
				*cp++ = *s;
			s++;
		}
		*cp = '\0';
		syslog(prio, buf);
		free(buf);
	} else
		syslog(LOG_CRIT, "Can't alloc log buf");
}

static int
add_log_fd(struct logger *loggerp, int fd)
{
	struct logdes *ldp;

	ldp = m_alloc(sizeof(struct logdes));
	if (ldp == NULL)
		return -1;
	ldp->tag = LDTY_FILDES;
	ldp->u.u_fildes = fd;
	ldp->loggerp = loggerp;
	mutex_lock(&mutex);
	SIMPLEQ_INSERT_TAIL(&log_list, ldp, link);
	mutex_unlock(&mutex);
	return 0;
}

/*
 * Parse fildes spec and enable logging to the resulting descriptor.
 */
static int
log_to_fildes(struct logger *loggerp, const char *spec)
{
	long	l;
	char	*cp;
	int	fd;
	struct stat stbuf;

	l = strtol(spec, &cp, 0);
	if (*spec == '\0' || *cp != '\0' || l < 0) {
		errno = EINVAL;
		return -1;
	}

	fd = (int )l;
	if (fstat(fd, &stbuf) != 0)
		return -1;

	return add_log_fd(loggerp, fd);
}

/*
 * Log to file descriptor mechanism.
 */
static void
logs_fildes(struct logdes *ldp, int prio IS_UNUSED, const char *s)
{
	size_t	len;
	char	*buf, *cp;

	if (ldp->tag != LDTY_FILDES)
		panic("logs_fildes: descriptor not file");

	/*
	 * We use our own buffer so that we can accomplish the proper
	 * semantics (append new-line) in a single write. To do otherwise
	 * runs the risk of having others logging to the same descriptor
	 * interleave their output into our stream.
	 */
	len = strlen(s);
	buf = m_alloc(len + 256);
	if (buf != NULL) {
		cp = buf;
		(void )strcpy(cp, s);
		cp += len;
		if (len && s[len - 1] != '\n') {
			*cp++ = '\n';
			*cp = '\0';
		}
		(void )write(ldp->u.u_fildes, buf, cp - buf);
		free(buf);
	} else
		(void )write(ldp->u.u_fildes, "Can't alloc log buf\n", 20);
	(void )fsync(ldp->u.u_fildes);
}

static int
log_to_file(struct logger *loggerp, const char *path)
{
	int	fd;

	fd = open(path, O_CREAT|O_APPEND|O_WRONLY, 0644);
	if (fd < 0)
		return -1;
	return add_log_fd(loggerp, fd);
}

/*
 * Add a log mechanism.
 */
int
log_to(const char *spec)
{
	char	*cp;
	unsigned indx;

	cp = strchr(spec, ':');
	if (cp == NULL) {
		errno = EINVAL;
		return -1;
	}
	cp++;

	for (indx = 0; indx < sizeof(loggers) / sizeof(struct logger); indx++)
		if (strncasecmp(loggers[indx].name,
		    spec,
		    strlen(loggers[indx].name)) == 0)
			break;
	if (indx >= sizeof(loggers) / sizeof(struct logger)) {
		errno = ENOTSUP;
		return -1;
	}

	return (*loggers[indx].func)(&loggers[indx], cp);
}

void
logs(int prio, const char *s)
{
	size_t	len;
	struct logdes *ldp;
	static char msgbuf[MSGSTR_MAX], *cp;
	char	*tstamp;
	char	*pidstr;

#define SMALL_MSGMODSIZ		256

	len = strlen(s);
	if (!len)
		return;
	tstamp = NULL;
	pidstr = NULL;
	mutex_lock(&mutex);
	for (ldp = SIMPLEQ_FIRST(&log_list);
	     ldp != NULL;
	     ldp = SIMPLEQ_NEXT(ldp, link)) {
		cp = msgbuf;
		if (ldp->loggerp->flags & LOGFLG_TSTAMP) {
			if (tstamp == NULL) {
				struct tm tm;

				tstamp = m_alloc(SMALL_MSGMODSIZ + 1);
				if (tstamp != NULL) {
					time_t	t;

					(void )time(&t);
					(void )localtime_r(&t, &tm);
					(void )strftime(tstamp,
							SMALL_MSGMODSIZ,
							"%b %d %X %Z",
							&tm);
				}
			}
			if (tstamp != NULL) {
				(void )strcpy(cp, tstamp);
				cp += strlen(tstamp);
			}
		}
		if (ldp->loggerp->flags & LOGFLG_PID) {
			if (pidstr == NULL) {
				pidstr = m_alloc(SMALL_MSGMODSIZ + 1);
				if (pidstr != NULL) {
					(void )sprintf(pidstr,
						       "%s"
#ifndef SINGLE_THREAD
						       "[" THREAD_T_FMT "]:",
#else
						       "[%ld]:",
#endif
						       cp != msgbuf ? " " : "",
#ifndef SINGLE_THREAD
						       thread_self()
#else
						       (long )getpid()
#endif
						       );
				}
			}
			if (pidstr != NULL) {
				(void )strcpy(cp, pidstr);
				cp += strlen(pidstr);
			}
		}
		(void )strcpy(cp, s);
		(*ldp->loggerp->logs)(ldp, prio, msgbuf);
	}
	mutex_unlock(&mutex);
	if (tstamp != NULL)
		free(tstamp);
	if (pidstr != NULL)
		free(pidstr);

#undef SMALL_MSGMODSIZ

}

void
vlogmsg(int prio, const char *file, unsigned line, const char *fmt, va_list ap)
{
	char	*msgbuf;
	static const char lostmsg[] =
	    "can't alloc msgbuf buffer -- message lost\n";

	msgbuf = m_alloc(MSGSTR_MAX);
	if (msgbuf != NULL) {
		(void )sprintf(msgbuf, "%s:%u:", (char *)file, line);
		(void )vsprintf(msgbuf + strlen(msgbuf), fmt, ap);
	} else
		msgbuf = (char *)lostmsg;
	logs(prio, msgbuf);
	if (msgbuf != lostmsg)
		free(msgbuf);
}

void
logmsg(int prio, const char *file, unsigned line, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vlogmsg(prio, file, line, fmt, ap);
	va_end(ap);
}
