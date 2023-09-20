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
/* $Id: integrity.c,v 1.9 2000/11/21 07:01:12 rolf Exp $ */

#include "integrity.h"          /* bit patterns */
#include "integrity-enum.h"     /* bit patterns */
#include "MCP.h"		/* For crap */
#include "MCPshmem.h"		/* For ALIGN_32b, DBL_BUF_SIZE, etc. */
#include "lanai_def.h"		/* For DMA_DIR, ISR, etc. */
#include "init.h"		/* fault() */

extern int PCIavailable;

#if defined (L4)
    #include "ebus.4.h"
#endif
#if defined (L7) || defined (L9)
    #include "ebus.7.h"
#endif

int dma_integrity_test(void);

/******************************************************************************/
extern __inline__ void l2e_dma_start(int *buf, int len, unsigned int dest_addr);
extern __inline__ void e2l_dma_start(int *buf, int len, unsigned int src_addr);
extern __inline__ int  l2e_dma_done(void);
extern __inline__ int  e2l_dma_done(void);

/******************************************************************************/
int
dma_integrity_test(void)
{
    int i, itest, jloop, top, *ptr, *buf;
    unsigned int start, end;
    int failure = 0;


    top = mcpshmem->DMA_test_repeat; 

    for (jloop=0; jloop<top; jloop++) { /* repeat test of all patterns 
                                                           and buffers */
      mcpshmem->DMA_test_done = 0;
      mcpshmem->DMA_test_iter = jloop+1;;
      for (itest=0; itest<sizeof(pattern)/sizeof(int); itest++) {

            /* l2e test of rcv_buf_A */

	    mcpshmem->DMA_test_buf = 0;
	    mcpshmem->DMA_test_len = DBL_BUF_SIZE;
	    mcpshmem->DMA_test_index = itest;
	    mcpshmem->DMA_test_result = 0;
	    mcpshmem->DMA_buf_id = RCV_BUF_A;
	    mcpshmem->LANai2host = INT_DMA_L2E_INTEGRITY_TEST;
	    SET_HOST_SIG_BIT();

            buf = mcpshmem->rcv_buf_A;

	    ptr = buf;
	    for (i=0; i<(DBL_BUF_SIZE / sizeof(int)); i++)   {
		*(ptr++) = pattern[itest];
	    }

	    /* wait for the host to set up a buffer we can dma into */
	    while (mcpshmem->DMA_test_buf == 0) ;

	    /* now put into it; measure how long it takes */
	    start = RTC;
            l2e_dma_start(buf, DBL_BUF_SIZE, mcpshmem->DMA_test_buf);
            while ( !l2e_dma_done() ) ;
	    end = RTC;

	    mcpshmem->DMA_test_result = end - start;

	    while ( mcpshmem->DMA_test_result != 0 ) ;
	    while ( !(EIMR & HOST_SIG_BIT) ) ;

	    /* break out of both loops if an error occurs */
	    if ( mcpshmem->DMA_test_len != 0 )   {
		return -1;
	    }

            /* l2e test of rcv_buf_B */

	    mcpshmem->DMA_test_buf = 0;
	    mcpshmem->DMA_test_len = DBL_BUF_SIZE;
	    mcpshmem->DMA_test_index = itest;
	    mcpshmem->DMA_test_result = 0;
	    mcpshmem->DMA_buf_id = RCV_BUF_B;
	    mcpshmem->LANai2host = INT_DMA_L2E_INTEGRITY_TEST;
	    SET_HOST_SIG_BIT();

            buf = mcpshmem->rcv_buf_B;

	    ptr = buf;
	    for (i=0; i<(DBL_BUF_SIZE / sizeof(int)); i++)   {
		*(ptr++) = pattern[itest];
	    }

	    /* wait for the host to set up a buffer we can dma into */
	    while (mcpshmem->DMA_test_buf == 0) ;

	    /* now put into it; measure how long it takes */
	    start = RTC;

            l2e_dma_start(buf, DBL_BUF_SIZE, mcpshmem->DMA_test_buf);
            while ( !l2e_dma_done() ) ;

	    end = RTC;

	    mcpshmem->DMA_test_result = end - start;

	    while ( mcpshmem->DMA_test_result != 0 ) ;
	    while ( !(EIMR & HOST_SIG_BIT) ) ;

	    /* break out of both loops if an error occurs */
	    if ( mcpshmem->DMA_test_len != 0 )   {
		return -1;
	    }

            /* e2l test of snd_buf_A */

	    mcpshmem->DMA_test_buf = 0;
	    mcpshmem->DMA_test_len = DBL_BUF_SIZE;
	    mcpshmem->DMA_test_index = itest;
	    mcpshmem->DMA_test_result = 0;
	    mcpshmem->DMA_buf_id = SND_BUF_A;
	    mcpshmem->LANai2host = INT_DMA_E2L_INTEGRITY_TEST;
	    SET_HOST_SIG_BIT();

	    /* wait for the host to set up a buffer we can pull in */
	    while (mcpshmem->DMA_test_buf == 0) ;

	    /* now go get it; measure how long it takes */
	    start = RTC;

	    e2l_dma_start(mcpshmem->snd_buf_A, DBL_BUF_SIZE, mcpshmem->DMA_test_buf);
	    while ( !e2l_dma_done() ) ;

	    end = RTC;

	    ptr = mcpshmem->snd_buf_A;
	    for (i=0; i<(DBL_BUF_SIZE / sizeof(int)); i++) {
	      if ( (*(ptr++)) != pattern[itest] ) {
		/* need this xtra variable since we want to test 
                   the result later, but DMA_test_result is
                   set back to 0 in a host handshake...  */
		failure = 1;
		mcpshmem->DMA_test_result = 1;
	      }
	    }
	    if ( mcpshmem->DMA_test_result == 0 ) {
              mcpshmem->DMA_test_result = end - start;
	    }

	    while ( mcpshmem->DMA_test_result != 0 ) ;
	    while ( !(EIMR & HOST_SIG_BIT) ) ;

	    if ( failure == 1 ) { /* stop testing this pattern */
	      return -1;
	    }

            /* e2l test of snd_buf_B */

            if ( itest == sizeof(pattern)/sizeof(int)-1 ) {
              mcpshmem->DMA_test_done = 1;
            }

	    mcpshmem->DMA_test_buf = 0;
	    mcpshmem->DMA_test_len = DBL_BUF_SIZE;
	    mcpshmem->DMA_test_index = itest;
	    mcpshmem->DMA_test_result = 0;
	    mcpshmem->DMA_buf_id = SND_BUF_B;
	    mcpshmem->LANai2host = INT_DMA_E2L_INTEGRITY_TEST;
	    SET_HOST_SIG_BIT();

	    /* wait for the host to set up a buffer we can pull in */
	    while (mcpshmem->DMA_test_buf == 0) ;

	    /* now go get it; measure how long it takes */
	    start = RTC;

	    e2l_dma_start(mcpshmem->snd_buf_B, DBL_BUF_SIZE, mcpshmem->DMA_test_buf);
	    while ( !e2l_dma_done() ) ;

	    end = RTC;

	    ptr = mcpshmem->snd_buf_B;
	    for (i=0; i<(DBL_BUF_SIZE / sizeof(int)); i++) {
	      if ( (*(ptr++)) != pattern[itest] ) {
		/* need this xtra variable since we want to test 
                   the result later, but DMA_test_result is
                   set back to 0 in a host handshake...  */
		failure = 1;
		mcpshmem->DMA_test_result = 1;
	      }
	    }
	    if ( mcpshmem->DMA_test_result == 0 ) {
              mcpshmem->DMA_test_result = end - start;
	    }

	    while ( mcpshmem->DMA_test_result != 0 ) ;
	    while ( !(EIMR & HOST_SIG_BIT) ) ;

	    if ( failure == 1 ) { /* stop testing */
	      return -1;
	    }

      } /* pattern */
    } /* jloop */
    return 0;
}  /* end of dma_integrity_test() */

/******************************************************************************/
