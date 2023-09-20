
#include <stdio.h>
#include <asm/page.h>                 /* PAGE_SIZE */
#include "Pkt_module.h"               /* FUDGE     */
#include "RTSCTS_pkthdr.h"            /* pkthdr_t  */
#include <lib-p30.h>                  /* ptl_hdr_t */

/* usage: % mpi_lsize [-v] */

int main(int argc, char **argv)
{
  if ( argc > 1 ) {
  printf("----------------------------------------------------------\n");
#ifdef __alpha__
  printf("on an alpha we get:\n");
#endif
#ifdef __i386__
  printf("on an x86 we get:\n");
#endif
  printf("size of PAGE:       %d\n", PAGE_SIZE);
  printf("size of FUDGE:      %d\n", FUDGE);
  printf("size of MYRPKT_MTU: %d\n", MYRPKT_MTU);
  printf("size of pkthdr_t:   %d\n", sizeof(pkthdr_t));
  printf("size of ptl_hdr_t:  %d\n", sizeof(ptl_hdr_t));
  printf("long msg size: MYRPKT_MTU - (pkthdr_t + ptl_hdr_t)=%d\n",
            MYRPKT_MTU - sizeof(pkthdr_t) - sizeof(ptl_hdr_t));
  printf("----------------------------------------------------------\n");
}
  printf("#define P30_DEFAULT_LONG_MSG %d\n", MYRPKT_MTU - sizeof(pkthdr_t) - sizeof(ptl_hdr_t));
  return 0;
}
