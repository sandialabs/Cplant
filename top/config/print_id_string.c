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
** $Id: print_id_string.c,v 1.1 2000/06/26 17:18:56 rolf Exp $
** This program scans the files on its command line (or stdin if no
** arguments are present) for CVS Id strings. It will print the information
** from the Id strings in a nicely formatted way.
** This output listing can be used to compare against
**     cat /proc/cplant/versions.rtscts
** for example to see if the file versions in a source directory match
** the file versions used to build the currently running rtscts module.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>


/* Command line options */
static struct option opts[]=
{
    {"help",		no_argument,		0, 'h'},
    {0, 0, 0, 0}
};


/* Some constants */
#define MAXLINE		(1024)


/* Local functions */
void usage(char *pname);
void prcess_file(FILE *infp);


/******************************************************************************/

int
main(int argc, char *argv[] )
{

extern char *optarg;
extern int optind;
int error;
int index;
int ch;

FILE *infp;


    /* Set the defaults */
    error= 0;

    while (1)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'h':
		error= 1;
		break;
	    default:
		error= 1;
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }

    if (optind < argc)   {
	/* Process files listed on command line */
	while (optind < argc)   {
	    infp= fopen(argv[optind], "r");
	    if (infp == NULL)   {
		fprintf(stderr, "%s: Can't open %s for reading\n", argv[0],
		    argv[optind]);
		perror("");
	    } else   {
		prcess_file(infp);
		fclose(infp);
	    }
	    optind++;
	}
    } else   {
	/* Process stdin */
	infp= stdin;
	prcess_file(infp);
    }

    return 0;

}  /* end of main() */

/******************************************************************************/

void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s {source files; e.g. *.[chsS]}\n", pname);
    fprintf(stderr, "   or: %s    to read stdin\n", pname);

}  /* end of usage() */

/******************************************************************************/

void
prcess_file(FILE *infp)
{

char line[MAXLINE];
char *pos;
char *field_file;
char *field_version;
char *field_date;
char *field_time;
char *field_user;
static int cnt= 0;
static int first_time= 1;


    if (first_time)   {
	printf("Nbr File name            Ver     Date         Time       "
	    "User\n");
	printf("---------------------------------------------------------"
	    "------\n");
	first_time= 0;
    }

    while (fgets(line, MAXLINE, infp) != NULL)   {

	/* Find the $\Id: part */
	pos= strstr(line, "$I" "d: ");
	if (pos == NULL)   {
	    /* This is not an ID line, skip it */
	    continue;
	}

	/* Skip the Id field */
	field_file= strsep(&pos, " ");
	if (field_file == NULL) continue;

	/* Go to the file name */
	field_file= strsep(&pos, " ");
	if (field_file == NULL) continue;

	/* Find the version */
	field_version= strsep(&pos, " ");
	if (field_version == NULL) continue;

	/* Find the date */
	field_date= strsep(&pos, " ");
	if (field_date == NULL) continue;

	/* Find the time */
	field_time= strsep(&pos, " ");
	if (field_time == NULL) continue;

	/* Find the user */
	field_user= strsep(&pos, " ");
	if (field_user == NULL) continue;

	printf("%-3d %-20s %-7s %-12s %-10s %-12s\n", cnt, field_file,
	    field_version, field_date, field_time, field_user);

	cnt++;
    }

}  /* end of process_file() */

/******************************************************************************/
