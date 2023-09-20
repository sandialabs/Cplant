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
   % fstat [-yod | -fyod | -enfs]
*/
   
static const char* default_file;
static const char* default_str  = "reo: xPOE\n";

int main (int argc, char* argv[])
{
        int fd, rc, ch=0, valid_option=0;
        struct stat myfstat;

        while( ch != EOF && valid_option==0 ) {
          ch = getopt(argc, argv, "eyfh");

          switch(ch) {
            case 'e':
              default_file = "enfs:/enfs/jsotto/io_test_file";
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
              printf("stat: valid options are -yod or -fyod or -enfs\n");
              exit(0);
              break;

            default:
              break;
          }
        }

        if ( valid_option != 1) {
          printf("fstat: bad command line options,\n");
          printf("fstat: valid options are -yod or -fyod or -enfs\n");
          exit(0);
        }

	printf("fstat: file to test: %s\n", default_file);

        printf("\n------------------------------------------------------------------\n\n");

        /* open() */
        fd = open(default_file, O_RDWR|O_CREAT,0666);
        if ( fd < 0 ) {
	  printf("fstat: FAILED open() test, call failed...\n");
          exit(-1);
        }
	printf("fstat: open() test SUCCEEDED...\n");

        /* write() */
        rc = write(fd,default_str,strlen(default_str));
        if ( rc != strlen(default_str)) {
          printf("fstat: FAILED write(), call failed...\n");
          exit(-1);
        }
        printf("fstat: write() test SUCCEEDED...\n");

        /* fstat() */
        rc = fstat(fd, &myfstat);
        if ( rc < 0 ) {
	  printf("fstat: FAILED fstat() test, called failed...\n");
          exit(-1);
        }
        if ( myfstat.st_uid != getuid() ) {
	  printf("fstat: FAILED fstat() test, myfstat.st_uid= %d...\n", myfstat.st_uid);
	  printf("fstat: FAILED fstat() test, getuid() = %d...\n", getuid());
          exit(-1);
        }
	printf("fstat: PASSED fstat() test, myfstat.st_uid= %d...\n", myfstat.st_uid);
	printf("fstat: PASSED fstat() test, getuid() = %d...\n", getuid());

        return 0;
}
