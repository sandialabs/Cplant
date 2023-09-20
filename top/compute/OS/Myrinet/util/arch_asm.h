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
** $Id: arch_asm.h,v 1.3 2001/11/01 01:58:42 pumatst Exp $
**
** Assembly stuff that is specific to the alpha
*/

#ifndef ARCH_ASM
#define ARCH_ASM

    #ifdef __alpha__
	#undef mb
	#undef wmb
	#define mb()	__asm__ __volatile__("mb": : :"memory")
	#define wmb()	__asm__ __volatile__("wmb": : :"memory")
	#define PYXIS_DMA_BASE	(1024UL * 1024 * 1024)
    #else
      #ifdef __i386__
      /* note, i386 linux defines mb() as __asm__ __volatile__ ("" : : :"memory")
         in asm-i386/system.h */
          #include <asm/system.h>
          #define wmb() mb()
          #define PYXIS_DMA_BASE     0
      #else
        #ifdef __ia64__
          #include <asm/system.h>
          #define PYXIS_DMA_BASE     0
        #else
          #error "You need to provide the mb() and wmb() macros for your arch"
          #error "(They may be (), if you don't have out of order writes)"
        #endif /* __ia64__ */
      #endif /* __i386__ */
    #endif /* __alpha__ */

#endif /* ARCH_ASM */
