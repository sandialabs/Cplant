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
** $Id: dclock.c,v 1.2 2001/08/22 16:45:14 pumatst Exp $
** a dclock() routine that uses the Myrinet card RTC (exported via
** /proc/dclock). See the test program below on how to use dclock()
** to measure time intervals. The RTC on the Myrinet card (at least
** the ones we have) gets incremented every 500ns.
**
** To test it, compile it with:
**     cc -O4 -DDCLOCK_TEST dclock.c -o dclock
*/
#include <stdio.h>

#define RTCTICK		(0.000000500)
#define PROCDCLOCK	"/proc/dclock"


/* ************************************************************************** */

double
dclock(void)
{

static FILE *fp= NULL;
int rc;
unsigned int rtc;


    if (fp == NULL)   {
	fp= fopen(PROCDCLOCK, "r");
	if (fp == NULL)   {
	    perror("Can't open " PROCDCLOCK);
	    return 0.0;
	}
    } else   {
	fseek(fp, 0l, SEEK_SET);
	fflush(fp);
    }

    rc=  fread(&rtc, sizeof(int), 1, fp);
    if (rc != 1)   {
	perror("Reading from " PROCDCLOCK);
	return 0.0;
    }

    return (rtc * RTCTICK);

}  /* end of dclock() */

/* ************************************************************************** */

#ifdef DCLOCK_TEST

int
main(int argc, char *argv[])
{

double t1, t2;


    /* prime dclock() */
    t1= dclock();


    t1= dclock();
    sleep(5);
    t2= dclock();
    printf("Time to sleep(5) was %9.6f\n", t2 - t1);


    t1= dclock();
    t2= dclock();
    printf("Time between two dclock() was %9.6f\n", t2 - t1);

}  /* end of main() */

#endif /* DCLOCK_TEST */

/* ************************************************************************** */
