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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

/* usage: 
   % trunc [-yod | -fyod | -enfs]
*/
   
static const char* default_file;
static const char* default_str  = "reo: xPOE\n";

static struct stat *buf;

int main (int argc, char* argv[])
{
        int fd, rc, ch=0, valid_option=0;

        while( ch != EOF  && valid_option==0  ) {
          ch = getopt(argc, argv, "eyfh");

          switch(ch) {
            case 'e':
              default_file = "enfs:/enfs/tmp/io_test_file";
              valid_option = 1;
              break;

            case 'y':
              default_file = "./io_test_file";
              valid_option = 1;
              break;

            case 'f':
              default_file = "/raid_010/tmp/io_test_file";
              valid_option = 1;
              break;

            case 'h':
              printf("trunc: valid options are -yod or -fyod or -enfs\n");
              exit(0);
              break;

            default:
              break;
          }
        }

        if ( valid_option != 1) {
          printf("trunc: bad command line options,\n");
          printf("trunc: valid options are -yod or -fyod or -enfs\n");
          exit(0);
        }

	printf("trunc: file to test: %s\n", default_file);

        printf("\n------------------------------------------------------------------\n\n");

        /* open() */
        fd = open(default_file, O_RDWR|O_CREAT,0666);
        if ( fd < 0 ) {
	  printf("trunc: FAILED open() test, call failed... %s\n",strerror(errno));
          exit(-1);
        }
	printf("trunc: open() test SUCCEEDED...\n");

        /* write() */
        rc = write(fd,default_str,strlen(default_str));
        if ( rc != strlen(default_str)) {
          printf("trunc: FAILED write(), call failed... %s\n",strerror(errno));
          exit(-1);
        }
        printf("trunc: write() test SUCCEEDED...\n");

        rc = close(fd);
        if ( rc < 0 ) {
          printf("trunc: close() FAILED... %s\n",strerror(errno));
          exit(-1);
        }
        printf("trunc: close() SUCCEEDED...\n");


        /* truncate */
        rc = truncate(default_file, 4);
        if ( rc < 0 ) {
          printf("trunc: truncate() FAILED... %s\n",strerror(errno));
          exit(-1);
        }

        printf("trunc: truncate() of %s SUCCEEDED?\n", default_file);
        printf("trunc: check to see if file is 4 bytes long...\n");


        return 0;
}
