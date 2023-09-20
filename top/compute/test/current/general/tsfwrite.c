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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "puma.h"

/*
**   $Id: tsfwrite.c,v 1.5 2001/03/22 01:15:20 jsotto Exp $
**
** optional arguments:
**
**     -f name of file to open
**     -n number of lines to write to it
*/

extern char *optarg;
extern int optind, opterr, optopt;

char *defname="tsfwrite-file";

void
usage()
{
    printf("** optional arguments:\n");
    printf("**\n");
    printf("**     -f name of file to open\n");
    printf("**     -n number of lines to write to it\n\n");
}

int main(int argc, char *argv[])
{
FILE *fp;
int rc, nbytes, lines, i, opt;
char buf[256];
char fname[MAXPATHLEN];

    lines = 1;

    sprintf(fname,"%s.%04d",defname,_my_rank);

    while(1){
        opt = getopt(argc, argv, "f:n:");

        if (opt==EOF) break;

        switch(opt)
        {
            case 'f':
                if (strlen(optarg) > MAXPATHLEN-10){
                    if (_my_rank == 0){
                        printf("MAXPATHLEN exceeded\n"); 
                    }
                    exit(-1);
                }
		sprintf(fname,"%s.%04d",optarg,_my_rank);
                break;

            case 'n':
                lines = atoi(optarg);
                break;

            default:
                if (_my_rank == 0){
		    usage();
                }

                break;
        }
        
    }
    if (_my_rank == 0){
        printf("\n%s writing %d lines to\n\t%s\n",argv[0],lines,fname);
    }

    sprintf(buf, "%04d/%06d: here's a message \n",_my_rank,_my_ppid);

    nbytes = strlen(buf) + 1;

    fp = fopen(fname,"w");

    if (!fp){
	if (_my_rank == 0){
	    printf("can't open file %s to write\n",fname);
	}
        return -1;
    }

    for (i=0; i<lines; i++){
	rc = fwrite(buf, nbytes, 1, fp );
	if (rc !=  1) break;
    }
    
    fclose(fp);

    if (rc != 1) 
        return rc;
    else 
        return 0;
}
