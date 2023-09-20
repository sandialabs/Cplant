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
** $Id: timer.h,v 1.4 2001/08/29 16:22:27 pumatst Exp $
*/


#ifndef TIMER_H
#define TIMER_H

#include "MCP.h"		/* For FALSE, etc. */
#include "lanai_def.h"		/* For TIME_INT_BIT, etc. */

/******************************************************************************/
/* The world timer, a 64-bit, 500ns resolution timer */

typedef struct   {
    unsigned int t0;    /* Low counter */
    unsigned int t1;    /* High counter */
} world_time_t;

extern world_time_t world_time;

/******************************************************************************/

extern __inline__ int  checkAlarm(world_time_t *next);
extern __inline__ void setAlarm(world_time_t *next, unsigned int delta);

/******************************************************************************/
/*
** Return whether world time > next. World time is essentially a 64-bit
** timer with a 500ns resolution. We use the interrupt timer IT to maintain
** the world clock. Whenever IT wraps from 0 to 0xffffffff it sets a bit
** in the ISR. We use that to increment the high value in the world time
** structure.
*/
extern __inline__ int
checkAlarm(world_time_t *next)
{

int rc;

    /* See if IT has wrapped */
    if (get_ISR() & TIME_INT_BIT)   {
	set_ISR(TIME_INT_BIT);
	world_time.t1++;
    }

    /* Now, update the low part of world timer. Remember, IT counts backwards */
    world_time.t0= 0xffffffff - IT;

    /* Now see if the world time is larger than next */
    rc= FALSE;
    if (world_time.t1 > next->t1)   {
	rc= TRUE;
    }
    if ((world_time.t1 == next->t1) && (world_time.t0 > next->t0))   {
	rc= TRUE;
    }

    return rc;

}  /* end of checkAlarm() */

/******************************************************************************/

extern __inline__ void
setAlarm(world_time_t *next, unsigned int delta)
{

    /* Update world time and then add delta to it */
    /* See if IT has wrapped */
    if (get_ISR() & TIME_INT_BIT)   {
	set_ISR(TIME_INT_BIT);
	world_time.t1++;
    }

    /* Now, update the low part of world timer. Remember, IT counts backwards */
    world_time.t0= 0xffffffff - IT;


    /* Finally, Let's set the new Alarm time value */
    next->t1= world_time.t1;
    if ((next->t0 = world_time.t0 + delta) < world_time.t0)   {
	/* Delta makes t0 wrap! */
	next->t1++;
    }

}  /* end of setAlarm() */

/******************************************************************************/

extern __inline__ void
getWorldTime(world_time_t *time)
{

    /* See if IT has wrapped */
    if (get_ISR() & TIME_INT_BIT)   {
	set_ISR(TIME_INT_BIT);
	world_time.t1++;
    }

    /* Now, update the low part of world timer. Remember, IT counts backwards */
    world_time.t0= 0xffffffff - IT;

    time->t0= world_time.t0;
    time->t1= world_time.t1;

}  /* end of getWorldTime() */

/******************************************************************************/

#endif /* TIMER_H */
