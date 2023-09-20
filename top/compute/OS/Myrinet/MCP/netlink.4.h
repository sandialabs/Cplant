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
** $Id: netlink.4.h,v 1.6 2002/02/14 17:10:27 jbogden Exp $
**
** This file contains functions that deal with the network DMA and are
** specific to LANai 7.x.
*/

#ifndef NETLINK_4_H
#define NETLINK_4_H

/******************************************************************************/
/*
** snd_bufEOM()
** Initiates a DMA transfer from a shared memory location to the network.
** The send includes an EOM.
*/
extern __inline__ void
snd_bufEOM(int *buf, int len)
{

    fail_if (a09, len < (2 * sizeof(int)));
    fail_if (a10, (unsigned)buf & ALIGN_64b);
    fail_if (a80, len > DBL_BUF_SIZE);
    #if defined(ASSERT)
	if ((buf >= mcpshmem->snd_buf_A) &&
		(buf < (mcpshmem->snd_buf_A + DBL_BUF_SIZE / sizeof(int))))   {
	    /* buf is somewhere in snd_buf_A. Make sure len is contained */
	    fail_if (a92, ((buf + len / sizeof(int)) >
		(mcpshmem->snd_buf_A + DBL_BUF_SIZE / sizeof(int))));
	} else if ((buf >= mcpshmem->snd_buf_B) &&
		(buf < (mcpshmem->snd_buf_B + DBL_BUF_SIZE / sizeof(int))))   {
	    /* buf is somewhere in snd_buf_B. Make sure len is contained */
	    fail_if (a93, ((buf + len / sizeof(int)) >
		(mcpshmem->snd_buf_B + DBL_BUF_SIZE / sizeof(int))));
	} else   {
	    /* buf pointer is outside snd buf A or B */
	    fail_if(a94,1);
	}
    #endif /* ASSERT */

    len= (len + ALIGN_64b) & ~ALIGN_64b;

    SMP= buf;
    GM_STBAR();
    SMLT= (int *)((int)buf + len - 4);
    ISR_BARRIER(SMLT);

}  /* end of snd_bufEOM() */

/******************************************************************************/
/*
** snd_buf()
** Initiates a DMA transfer from a shared memory location to the network.
** The send does not include an EOM.
** Must be 64-bit aligned and multiple of 64 bit long.
*/
extern __inline__ void
snd_buf(int *buf, int len)
{

    fail_if (a62, ((unsigned)len & ALIGN_64b));
    fail_if (a09, len < (2 * sizeof(int)));
    fail_if (a10, (unsigned)buf & ALIGN_64b);
    fail_if (a80, len > DBL_BUF_SIZE);
    #if defined(ASSERT)
	if ((buf >= mcpshmem->snd_buf_A) &&
		(buf < (mcpshmem->snd_buf_A + DBL_BUF_SIZE / sizeof(int))))   {
	    /* buf is somewhere in snd_buf_A. Make sure len is contained */
	    fail_if (a92, ((buf + len / sizeof(int)) >
		(mcpshmem->snd_buf_A + DBL_BUF_SIZE / sizeof(int))));
	} else if ((buf >= mcpshmem->snd_buf_B) &&
		(buf < (mcpshmem->snd_buf_B + DBL_BUF_SIZE / sizeof(int))))   {
	    /* buf is somewhere in snd_buf_B. Make sure len is contained */
	    fail_if (a93, ((buf + len / sizeof(int)) >
		(mcpshmem->snd_buf_B + DBL_BUF_SIZE / sizeof(int))));
	} else   {
	    /* buf pointer is outside snd buf A or B */
	    fail_if(a94,1);
	}
    #endif /* ASSERT */

    SMP= buf;
    GM_STBAR();
    SML= (int *)((int)buf + len - 4);
    ISR_BARRIER(SML);

}  /* end of snd_buf() */


#endif /* NETLINK_4_H */
