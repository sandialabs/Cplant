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
** $Id: mcpmemtst.c,v 1.7 2001/08/22 16:45:15 pumatst Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "lanai_device.h"
#include "defines.h"
#include "../MCP/integrity.h"         /* bit patterns */


/*
** Command line options
*/
static struct option opts[]=
{
    {"unit",		required_argument,	0, 'u'},
    {"verbose",		no_argument,		0, 'v'},
    {"suppress",	no_argument,		0, 's'},
    {"help",		no_argument,		0, 'h'},
    {"continue",	no_argument,		0, 'c'},
    {"loop",		required_argument,	0, 'l'},
    {0, 0, 0, 0}
};

#if 0
/* test patterns */
static int pattern[] = { 0x00000000, 0xffffffff, 0xaaaaaaaa, 0x55555555,
                         0x5a5a5a5a, 0xa5a5a5a5, 0xf0f0f0f0, 0x0f0f0f0f,
                         0xff00ff00, 0x00ff00ff, 0xffff0000, 0x0000ffff,
                         0xaa55aa55, 0x55aa55aa, 0xaaaa5555, 0x5555aaaa
                       };

/* test pattern strings */
static const char* stringp [] = { "0x00000000", "0xffffffff", "0xaaaaaaaa", 
                                  "0x55555555", "0x5a5a5a5a", "0xa5a5a5a5", 
                                  "0xf0f0f0f0", "0x0f0f0f0f", "0xff00ff00", 
                                  "0x00ff00ff", "0xffff0000", "0x0000ffff",
                                  "0xaa55aa55", "0x55aa55aa", "0xaaaa5555", 
                                  "0x5555aaaa" 
                                };
#endif

/*
** Local functions
*/
void usage(char *pname);
int generic(unsigned int pattern, const char* stringp, unsigned int *start, unsigned int *end, 
            unsigned long offset, int cont, int verbose);
int prnd(unsigned int *start, unsigned int *end, unsigned long offset,
    int cont, int verbose);


/*
** External Functions
*/
extern int map_lanai(char *pname, int verbose, int unit);


