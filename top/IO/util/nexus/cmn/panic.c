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
#include <stdarg.h>

#include "cmn.h"
#include "smp.h"

IDENTIFY("$Id: panic.c,v 0.3 2000/03/17 20:23:56 lward Exp $");

/*
 * Information about the current panic.
 */
struct {
	unsigned inprogress;
	union {
		thread_t thread;
	} u;
} panicinfo = { 0, {} };

/*
 * Print panic message.
 */
static void
vlogpanicf(const char *file, unsigned line, const char *msg, va_list ap)
{
	int	err;
	static mutex_t mutex = MUTEX_INITIALIZER;
	static char panicfmt[4096];

	if (panicinfo.inprogress)
		abort();

	err = _mutex_lock(&mutex);
	if (err) {
		logs(LOG_EMERG, "vlogpanicf: can't lock");
		return;
	}
	else if (panicinfo.inprogress && panicinfo.u.thread == thread_self())
		logs(LOG_EMERG, "vlogpanicf: reentered(2)");
	else {
		panicinfo.inprogress = 1;
		panicinfo.u.thread = thread_self();
		(void )sprintf(panicfmt, "panic: %s", msg);
		vlogmsg(LOG_EMERG, file, line, panicfmt, ap);
	}
	err = _mutex_unlock(&mutex);
	if (err)
		logs(LOG_EMERG, "vlogpanicf: can't unlock panic");
}

/*
 * Print a message and abort.
 */
void
_panic(const char *file, unsigned line, const char *msg, ...)
{
	va_list	ap;

	va_start(ap, msg);
	vlogpanicf(file, line, msg, ap);
	va_end(ap);
	abort();
}
