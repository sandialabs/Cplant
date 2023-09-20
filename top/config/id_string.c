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
** $Id: id_string.c,v 1.2 2001/08/15 22:37:38 pumatst Exp $
** Process a list of CVS Id strings and create a C file that initializes
** a data structure with file names, versions, authors, dates, etc.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define OUTFILE		"versions.c"
#define MAXLINE		(1024)


int
main(int argc, char *argv[])
{

FILE *outfp;
FILE *infp;
int cnt_read;
int cnt_write;
char line[MAXLINE];
char *pos;
char *field_file;
char *field_version;
char *field_date;
char *field_time;
char *field_user;


    /* Create the output file */
    outfp= fopen(OUTFILE, "w+");
    if (outfp == NULL)   {
	fprintf(stderr, "%s: Could not open/create output file \"%s\"\n",
	    argv[0], OUTFILE);
	perror("");
	exit(-1);
    }


    /* Put the preamble into OUTFILE */
    fprintf(outfp, "/*\n");
    fprintf(outfp, "** This file has been created automatically by %s\n",
	argv[0]);
    fprintf(outfp, "** DO NOT MODIFY IT MANUALLY OR CHECK IT INTO CVS!\n");
    fprintf(outfp, "*/\n\n");
    fprintf(outfp, "#include <stdio.h>\n");
    fprintf(outfp, "#include \"versions.h\"\n\n");
    fprintf(outfp, "version_str_t version_strings[]= {\n");


    /*
    ** Now read from stdin and process the lines.
    ** We expect to see lines of the following form:
    ** ** $\Id: Pkt_handler.c,v 1.6 1999/12/01 21:55:49 rolf Exp $
    ** We discard lines that don't have the $\Id: present.
    */
    infp= stdin;
    cnt_read= 0;
    cnt_write= 0;
    while (fgets(line, MAXLINE, infp) != NULL)   {
	cnt_read++;

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


	fprintf(outfp, "\t{");
	fprintf(outfp, "\"%s\", ", field_file);
	fprintf(outfp, "\"%s\", ", field_version);
	fprintf(outfp, "\"%s\", ", field_date);
	fprintf(outfp, "\"%s\", ", field_time);
	fprintf(outfp, "\"%s\", ", field_user);
	fprintf(outfp, "},\n");
	cnt_write++;
    }

    /* Postamble of the version file */
    fprintf(outfp, "\t{NULL}\n");
    fprintf(outfp, "};\n");
    fclose(outfp);

    printf("%s: Read %d lines, wrote %d entries to %s\n", argv[0], cnt_read,
	cnt_write, OUTFILE);

    return 0;

}  /* end of main() */
