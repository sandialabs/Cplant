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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

extern int _my_rank;


/*
** $Id: bigio.c,v 1.1 2000/11/22 21:14:30 lafisk Exp $
**
**   Test reads and writes of large amounts of stuff.
**
**   Argument: {file name}.
**
**   Test will stat the file, then read the entire file into memory (if
**     there's room), then create a new file, and write the whole
**     thing back out.  The person running the test should then
**     do a diff to check that the files are the same.
**
**     Default is to read in the executable.
*/
   
char *origfile;
char newfile[MAXPATHLEN];

extern double dclock(void);

int main (int argc, char* argv[])
{
    int fd, fdnew, rc;
    struct stat statbuf;
    size_t nbytes;
    char *inbuf;
    double t1, t2;

    origfile = argv[0];

    if (argc > 1){
        origfile = argv[1];
    }

    if (_my_rank == 0){
        printf("Read/Write test using %s\n",origfile);
    
        printf("\n------------------------------------------------------------------\n\n");
    }

    /* open() */
    fd = open(origfile, O_RDONLY);
    if ( fd < 0 ) {
        printf("(%d) FAILED to open %s (%s)\n",_my_rank,origfile,strerror(errno));
        exit(-1);
    }

    /* fstat() */
    rc = fstat(fd, &statbuf);
    if ( rc < 0 ) {
        printf("(%d) FAILED fstat of %s (%s)\n",_my_rank,origfile,strerror(errno));
        exit(-1);
    }

    if (_my_rank == 0){
        printf("Size of original file %s: %ld\n",origfile,statbuf.st_size);
    }

    inbuf = (char *)malloc(statbuf.st_size);

    if (!inbuf){
        printf("(%d) FAILED malloc of size %ld (%s)\n",_my_rank,statbuf.st_size,strerror(errno));
        exit(-1);
    }

    t1 = dclock();
    nbytes = read(fd, (void *)inbuf, statbuf.st_size);

    t2 = dclock();

    if (nbytes < statbuf.st_size){
        printf("(%d) FAILED to read, rc %ld, %s\n",_my_rank,nbytes,strerror(errno));
        exit(-1);
    }
    printf("(%d) read %ld bytes, Mbytes/sec %.6g\n",  _my_rank, nbytes,
                      ((double)nbytes/1000000.0)/(t2-t1));

    sprintf(newfile,"%s.test.%d",origfile,_my_rank);

    /* open() */
    fdnew = open(newfile, O_WRONLY|O_CREAT, statbuf.st_mode);
    if ( fdnew < 0 ) {
         printf("(%d) FAILED to open %s (%s)\n",_my_rank,newfile,strerror(errno));
         exit(-1);
    }

    t1 = dclock();

    nbytes = write(fdnew, inbuf, statbuf.st_size);

    t2 = dclock();

    if (nbytes < statbuf.st_size){
        printf("(%d) FAILED to write %s, rc %ld, %s\n",_my_rank,
                  newfile,nbytes,strerror(errno));
        exit(-1);
    }
    printf("(%d) write %ld bytes, Mbytes/sec %.6g\n", _my_rank, nbytes,
                      ((double)nbytes/1000000.0)/(t2-t1));

    if (_my_rank == 0){
        printf("PASSED: write of %s completed, why don't you diff the files.\n",
                            newfile);
    }

        
    close(fd);
    close(fdnew);

    return 0;
}
