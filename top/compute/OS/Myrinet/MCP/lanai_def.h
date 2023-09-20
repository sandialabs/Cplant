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
** $Id: lanai_def.h,v 1.5 2001/08/29 16:22:27 pumatst Exp $
*/

#ifndef LANAI_DEF_H
#define LANAI_DEF_H

#if defined (L4)
    #include <lanai4_def.h>
    #define SET_HOST_SIG_BIT()	set_ISR(HOST_SIG_BIT)
#endif
#if defined (L5)
    #include <lanai5_def.h>
    #error "LANai 5 is not currently supported"
#endif
#if defined (L7)
    #include <lanai7_def.h>
    #define IT			IT0
    #define CRC_ENABLE_BIT	(0)
    #define TIMEOUT		LINK
    #define SEND_RDY_BIT	SEND_INT_BIT
    #define TIME_INT_BIT	TIME0_INT_BIT
    #define DMA_INT_BIT		WAKE_INT_BIT

    /* There is a bug in some LANai 7.x that wont always take _sig bits */
    #define SET_HOST_SIG_BIT()	while (!(get_ISR() & HOST_SIG_BIT)) {	\
				    set_ISR(HOST_SIG_BIT);		\
				}
#endif
#if defined (L9)
    #include <lanai9_def.h>
    #define IT			IT0
    #define MEMORY_INT_BIT	MEM_INT_BIT
    #define PARITY_INT_BIT	PAR_INT_BIT
    #define LAN0_SIG_BIT	LANAI_SIG_BIT	/* Just for benchmarking */
    #define SEND_RDY_BIT	SEND_INT_BIT
    #define TIME_INT_BIT	TIME0_INT_BIT
    #define SET_HOST_SIG_BIT()	set_ISR(HOST_SIG_BIT)
#endif

/*
** The following is Myricom code
*/

/* ": : :" for C++ compat. */
#define GM_STBAR() asm volatile ("! GM_STBAR" : : :"memory")
#define GM_PARAMETER_MAY_BE_UNUSED(p) ((void)(p))


/* Introduce a barrier between a write to a special register and
   the subsequent read of ISR.

   Here we pass SPECIAL as a pointer to prevent the compiler from from
   re-reading the volatile special register after it is written. */

extern __inline__ void
isr_barrier (unsigned int* special)
{
    #if L4 | L5 | L6
	asm ("nop ! ISR_BARRIER()": : "m" (*special):"isr");
    #elif L7 | L8
	GM_PARAMETER_MAY_BE_UNUSED (special);
	/* no need to introduce delay */
    #elif L9
	asm ("nop ! ISR_BARRIER()": "=m" (ISR):"m" (*special));
    #else
	#error unrecognized LANai type
    #endif
}

#define ISR_BARRIER(special) isr_barrier ((unsigned int*) &(special))

extern __inline__ unsigned int
get_ISR(void)        
{
    #if (L4 | L5 | L6)              /*  && GCC_VERSION >= GCC (2,8) */
	register unsigned int isr asm ("isr");
	asm volatile ("nop ! ISR barrier":"=r" (isr));
	return isr;
    #elif L7 | L8 | L9
	return ISR;
    #endif
}

extern __inline__ void
set_ISR (unsigned int val)
{                     
    #if defined lanai3              /*  && GCC_VERSION >= GCC (2,8) */
	/* BAD: circumvent volatile register bug in lanai3-gcc-2.8.1 */
	asm volatile ("mov %0,%?isr\n\t"::"r" (val):"isr", "imr", "memory");
    #else
	ISR= (int)val;
    #endif
}

extern __inline__ unsigned int
get_IMR(void)
{
    #if L4 | L5 | L6
	register unsigned int imr asm ("imr");
	asm volatile ("! IMR barrier":"=r" (imr));
	return imr;
    #elif L7 | L8 | L9
	return 0;
    #else
	#error unrecognize processor type
    #endif
}

extern __inline__ void
set_IMR (unsigned int val)
{
    #if L4 | L5 | L6
	#if GCC_VERSION >= GCC(2,95) || 1
	    /* This compiler does not support volatile registers */
	    asm volatile ("mov %0,%?imr"::"r" (val):"isr", "imr", "memory");
	#else
	    IMR= val;
	#endif
    #elif L7 | L8 | L9
	/* No IMR register on this machine. */
	GM_PARAMETER_MAY_BE_UNUSED (val);
    #else
	#error Do not know what to do.
    #endif
    GM_STBAR ();
}

#endif /* LANAI_DEF_H */
