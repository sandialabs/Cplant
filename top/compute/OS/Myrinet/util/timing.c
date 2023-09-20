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
** $Id: timing.c,v 1.4 2001/08/22 16:45:15 pumatst Exp $
** Compile with 
**     gcc -DTEST timing.c -o timing
** for standalone operation. Use
**     gcc -c timing.c -o timing.o to link with other code.
*/
#include <sys/time.h>
#include <unistd.h>
#include "timing.h"


static struct timeval start_tv, stop_tv;
static void tvsub(struct timeval *tdiff, struct timeval *t1,
	    struct timeval *t0);
 
/******************************************************************************/

/*
 * Start timing now.
 */
void
start(void)
{
        (void) gettimeofday(&start_tv, (struct timezone *) 0);
}

/******************************************************************************/
 
/*
 * Stop timing and return real time in microseconds.
 */
double
stop(void)
{
        struct timeval tdiff;
	unsigned long result;
	double double_result;
 
        (void) gettimeofday(&stop_tv, (struct timezone *) 0);
 
        tvsub(&tdiff, &stop_tv, &start_tv);
        result = (tdiff.tv_sec * 1000000 + tdiff.tv_usec);

	double_result = (double)result / 1000000.0;
	return double_result;
}

/******************************************************************************/

static void
tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{
    tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
    #ifdef DEBUG
	printf("seconds     start %4d, stop %4d, diff %4d\n",
	    t0->tv_sec, t1->tv_sec, tdiff->tv_sec);
    #endif /* DEBUG */

    tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
    #ifdef DEBUG
	printf("u seconds   start %4d, stop %4d, diff %4d\n",
	    t0->tv_usec, t1->tv_usec, tdiff->tv_usec);
    #endif /* DEBUG */

    if (tdiff->tv_usec < 0)
	    tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

/******************************************************************************/

#ifdef TEST
char buf[1024];
#define NUMTRAPS	(1000)

main()
{

int i;
double t;
double overhead;
int uid;


	start();
	for (i= 0; i < NUMTRAPS; i++)   {
	}
	overhead= stop();
	printf("overhead is %5.2f us\n", overhead * 1000000.0);


	start();
	for (i= 0; i < NUMTRAPS; i++)   {
		/* uid= getuid(); */
		mlock(buf, 1024);
	}
	t= stop();
	t= t - overhead;
	printf("Average trap time is %5.2f us (%d traps done)\n",
	    (t / NUMTRAPS) * 1000000.0, NUMTRAPS);
}
#endif /* TEST */