/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
main(int argc, char *argv[] )
{

int unit;
int verbose;
int suppress;
int cont;
int loop;
int loop_cnt;
int iter;

unsigned int mcpmem_len;
unsigned int *mcpmem_start;
unsigned long offset;
unsigned int *mcpmem_end;

extern char *optarg;
extern int optind;
int error;
int index, i;
int ch;


    /* Set the defaults */
    verbose= 0;
    unit= 0;
    suppress= FALSE;
    cont= FALSE;
    loop= 1;
    error= FALSE;

    while (TRUE)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'u':
		unit= strtol(optarg, NULL, 10);
		break;
	    case 'v':
		verbose++;
		break;
	    case 'c':
		cont= TRUE;
		break;
	    case 'l':
		loop= strtol(optarg, NULL, 10);
		if (loop < 0)   {
		    fprintf(stderr, "%s: -loop cannot be < 0\n", argv[0]);
		    error= TRUE;
		}
		break;
	    case 's':
		suppress= TRUE;
		break;
	    case 'h':
	    case '?':
	    default:
		error= TRUE;
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }


    if (!suppress)   {
	printf("This program runs memory tests on the Myrinet card. Any "
	    "information on the\n");
	printf("card, including running programs (MCP). Will be destroyed.\n");
	printf("You will have to reload the MCP using the mcpload program\n");
	printf("    Do you want to continue? (Type y or Y. Anything else "
	    "aborts) ");
	ch= getchar();
	if ((ch != 'y') && (ch != 'Y'))   {
	    printf("Aborting...\n");
	    exit(0);
	}
    }

    if (map_lanai(argv[0], (verbose > 1), unit) != OK)   {
	exit(-1);
    }
    if (verbose)   {
	printf("Successfully mapped LANai[%d]\n", unit);
    }

    lanai_reset_unit(unit, 1);
    if (verbose)   {
	printf("Reset LANai[%d]\n", unit);
    }

    mcpmem_start= (unsigned int *) (&LANAI[unit][0]);
    offset= (unsigned long)mcpmem_start;
    mcpmem_len= lanai_memory_size(unit);
    mcpmem_end= (unsigned int *) ((unsigned long)mcpmem_start + mcpmem_len -
		    sizeof(int));
    printf("Mappings:\n");
    printf("    MCP memory start             0x%08x (%12d)   is   %p\n",
	0, 0, mcpmem_start);
    printf("    MCP memory end               0x%08x (%12d)   is   %p\n",
	mcpmem_len - 4, mcpmem_len - 4, mcpmem_end);

    printf("Tests:\n");

    if (loop == 0)   {
	loop_cnt= 1;
    } else   {
	loop_cnt= loop;
    }

    iter= 0;
    error= FALSE;
    while (loop_cnt)   {

      printf("   \"All\" tests fill SRAM with pattern and verify correctness\n");
      for (i=0; i<sizeof(pattern)/sizeof(unsigned int); i++) {

	if (generic(pattern[i], stringp[i], mcpmem_start, mcpmem_end, offset, cont, 
                    verbose)) {
	    printf("    All %s's test PASSED\n", stringp[i]);
	} else   {
	    printf("    All %s'ss test FAILED\n", stringp[i]);
	    error= TRUE;
	}
      }

	printf("    Random test fills SRAM with pseudo random numbers\n");
	if (prnd(mcpmem_start, mcpmem_end, offset, cont, verbose))   {
	    printf("    Random test PASSED\n");
	} else   {
	    printf("    Random test FAILED\n");
	    error= TRUE;
	}

	loop_cnt--;
	if ((loop_cnt == 0) && (loop == 0))   {
	    loop_cnt= 1;
	}
	iter++;
	if (iter == 1)   {
	    printf("Tests completed once\n");
	} else if (iter == 2)   {
	    printf("Tests completed twice\n");
	} else   {
	    printf("Tests completed %d times\n", iter);
	}
	if (error)   {
	    if (loop_cnt > 0)   {
		printf("Aborting program because of errors in tests\n");
	    }
	    break;
	}
    }

    return 0;

}  /* end of main() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

void
usage(char *pname)
{

    fprintf(stderr,
	"Usage: %s [-verbose] [-unit num] [-continue] [-suppress] [-help]\n"
	    , pname);
    fprintf(stderr, "       [-loop <cnt>]\n");
    fprintf(stderr, "   -verbose   Display progress information. This option "
	"can be repeated for more\n");
    fprintf(stderr, "              detailed information\n");
    fprintf(stderr, "   -continue  Don't stop after first error in each "
	"test\n");
    fprintf(stderr, "   -unit      Myrinet interface unit number. Default "
	"is 0\n");
    fprintf(stderr, "   -suppress  Suppress warning message about overwriting "
	"LANai memory\n");
    fprintf(stderr, "   -loop      Repeat tests <cnt> times. 0 means infinte "
	"loop\n");
    fprintf(stderr, "   -help      Display this usage information and exit\n");

}  /* end of usage() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
generic(unsigned int pattern, const char* stringp, unsigned int *start, unsigned int *end, 
        unsigned long offset, int cont, int verbose)
{

  unsigned int *current;
  int errcnt;


  if (verbose > 2)   {
    printf("   generic(pattern %s, start %p, end %p, offset %ld, cont %d\n",
    stringp, start, end, offset, cont);
  }
  current = start;
  while (current <= end)   {
   *current = pattern;
    current++;
  }
  if (verbose > 1)   {
    printf("    generic() Written all %s's\n", stringp);
  }

#ifdef DEBUG
  current -= 10;   *current= 1;
  current -= 1023; *current= 0x0a00000;
  current -= 1024; *current= 5;
  current -= 3333; *current= 0xffff0000;
#endif /* DEBUG */

  current= start;
  errcnt= 0;
  while (current <= end)   {
    if (*current != pattern)   {
      if ((errcnt == 0) || cont)   {
	if (errcnt == 0)   {
          printf("        Address             value       expected\n");
        }
        printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, pattern);
      }
      errcnt++;
     }
     current++;
  }

  if (errcnt > 0)   {
    printf("    There were %d errors in this test\n", errcnt);
    return FALSE;
  }

  return TRUE;

}  /* end of generic() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
prnd(unsigned int *start, unsigned int *end, unsigned long offset, int cont,
    int verbose)
{

unsigned int *current;
unsigned int value;
unsigned int seed;
int errcnt;


    if (verbose > 2)   {
	printf("    prnd(start %p, end %p, offset %ld, cont %d\n",
	    start, end, offset, cont);
    }

    seed= (unsigned int)time(NULL);
    srandom(seed);
    value= (unsigned int)random();
    if (verbose)   {
	printf("    prnd() Initial random value is 0x%08x (%d)\n", value, value);
    }

    current= start;
    while (current <= end)   {
	*current= value;
	current++;
	value= (unsigned int)random();
    }
    if (verbose > 1)   {
	printf("    prnd() Written all random values\n");
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */

    current= start;
    srandom(seed);
    value= (unsigned int)random();
    errcnt= 0;
    while (current <= end)   {
	if (*current != value)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        0x%08lX          0x%08X  0x%08X\n",
		    (unsigned long)current - offset, *current, value);
	    }
	    errcnt++;
	}
	current++;
	value= (unsigned int)random();
    }

    if (errcnt > 0)   {
	printf("    There were %d errors in this test\n", errcnt);
	return FALSE;
    }

    return TRUE;

}  /* end of prnd() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */
