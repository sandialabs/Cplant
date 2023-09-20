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
** $Id: pbs_read_usage.c,v 1.1 2000/03/20 20:59:30 lafisk Exp $
**
** Reads and prints out the scheduler's usage file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

typedef struct group_node_usage
{
  char name[9];
  long usage;
}node;

char *fname="/tmp/pbs/working/sched_priv/usage";

main(int argc, char *argv[])
{
node *nds;
int rc, fd, i, ii, len, recs;
struct stat buf;

    if (argc > 1){
        fname = argv[1];
    }

    rc = stat(fname, &buf);

    if (rc){
        perror("stat");
        exit(-1);
    }
    len = buf.st_size;
    recs = len / sizeof(node);

    if (len){
        nds = (node *)malloc(len);

        if (!nds){
            perror("malloc");
            exit(-1);
        }

        fd = open(fname, O_RDONLY);

        if (rc < 0){
            perror("open");
            exit(-1);
        }

        rc = read(fd, (void *)nds, len);

        if (rc < len){
            printf("Couldn't read usage file.\n");
            exit(-1);
        }

        close(fd);

        if (recs){
            printf("User\tRecent Node Hours\n====\t=================\n");
        }

        for (i=0; i < recs; i++){

            for (ii=0; ii<9; ii++){
                if (!isalpha((int)(nds[i].name[ii]))) break;
                printf("%c", nds[i].name[ii]);
            }
/*
            printf("\t%d\n",nds[i].usage);
*/
            printf("\t%12.4f\n",(float)nds[i].usage/3600.0);
        }

    }
    else{
        printf("usage file is empty.\n");
    }

    
}
