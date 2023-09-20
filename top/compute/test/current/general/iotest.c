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
   % iotest [-yod | -fyod | -enfs]
*/
   
static const char* default_file;
static const char* ydir;
static const char* mvd_file;
static const char* link_file;
static const char* default_str  = "reo: xPOE\n";

/* first 6 steps tidy things up:

   1) remove file w/ unlink()
   2) check for nonexistence w/ access()

   3) remove file from subdir w/ unlink()
   4) check for nonexistence w/ access()

   5) remove subdir w/ rmdir()
   6) check for nonexistence of subdir w/ access()

   -) remove link w/ unlink()
   -) check for nonexistence w/ access()

   ---------------------------------------------------

   7) create file w/ open() -- w/ only user read access
   8) check for existence w/ access()
   9) check for write permission w/ access()
  10) write()
  11) close()
  12) chmod() to turn off read permission
  13) open() for reading -- should fail
  14) chmod() to turn on read permission
  13) open() for reading -- should succeed

  16) lseek()
  17) read()

  18) fstat()

  19) close()

  20) stat()

   ---------------------------------------------------

  21) mkdir(subdir)
  22) rename() -- move file to subdir
  23) check for nonexistence of file in ./ w/ access()
  24) check for existence of file in subdir w/ access()
  25) rename() -- move file to ./
  26) check for existence of file in ./ w/ access()
  27) check for nonexistence of file in subdir w/ access()
  28) rmdir(subdr)
  29) check for nonexistence of subdir w/ access()
  30) link()
  31) unlink()
  32) check for nonexistence of file w/ access()
  33) test mmap() of file
*/

