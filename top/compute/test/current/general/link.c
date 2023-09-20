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
   % link [-yod | -fyod | -enfs]
*/
static const char* TNAME = "link";
   
static const char* default_file=NULL;
static const char* link_file;

/* first 4 steps tidy things up:

   1) remove file w/ unlink()
   2) check for nonexistence w/ access()

   3) remove link w/ unlink()
   4) check for nonexistence of link w/ access()

   ---------------------------------------------------

   5) create file w/ open() -- w/ only user read access
   6) check for existence w/ access()

   7)...individual test

   ---------------------------------------------------
*/

int main (int argc, char* argv[])
{
        int fd, rc, ch=0, valid_option=0;

        while( ch != EOF  && valid_option==0  ) {
          ch = getopt(argc, argv, "eyfh");

          switch(ch) {
            case 'e':
              default_file = "enfs:/enfs/tmp/io_test_file";
              link_file    = "enfs:/enfs/tmp/io_link_file";
              valid_option = 1;
              break;

            case 'y':
              default_file = "./io_test_file";
              link_file    = "./io_link_file";
              valid_option = 1;
              break;

            case 'f':
              default_file = "/raid_010/tmp/io_test_file";
              link_file    = "/raid_010/tmp/io_link_file";
              valid_option = 1;
              break;

            case 'h':
              printf("%s: valid options are -yod or -fyod or -enfs\n", TNAME);
              exit(0);
              break;

            default:
              break;
          }
        }

        if ( valid_option != 1) {
          printf("%s: bad command line options,\n", TNAME);
          printf("%s: valid options are -yod or -fyod or -enfs\n", TNAME);
          exit(0);
        }

	printf("%s: file to create, %s\n", TNAME, default_file);
	printf("%s: link to create, %s\n", TNAME, link_file);

        printf("\n------------------------------------------------------------------\n\n");

        /* 1) remove specified file using unlink -- if the call
           fails becuase the file does not exist, that's still OK
        */
        rc = unlink(default_file);
        if ( rc < 0 ) {
          if ( errno != ENOENT ) {
            printf("%s: FAILED T1 unlink(default_file) test\n", TNAME);
            exit(-1);
          }
        }
        printf("%s: PASSED T1 unlink(default_file) test\n", TNAME);

        /* 2) use access to verify file does not exist... */

        rc = access(default_file, F_OK);
        if ( rc == 0 ) {
          printf("%s: FAILED T2 access(default_file,~F_OK) test -- %s should NOT exist\n", TNAME, default_file);
          exit(-1);
        }
        printf("%s: PASSED T2 access(default_file,~F_OK) test -- %s does not exist\n", TNAME, default_file);

        /* 3) remove specified link using unlink -- if the call
           fails becuase the file does not exist, that's still OK
        */
        rc = unlink(link_file);
        if ( rc < 0 ) {
          if ( errno != ENOENT ) {
            printf("%s: FAILED T3 unlink(link_file) test\n", TNAME);
            exit(-1);
          }
        }
        printf("%s: PASSED T3 unlink(link_file) test\n", TNAME);

        /* 4) use access to verify link does not exist... */

        rc = access(link_file, F_OK);
        if ( rc == 0 ) {
          printf("%s: FAILED T4 access(link_file,~F_OK) test -- %s should NOT exist\n", TNAME, link_file);
          exit(-1);
        }
        printf("%s: PASSED T4 access(link_file,~F_OK) test -- %s does not exist\n", TNAME, link_file);

        /* 5) create file */

	fd = open(default_file,O_RDWR|O_CREAT,0666);
	if ( fd < 0 ) {
          printf("%s: FAILED T5 open() test for file %s: fd=%d\n", TNAME, default_file, fd);
	  exit(-1);
	}
        printf("%s: PASSED T5 open() test for file %s: fd=%d\n", TNAME, default_file, fd);

        /* 6) see that file exits */

        rc = access(default_file, F_OK);
        if ( rc < 0 ) {
	  printf("%s: FAILED T6 access(F_OK) test...\n", TNAME);
          exit(-1);
        }
	printf("%s: PASSED T6 access(F_OK) test...\n", TNAME);

/*--------------------------------------------------------------------------*/

        /* 7) link() */
        rc = link(default_file, link_file);
        if ( rc < 0 ) {
	  printf("%s: FAILED T7 link() test, call failed\n", TNAME);
          exit(-1);
        }

        printf("%s: PASSED T7 link() test\n", TNAME);

        return 0;
}
