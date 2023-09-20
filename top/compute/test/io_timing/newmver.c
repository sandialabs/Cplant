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
**  $Id   $
**
**	This is multi node Write and Verify Read program for
**	use testing FYOD.
**		It is an "app" and is supposed to work both
**			From a DU user environment  and
**			From a Linux developer environment.
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>   /* errno comes from  linux/include/asm */
#ifdef LINUX_PORTALS
#include <puma_errno.h>
#include <puma.h>
#else
#define _my_nid      0
#define TRUE         1
#endif
#include <strings.h>

/* 			This uses  O P E N	*/

/*		This is the multi node Write and Verify Read Program
	*/
int jvd_acks;
int johns_dbflag;

main(int argc, char *argv[])
{
/*
#define BLSZ 8192
#define BLSZ 16384
#define BLSZ 32768
#define BLSZ 65536
*/

/*
int FALSE=0;
int TRUE=1;
*/
#define BLSZ 65536
long a[BLSZ];
long b[BLSZ];
FILE *str;
int fd;
int i, j, nr;
long o;
int rc;
int t0, elapsed, bytes=0;
int dowrite;
int wsize=1024;
int mult=2;
char filn[128];
char fid[]="0";  
int rlist[1001];
uint tseed;
int last;
int iam;
char *c;
int file_err;

/*
**	Note that this program accepts three optional parameters
**	They are order dependent
**	First is a file name to write to
**	Second is a -D which turns on some debugging in the 
**	   program and in the library
**	Third is -M ##, which specifies a buffer multiplier.
**	Hence the invocation line is
**		yod [yod parameters] timit [dest_file] [-D] [-M #]
**	where "timit" is the name created for the executable.
**      -M added read with caution
**
**	This version is supposed to work either in a Linux developers
**	environment or a DU applications environment.
*/
/*
			PROCESS PARAMETERS
	*/
  printf("  VERSION --  July  1999    Writes and reads \n");
  dowrite=TRUE;
  fid[0]='a'+_my_nid;
  iam = _my_nid;

  for ( i=argc; i>1; i-- ) {
/*  printf(" FAILING  %d  %s %d \n", i, argv[i], argc);  */
    if ( strncmp( argv[i-1] , "-M", 2) == 0 ) {
       mult=atoi(argv[i]);
       argc--;
       argc--;
       }
     }
  printf("%d  John s Multiplier is %d  (argc is %d) \n", iam, mult, argc);
  wsize = mult*wsize;
  if ( wsize > 8*BLSZ )
	   wsize = 8*BLSZ;
  printf("%d  Block size is %d bytes \n", iam, wsize);

  johns_dbflag=0;
  for ( i=1; i<= argc; i++ ) {
    if ( strncmp( argv[i] , "-D", 2) == 0 ) {
       johns_dbflag = 1;
       printf(" John s  Debug Flag set to one \n");
       argc--;
       break;
       }
     }

  i=0;
  if (johns_dbflag == 1)
       printf(" Using Open,  First Parameter is %s \n", argv[1]);

	/*
			SET FILE NAME and Do The OPEN
	*/
  t0=(int)time(NULL);
  if ( argc == 1 ) {
    (void)strcpy(filn, "/scsi_051/work/test.file.a");
    }
  else {
    strcpy(filn, argv[1]);
    }
  strcat(filn, fid);
  if (strncmp( filn, "/scsi_", 6) == 0 ) {
     filn[8]='0'+ (_my_nid & 0x1);
  }
  printf("%d Will use file -- %s\n", iam, filn);
  fd = open(filn, O_RDWR|O_CREAT, S_IRWXU );
  elapsed = (int)time(NULL)-t0;

  printf("%d Open took %d seconds \n",iam, elapsed);

  if (johns_dbflag == 1)
     printf("%d After open  %d \n", iam, fd);
  if ( fd == -1) {
     printf("%d File OPEN failed \n\n", iam);
#ifdef LINUX_PORTALS
     printf("%d errno, ERRNO %d %d \n", iam, errno, ERRNO);
#else
     printf("%d errno %d \n", iam, errno);
     file_err = errno;
#endif
     if ( file_err == ENOENT ) {
        printf(" ENOENT - no file or directory\n");
     }
     if ( file_err == EREMOTEIO ) {
        printf(" EREMOTEIO - Remote I/O error\n"
        "\t\tWhich includes, can't get to that unit\n");
     }

     exit (-1);
     }

/*			DO THE   WRITE
	*/

    last = (wsize/8)-1;
    tseed = (uint)time(NULL);
    srand(tseed);
  if (johns_dbflag == 1) {
    printf("%d Random Seed is %x\n", iam, tseed);
    printf("%d First Random number is %d \n", iam, rand());
    }
    for (j=0; j<1001; j++)
       rlist[j] = rand();

    for (j=0; j< BLSZ; j++)
       a[j] =  j ;

  nr=1000;
  t0=(int)time(NULL);
  for ( i=0; i< 1000 ; i++ ) {
    a[0]= rlist[i];
    a[last] = rlist[1000-i];
    a[last-31]= rlist[i];

    if (dowrite) {
      errno = 0;
      rc = write(fd, a, wsize);
      if ( rc != wsize) {
        printf("%d Write failed %d vs. %d\n", iam, rc, wsize);
	printf("%d errno,  %d \n", iam, errno);
#ifdef LINUX_PORTALS
	if (ERRNO != 0)
	   printf("  ERRNO %d \n", ERRNO);
#endif
	}
      }
    bytes += wsize;
    if ( (elapsed = (int)time(NULL)-t0) > 30 ) {
           nr=i+1;
	   break;
           }
    }
  printf("%d  Done writting\n", iam);
  printf("%d --  %d bytes in %d seconds\n", iam, bytes, elapsed);
  if (elapsed == 0)
	elapsed = 1;
  printf(" \t\t%d  Data Rate      %f  Megabytes/sec \n", iam,
		1.0e-6 * (float)bytes / (float)elapsed );

  close (fd);
  printf("%d Number of records written %d\n", iam, nr);
  printf (" \t\t%d --  %d  bytes \n\n", iam, bytes);

/*			ReOPEN the File and 
		   READ   and     VERIFY
	*/
  bytes = 0;
    fd = open(filn, O_RDWR );

  printf("%d After open for read  %d \n", iam, fd);
  if ( fd == -1) {
     printf(" Took the path to EXIT \n\n");
     exit (-1);
     }

  t0=(int)time(NULL);
  for ( i=0; i<nr ; i++ ) {
    if (dowrite) {
      if (johns_dbflag == 1)
 	printf("%d Begin read number %d\n", iam, i+1);
      rc = read(fd, b, wsize);
      bytes += rc;
      if (johns_dbflag == 1)
	printf("%d Read %d bytes\n", iam, rc);
      if ( rc != wsize ) {
        printf("%d Read terminated %d\n", iam, rc);
	break;
        }
        if ((b[0] != rlist[i]) || (b[last] != rlist[1000-i]) ||
		(b[last-31] != rlist[i]) ) {
	   printf(" \n%d Compare failed record %d \n\n", iam, i);
	   printf(" %d %d %d  \n", b[0], b[last], b[last-31] );
	   printf(" %d %d %d \n", rlist[i], rlist[1000-i], rlist[i]);
	   printf(" %d %d %d \n\n", b[1], b[2], b[3]);
	   break;
	   }
      }
    if ( (elapsed = (int)time(NULL)-t0) > 40 )
	   break;
    }
  printf("%d  Done reading\n", iam);
  printf("%d --  %d bytes in %d seconds\n", iam, bytes, elapsed);
  if (elapsed == 0)
	elapsed = 1;
  printf(" \t\t%d  Data Rate      %f  Megabytes/sec \n", iam,
		1.0e-6 * (float)bytes / (float)elapsed );

  close (fd);

  printf("%d  all done %d\n", iam, jvd_acks);
  return 0;
}