int main (int argc, char* argv[])
{
        int fd, i, rc, bufsiz, ch=0, valid_option=0;
        off_t offset;
        char buf;
        char *mmapbuf;

        struct stat mystat, myfstat;

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
              printf("iotest: valid options are -yod or -fyod or -enfs\n");
              exit(0);
              break;

            default:
              break;
          }
        }

        if ( valid_option != 1) {
          printf("iotest: bad command line options,\n");
          printf("iotest: valid options are -yod or -fyod or -enfs\n");
          exit(0);
        }

	printf("iotest: file to create, %s\n", default_file);
	printf("iotest: subdir to create, %s\n", ydir);
	printf("iotest: word to put in file, %s\n", default_str);

        printf("\n------------------------------------------------------------------\n\n");

        /* 1) remove specified file using unlink -- if the call
           fails becuase the file does not exist, that's still OK
        */
        rc = unlink(default_file);
        if ( rc < 0 ) {
          if ( errno != ENOENT ) {
            printf("iotest: FAILED T1 unlink(default_file) test\n");
            exit(-1);
          }
        }
        printf("iotest: PASSED T1 unlink(default_file) test\n");

        /* 2) use access to verify file does not exist... */

        rc = access(default_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T2 access(default_file,~F_OK) test -- %s should NOT exist\n", default_file);
          exit(-1);
        }
        printf("iotest: PASSED T2 access(default_file,~F_OK) test -- %s does not exist\n", default_file);

        /* 3) remove mvd file... */

        rc = unlink(mvd_file);
        if ( rc < 0 ) {
          if ( errno != ENOENT ) {
            printf("iotest: FAILED T3 unlink(mvd_file) test\n");
            exit(-1);
          }
        }
        printf("iotest: PASSED T3 unlink(mvd_file) test\n");

        /* 4) use access to verify mvd file does not exist... */

        rc = access(mvd_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T4 access(mvd_file,~F_OK) test -- %s should NOT exist\n", mvd_file);
          exit(-1);
        }
        printf("iotest: PASSED T4 access(mvd_file,~F_OK) test -- %s does not exist\n", mvd_file);

        /* 5) rmdir subdir... */
        rc = rmdir(ydir);

        if ( rc < 0) {
          if ( errno != ENOENT ) {
            printf("iotest: FAILED T5 rmdir(%s) test\n", ydir);
            exit(-1);
          }
        }

        /* 6) verify nonexistence of subdir */
        rc = access(ydir, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T6 access(ydir,~F_OK) test -- %s should NOT exist\n", ydir);
          exit(-1);
        }
        printf("iotest: PASSED T5/T6 rmdir(%s)/access test, file does not exist\n", ydir);

        printf("\n------------------------------------------------------------------\n\n");

        /* -) remove specified link using unlink -- if the call
           fails becuase the file does not exist, that's still OK
        */
        rc = unlink(link_file);
        if ( rc < 0 ) {
          if ( errno != ENOENT ) {
            printf("iotest: FAILED T- unlink(link_file) test\n");
            exit(-1);
          }
        }
        printf("iotest: PASSED T- unlink(link_file) test\n");

        /* -) use access to verify file does not exist... */

        rc = access(link_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T- access(link_file,~F_OK) test -- %s should NOT exist\n", link_file);
          exit(-1);
        }
        printf("iotest: PASSED T- access(link_file,~F_OK) test -- %s does not exist\n", link_file);

        /* 7) create file */

	fd = open(default_file,O_RDWR|O_CREAT,0666);
	if ( fd < 0 ) {
          printf("iotest: FAILED T7 open() test for file %s: fd=%d\n", default_file, fd);
	  exit(-1);
	}
        printf("iotest: PASSED T7 open() test for file %s: fd=%d\n", default_file, fd);

        /* 8) see that file exits */

        rc = access(default_file, F_OK);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T8 access(F_OK) test...\n");
          exit(-1);
        }
	printf("iotest: PASSED T8 access(F_OK) test...\n");

        /* 9) see that file is writeable */

        rc = access(default_file, W_OK);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T9 access(W_OK) test...\n");
          exit(-1);
        }
	printf("iotest: PASSED T9 access(W_OK) test...\n");


        /* 10) write the specified string */

	rc = write(fd,default_str,strlen(default_str));
        if (rc != strlen(default_str)) {
	  printf("iotest: FAILED T10 write() test, no. bytes written : %d\n",rc);
	  printf("iotest: FAILED T10 write() test, no. bytes requestd: %d\n",(int)strlen(default_str));
          exit(-1);
        }
	printf("iotest: PASSED T10 write() test, no. bytes written : %d\n",rc);

        /* 11) close */

        if ( close(fd ) < 0 ) {
	  printf("iotest: FAILED T11 close() test on file %s...\n", default_file);
          exit(-1);
        }
	printf("iotest: PASSED T11 close() test on file %s...\n", default_file);

        /* 12) chmod() to turn off read permission */

        rc = chmod(default_file, S_IWUSR );
        if ( rc < 0 ) {
	  printf("iotest: FAILED T12 chmod(S_IWUSR) test, call failed...\n");
          exit(-1);
        }

        /* 13) try to reopen read-only -- should fail */

        if ( getuid() != 0 ) {
          printf("weird: getuid()= %d\n", getuid());

	fd = open(default_file,O_RDONLY);
	if ( fd > 0 ) {
          printf("iotest: FAILED T13 open() test for file %s: fd=%d\n",default_file, fd);
	  exit(-1);
	}
        printf("iotest: PASSED T13 open() test for file %s: fd=%d\n",default_file, fd);

        }

        /* 14) chmod() to turn on read permission */

        rc = chmod(default_file, S_IWUSR | S_IRUSR );
        if ( rc < 0 ) {
	  printf("iotest: FAILED T14 chmod(S_IWUSR) test, call failed...\n");
        }

        /* 15) try to reopen read-only -- should succeed */

	fd = open(default_file,O_RDONLY);
	if ( fd < 0 ) {
          printf("iotest: FAILED T15 open() test for file %s: fd=%d\n",default_file, fd);
	  exit(-1);
	}
        printf("iotest: PASSED T12/13/14/15 chmod/open/chmod/open() test for file %s: fd=%d\n",default_file, fd);

        /* 16) lseek */

        offset = lseek(fd, 5, SEEK_SET);

        if ( offset != 5 ) {
          printf("iotest: FAILED T16 lseek() test...\n");
          exit(-1);
        }

        /* 17) read */
        
        rc = read(fd, &buf, 1);

        if ( rc != 1 ) {
          printf("iotest: FAILED T16/17 lseek/read test, read() failed...\n");
          exit(-1);
        }

        if ( buf != 'x' ) {
          printf("iotest: FAILED T16/17 lseek/read test, value read doesnt match value written...\n");
          exit(-1);
        }
        printf("iotest: PASSED T16/T17 lseek/read test...\n");

        /* 18) fstat() */
        rc = fstat(fd, &myfstat);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T18 fstat() test, call failed...\n");
          exit(-1);
        }
        if ( myfstat.st_uid != getuid() ) {
	  printf("iotest: FAILED T18 fstat() test, myfstat.st_uid= %d...\n", myfstat.st_uid);
	  printf("iotest: FAILED T18 fstat() test, getuid() = %d...\n", getuid());
          exit(-1);
        }
	printf("iotest: PASSED T18 fstat() test, myfstat.st_uid= %d...\n", myfstat.st_uid);
	printf("iotest: PASSED T18 fstat() test, getuid() = %d...\n", getuid());

        /* 19) close() */

        if ( close(fd ) < 0 ) {
	  printf("iotest: FAILED T19 close() test on file %s...\n", default_file);
          exit(-1);
        }
	printf("iotest: PASSED T19 close() test on file %s...\n", default_file);

        /* 20) stat() */
        rc = stat(default_file, &mystat);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T20 stat() test, call failed...\n");
          exit(-1);
        }
        if ( mystat.st_uid != getuid() ) {
	  printf("iotest: FAILED T20 stat() test, mystat.st_uid= %d...\n", mystat.st_uid);
	  printf("iotest: FAILED T19 stat() test, getuid() = %d...\n", getuid());
          exit(-1);
        }
	printf("iotest: PASSED T20 stat() test, mystat.st_uid= %d...\n", mystat.st_uid);
	printf("iotest: PASSED T20 stat() test, getuid() = %d...\n", getuid());

        /* 21) mkdir() */

        rc = mkdir(ydir, 0777);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T21 mkdir() test... \n");
          exit(-1);
        }
	printf("iotest: PASSED T21 mkdir() test... \n");

        /* 22) rename -- move file to subdir */

        rc = rename(default_file, mvd_file);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T22 rename() test... \n");
          exit(-1);
        }

        /* 23) check for nonexistence of file in ./ */

        rc = access(default_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T23 access(default_file,~F_OK), file should no longer exist in cwd\n");
          exit(-1);
        }

        /* 24) check for existence of file in subdir */

        rc = access(mvd_file, F_OK);
        if ( rc < 0 ) {
          printf("iotest: FAILED T24 access(mvd_file,~F_OK), file should have moved to %s\n", ydir);
          exit(-1);
        }
        printf("iotest: PASSED T22/23/24 rename() test...\n"); 

        /* 25) rename -- move file back to ./ */

        rc = rename(mvd_file, default_file);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T25 rename() test... \n");
          exit(-1);
        }

        /* 26) check for nonexistence of file in subdir */

        rc = access(mvd_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T26 access(mvd_file,~F_OK), file should no longer exist in %s\n", ydir);
          exit(-1);
        }

        /* 27) check for existence of file in ./ */

        rc = access(default_file, F_OK);
        if ( rc < 0 ) {
          printf("iotest: FAILED T27 access(default_file,~F_OK), file be back in cwd\n");
          exit(-1);
        }
        printf("iotest: PASSED T25/25/27 rename() test...\n"); 


        /* 28) rmdir(subdir) */
       
        rc = rmdir(ydir);
        if ( rc < 0 ) {
          printf("iotest: FAILED T28 rmdir(%s)...\n", ydir);
          exit(-1);
        }
        printf("iotest: PASSED T28 rmdir(%s)...\n", ydir);

        /* 29) access test to see that subdir is deleted */
        rc = access(ydir, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T29 access(%s, ~F_OK) test, subdir should have been deleted\n", ydir);
          exit(-1);
        }
        printf("iotest: PASSED T29 access(%s, ~F_OK) test, subdir deleted\n", ydir);

        /* 30) link() */
        rc = link(default_file, link_file);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T30 link() test, call failed\n");
          exit(-1);
        }

        /* 31) unlink() */
        rc = unlink(link_file);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T31 unlink() test, call failed\n");
          exit(-1);
        }

        rc = unlink(default_file);
        if ( rc < 0 ) {
	  printf("iotest: FAILED T31 unlink() test, call failed\n");
          exit(-1);
        }

        /* 32 access test to see that file is deleted */
        rc = access(default_file, F_OK);
        if ( rc == 0 ) {
          printf("iotest: FAILED T32 final access(%s, ~F_OK) test, file should have been deleted\n", default_file);
          exit(-1);
        }
        printf("iotest: PASSED T30/T31/32 final unlink/access(%s, ~F_OK) test, file deleted\n", default_file);

        /* 33 try out mmap */

        fd = open(default_file, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);

        if (!fd){
          printf("iotest: FAILED T33 mmap test, can't open %s to write (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }
 
        bufsiz=64*1024;

        mmapbuf = (char *)malloc(bufsiz);

        if (!mmapbuf){
          printf("iotest: FAILED T33 mmap test because of memory allocation failure\n");
          printf("        This is NOT an IO problem.\n");
          exit(-1);
        }

        for (i=0;  i<bufsiz; i++){
            mmapbuf[i] = i % 128;
        }

        rc = write(fd, mmapbuf, bufsiz);

        if (rc < bufsiz){
          printf("iotest: FAILED T33 mmap test, can't write %s (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }
        rc = lseek(fd, 0, SEEK_SET);

        if (rc < 0){
          printf("iotest: FAILED T33 mmap test, can't rewind %s (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }

        free(mmapbuf);

        mmapbuf = (char *)mmap(0, bufsiz, PROT_READ, MAP_PRIVATE, fd, 0);

        if (mmapbuf == (char *)MAP_FAILED){
          printf("iotest: FAILED T33 mmap of %s (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }

        for (i=0; i<bufsiz; i++){
            if (mmapbuf[i] != i % 128){
                 printf(
                     "iotest: FAILED T33 mmap of %s: mmap buffer is corrupt (%d - %d/%d)\n",
                     default_file,i,i%128,mmapbuf[i]);
                 exit(-1);
            }
        }

        rc = munmap(mmapbuf, bufsiz);

        if (rc){
          printf("iotest: FAILED T33 munmap of %s (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }

        rc = close(fd);

        if (rc ){
          printf("iotest: FAILED T33 mmap test, can't close %s (%s)\n",
                     default_file, strerror(errno));
          exit(-1);
        }

        unlink(default_file);

        printf("iotest: PASSED T33 mmap test\n");
        
        return 0;
}
