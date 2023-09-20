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
** $Id: hstmemtst.c,v 1.3 2001/08/22 16:45:15 pumatst Exp $
** Malloc and test a region of memory
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#define TRUE	(1)
#define FALSE	(0)


/*
** Command line options
*/
static struct option opts[]=
{
    {"verbose",		no_argument,		0, 'v'},
    {"help",		no_argument,		0, 'h'},
    {"continue",	no_argument,		0, 'c'},
    {"loop",		required_argument,	0, 'l'},
    {"length",		required_argument,	0, 'L'},
    {0, 0, 0, 0}
};


/*
** Local functions
*/
void usage(char *pname);
int all0s(unsigned int *start, unsigned int *end, int cont, int verbose);
int all1s(unsigned int *start, unsigned int *end, int cont, int verbose);
int alternate(unsigned int *start, unsigned int *end, int cont, int verbose);
int prnd(unsigned int *start, unsigned int *end, int cont, int verbose);


/*
** External Functions
*/


/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
main(int argc, char *argv[] )
{

int verbose;
int cont;
int loop;
int loop_cnt;
int iter;
int tstlen;

unsigned int *mem_start;
unsigned int *mem_end;

extern char *optarg;
extern int optind;
int error;
int index;
int ch;


    /* Set the defaults */
    verbose= 0;
    tstlen= 10 * 1024 * 1024;
    cont= FALSE;
    loop= 1;
    error= FALSE;

    while (TRUE)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    case 'c':
		cont= TRUE;
		break;
	    case 'L':
		tstlen= strtol(optarg, NULL, 10);
		if (tstlen < 0)   {
		    fprintf(stderr, "%s: -length %d must not be < 0\n",
			argv[0], tstlen);
		    error= TRUE;
		}
		break;
	    case 'l':
		loop= strtol(optarg, NULL, 10);
		if (loop < 0)   {
		    fprintf(stderr, "%s: -loop %d must not be < 0\n",
			argv[0], loop);
		    error= TRUE;
		}
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



    mem_start= (unsigned int *) malloc(tstlen);
    if (mem_start == NULL)   {
	fprintf(stderr, "%s: Not enough memory to malloc %d bytes\n",
	    argv[0], tstlen);
	exit(-1);
    }
    mem_end= (unsigned int *) ((unsigned long)mem_start + tstlen - sizeof(int));
    printf("Mappings:\n");
    printf("    memory start                 %p\n", mem_start);
    printf("    memory end                   %p\n", mem_end);

    printf("Tests:\n");

    if (loop == 0)   {
	loop_cnt= 1;
    } else   {
	loop_cnt= loop;
    }

    iter= 0;
    error= FALSE;
    while (loop_cnt)   {
	printf("    All zeros fills memory with 0's and verifies correctness\n");
	if (all0s(mem_start, mem_end, cont, verbose))   {
	    printf("    All 0's test PASSED\n");
	} else   {
	    printf("    All 0's test FAILED\n");
	    error= TRUE;
	}

	printf("    All ones fills memory with 1's and verifies correctness\n");
	if (all1s(mem_start, mem_end, cont, verbose))   {
	    printf("    All 1's test PASSED\n");
	} else   {
	    printf("    All 1's test FAILED\n");
	    error= TRUE;
	}

	printf("    Alternate tests memory with 101010... and 010101...\n");
	if (alternate(mem_start, mem_end, cont, verbose))   {
	    printf("    Alternate test PASSED\n");
	} else   {
	    printf("    Alternate test FAILED\n");
	    error= TRUE;
	}

	printf("    Random test fills memory with pseudo random numbers\n");
	if (prnd(mem_start, mem_end, cont, verbose))   {
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

    fprintf(stderr, "Usage: %s [-verbose] [-continue] [-loop <cnt>] "
	"[-length <len>] [-help]\n" , pname);
    fprintf(stderr, "   -verbose   Display progress information. This option "
	"can be repeated for more\n");
    fprintf(stderr, "              detailed information\n");
    fprintf(stderr, "   -continue  Don't stop after first error in each "
	"test\n");
    fprintf(stderr, "   -loop      Repeat tests <cnt> times. 0 means infinte "
	"loop. 1 is default\n");
    fprintf(stderr, "   -length    Malloc and test <len> bytes. 10 MB default\n");
    fprintf(stderr, "   -help      Display this usage information and exit\n");

}  /* end of usage() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
all0s(unsigned int *start, unsigned int *end, int cont, int verbose)
{

unsigned int *current;
int errcnt;


    if (verbose > 2)   {
	printf("    all0s(start %p, end %p, cont %d\n", start, end, cont);
    }
    current= start;
    while (current <= end)   {
	*current= 0;
	current++;
    }
    if (verbose > 1)   {
	printf("    all0s() Written all zeros\n");
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
	if (*current != 0)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0);
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

}  /* end of all0s() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
all1s(unsigned int *start, unsigned int *end, int cont, int verbose)
{

unsigned int *current;
int errcnt;


    if (verbose > 2)   {
	printf("    all1s(start %p, end %p, cont %d\n", start, end, cont);
    }
    current= start;
    while (current <= end)   {
	*current= 0xffffffff;
	current++;
    }
    if (verbose > 1)   {
	printf("    all1s() Written all ones\n");
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */

    current= start;
    errcnt= 0;
    while (current <= end)   {
	if (*current != 0xffffffff)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0xffffffff);
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

}  /* end of all1s() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
alternate(unsigned int *start, unsigned int *end, int cont, int verbose)
{

unsigned int *current;
int errcnt;


    if (verbose > 2)   {
	printf("    alternate(start %p, end %p, cont %d\n", start, end, cont);
    }
    current= start;
    while (current < end)   {
	*current= 0xAAAAAAAA;
	current++;
	*current= 0x55555555;
	current++;
    }
    if (verbose > 1)   {
	printf("    alternate() Written all 0xAAAAAAAA and 0x55555555\n");
    }

    #ifdef DEBUG
	current -= 1023; *current= 0x0a00000;
	current -= 1024; *current= 5;
	current -= 10;   *current= 1;
	current -= 3333; *current= 0xffff0000;
    #endif /* DEBUG */

    current= start;
    errcnt= 0;
    while (current <= end)   {
	if (*current != 0xAAAAAAAA)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0xAAAAAAAA);
	    }
	    errcnt++;
	}
	current++;
	if (*current != 0x55555555)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0x55555555);
	    }
	    errcnt++;
	}
	current++;
    }



    current= start;
    while (current <= end)   {
	*current= 0x55555555;
	current++;
	*current= 0xAAAAAAAA;
	current++;
    }
    if (verbose > 1)   {
	printf("    alternate() Written all 0x55555555 and 0xAAAAAAAA\n");
    }

    #ifdef DEBUG
	current -= 11023; *current= 0x0a00000;
	current -= 11024; *current= 5;
	current -= 110;   *current= 1;
	current -= 13333; *current= 0xffff0000;
    #endif /* DEBUG */

    current= start;
    while (current <= end)   {
	if (*current != 0x55555555)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0x55555555);
	    }
	    errcnt++;
	}
	current++;
	if (*current != 0xAAAAAAAA)   {
	    if ((errcnt == 0) || cont)   {
		if (errcnt == 0)   {
		    printf("        Address             value       expected\n");
		}
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, 0xAAAAAAAA);
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

}  /* end of alternate() */

/* --> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ <-- */

int
prnd(unsigned int *start, unsigned int *end, int cont, int verbose)
{

unsigned int *current;
unsigned int value;
unsigned int seed;
int errcnt;


    if (verbose > 2)   {
	printf("    prnd(start %p, end %p, cont %d\n", start, end, cont);
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
		printf("        %p          0x%08X  0x%08X\n",
		    current, *current, value);
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
