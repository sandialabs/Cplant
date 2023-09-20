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
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

char *temp;

/* usage: 
   % stdio [-yod | -fyod | -enfs]
*/
   
static const char* default_file;
static const char* ydir;
static const char* mvd_file;
static const char* link_file;
static const char* default_str  = "reo: xPOE\n";

   /*---------------------------------------------------

   1) create file w/ fopen() -- w/ only user read access
   2) check for existence w/ access()
   3) check for write permission w/ access()
   4) fwrite()
   5) fclose()
   6) fseek()
   7) fread()

   ---------------------------------------------------*/

int main (int argc, char* argv[])
{
        int rc, ch=0, valid_option=0;
        int off;
        long offset;
        char buf;
        FILE *file;
        size_t rr;

/*      register_io_proto(ENFS_IO_PROTO, &io_ops_yod, "fudge:"); */

        while( ch != EOF  && valid_option==0  ) {
          ch = getopt(argc, argv, "eyfh");

          switch(ch) {
            case 'e':
              default_file = "enfs:/enfs/tmp/io_test_file";
              ydir         = "enfs:/enfs/tmp/io_test_dir";
              mvd_file     = "enfs:/enfs/tmp/io_test_dir/io_test_file";
              link_file    = "enfs:/enfs/tmp/io_link_file";
              valid_option = 1;
              break;

            case 'y':
              default_file = "./io_test_file";
              ydir         = "./io_test_dir";
              mvd_file     = "./io_test_dir/io_test_file";
              link_file    = "./io_link_file";
              valid_option = 1;
              break;

            case 'f':
              default_file = "/raid_010/tmp/io_test_file";
              ydir         = "/raid_010/tmp/io_test_dir";
              mvd_file     = "/raid_010/tmp/io_test_dir/io_test_file";
              link_file    = "/raid_010/tmp/io_link_file";
              valid_option = 1;
              break;

            case 'h':
              printf("stdio: valid options are -yod and -fyod\n");
              exit(0);
              break;

            default:
              break;
          }
        }

        if ( valid_option != 1) {
          printf("stdio: bad command line options,\n");
          printf("stdio: valid options are -yod and -fyod\n");
          exit(0);
        }

	printf("stdio: file to create, %s\n", default_file);
	printf("stdio: subdir to create, %s\n", ydir);
	printf("stdio: word to put in file, %s\n", default_str);

        printf("\n------------------------------------------------------------------\n\n");

        /* 1) fopen file */
     
        file = fopen(default_file,"w+");
	if ( file == NULL ) {
          printf("stdio: FAILED T1 fopen() test for file %s\n", default_file);
	  exit(-1);
	}
        printf("stdio: PASSED T1 fopen() test for file %s\n", default_file);

        /* 2) see that file exits */

        rc = access(default_file, F_OK);
        if ( rc < 0 ) {
	  printf("stdio: FAILED T2 access(F_OK) test...\n");
          exit(-1);
        }
	printf("stdio: PASSED T2 access(F_OK) test...\n");

        /* 3) see that file is writeable */

        rc = access(default_file, W_OK);
        if ( rc < 0 ) {
	  printf("stdio: FAILED T3 access(W_OK) test...\n");
          exit(-1);
        }
	printf("stdio: PASSED T3 access(W_OK) test...\n");


        /* 4) fwrite the specified string */

	rc = fwrite(default_str,1,strlen(default_str),file);
        if (rc != strlen(default_str)) {
	  printf("stdio: FAILED T4 fwrite() test, no. bytes written : %d\n",rc);
	  printf("stdio: FAILED T4 fwrite() test, no. bytes requestd: %d\n",(int)strlen(default_str));
          exit(-1);
        }
	printf("stdio: PASSED T4 fwrite() test, no. bytes written : %d\n",rc);

#if 0
        /* 5) fclose */

        if ( fclose(file ) != 0 ) {
	  printf("stdio: FAILED T5 fclose() test on file %s...\n", default_file);
          exit(-1);
        }
	printf("stdio: PASSED T5 fclose() test on file %s...\n", default_file);
#endif

        /* 6) fseek */

        off = fseek(file, 5, SEEK_SET);

        if ( off != 0 ) {
          printf("stdio: FAILED T6 fseek() test...\n");
          perror("stdio: fseek failure --");
          exit(-1);
        }
        printf("stdio: PASSED T6 fseek() test...\n");

        /* 6.1) ftell */

        offset = ftell( file);

        if ( offset != 5 ) {
          printf("stdio: FAILED T6.1 ftell() test...\n");
          perror("stdio: ftell failure --");
          exit(-1);
        }
        printf("stdio: PASSED T6.1 ftell() test: %ld...\n", offset);

        /* 7) fread */
        
        rr = fread(&buf,1,1,file);

        if ( rr != 1 ) {
          printf("stdio: FAILED T6/7 fseek/fread test, fread(): %d\n", 
                                                           (int) rr);
          perror("stdio: fread failure --");
          exit(-1);
        }
        printf("stdio: PASSED T7 fread() returned ok: %d...\n", (int) rr);

        if ( buf != 'x' ) {
          printf("stdio: FAILED T6/7 fseek/fread test, value read doesnt match value written\n");
          exit(-1);
        }
        printf("stdio: PASSED T6/T7 fseek/fread test...\n");
        return 0;
}
