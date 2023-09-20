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
/* mver.c - A sfyod test program */

/*
#include "qkdefs.h"

TITLE(mver_c, "@(#) $Id: mver.c,v 1.8 1999/09/01 22:56:13 wmdavid Exp $");
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>

#ifdef __linux__
/* Assume we have puma includes available */
#include "puma_errno.h"
#include "puma.h"
#else
/* Must be osf; use my include file */
#include "cplant.h"
#endif /* __linux__ */

#include "cplantDebug.h"
#include "cplantError.h"
#include "parseOptions.h"

#define Kvalue (1024)
#define Mvalue (Kvalue*Kvalue)

/* Change this one to enable/disable debug output */
#define DEFAULT_DEBUG_LEVEL (0)		/* debug level variable (-D) */

#define WRITE
#define READ
#define DEBUG_LEVEL 3		/* Messages at this level */
#define MAX_USAGE 256
/* Debug bit definitions in top/service/util/yod/include/util.h. */
#define DEFAULT_MULTI 2
/*#define DEFAULT_FNAME "/scsi_010/work/test.file.wmd" /* */
#define DEFAULT_SNAME "/sfs/wmdavid/test.file"
#define DEFAULT_ZNAME "/zfs/wmdavid/test.file"
#define DEFAULT_BLOCK (1*Mvalue)
#define DEFAULT_INNER 1
#define DEFAULT_OFFSET 0x01000000
typedef int datatype;

/* This uses  O P E N	*/

/* This is the multi node Write and Verify Read Program */

/*
#define BLSZ 8*Kvalue
#define BLSZ 16*Kvalue
#define BLSZ 32*Kvalue
#define BLSZ 64*Kvalue
*/
#define BLSZ 1*Mvalue

/*
int FALSE=0;
int TRUE=1;
*/

/* options */
enum {
	debug_OPTION,
	multi_OPTION,
	fname_OPTION,
	block_OPTION,
	inner_OPTION,
	seperate_OPTION,
	nowrite_OPTION,
	noread_OPTION,
	offset_OPTION,
	help_OPTION,
	OPTION_END
};
#define OPTIONS OPTION_END

argData options[OPTIONS];

/* Some debug flags used by other libraries */
int Dbgflag = 0;

defineDebugVar(DEFAULT_DEBUG_LEVEL);

extern double dclock(void);

static void displayError(datatype *array, int index, int expect, int range)
{
	int istart;
	int i;

	fprintf(stderr, "At %1d, data=%d, should be %d\n",
	    index, array[index], expect);

	istart = index - range;

	for (i=0; i<2*range; i++)
	{
		fprintf(stderr, "Index: %d = %d\n", istart, array[istart]);
		istart++;
	}

}
		
int main(int argc, char *argv[])
{
	static char fname[] = "main";
char usage[MAX_USAGE];
datatype *a;
datatype *b;
FILE *str;
int fd;
int i, nr;
int rc;
double t0;
double elapsed;
int bytes=0;
int dowrite;
int mult;
char filn[128];
int rlist[1001];
unsigned int tseed;
int last;
int iam;
char *filename;
int debugLevel;
int len;
int block;
int innerLoop;
int count;
int offset;
int expect;
int lastblock;
int intCounter;
char *sp;

/* Make an usage string */
	sprintf(usage, "Usage: %s\n", argv[0]);

	initOption(options[debug_OPTION], "debug|D", OPTION_DECIMAL,
		"Enable debug");

	initOption(options[multi_OPTION], "multi|M", OPTION_DECIMAL,
		"Specify multiplier value");

	initOption(options[fname_OPTION], "fname|f", OPTION_STRING,
		"Specify file name");

	initOption(options[block_OPTION], "block|b", OPTION_HEX,
		"Specify block size");

	initOption(options[inner_OPTION], "inner|i", OPTION_DECIMAL,
		"Specify inner loop");

	initOption(options[seperate_OPTION], "seperate|s", OPTION_BINARY,
	    "Each node opens unique filename");

	initOption(options[nowrite_OPTION], "nowrite|w", OPTION_BINARY,
	    "No writes are perfomed");

	initOption(options[noread_OPTION], "noread|r", OPTION_BINARY,
	    "No reads are perfomed");

	initOption(options[offset_OPTION], "offset|o", OPTION_DECIMAL,
	    "Specify starting data offset");

	initOption(options[help_OPTION], "help", OPTION_BINARY,
		"Prints this message");

	if ( parseOptions(&argc, &argv, options, OPTIONS) == ERROR )
	{
		error(fname, EWARNING, "parseOptions() failed");
		parseOptionsUsage(usage, options, OPTIONS);
		exit(-1);
	}

	if ( testOption(options[help_OPTION]) )
	{
		if ( _my_nid == 0 )
			parseOptionsUsage(usage, options, OPTIONS);
		exit(-1);
	}

	debugLevel = getIntValue(options[debug_OPTION], DEFAULT_DEBUG_LEVEL);
	setDebugVar(debugLevel);
	Dbgflag = debugLevel;

	mult = getIntValue(options[multi_OPTION], DEFAULT_MULTI);

	if ( testOption(options[seperate_OPTION]) )
	    filename = getStrValue(options[fname_OPTION], DEFAULT_ZNAME);
	else
	    filename = getStrValue(options[fname_OPTION], DEFAULT_SNAME);

	block = getIntValue(options[block_OPTION], DEFAULT_BLOCK);

	innerLoop = getIntValue(options[inner_OPTION], DEFAULT_INNER);
	offset = getIntValue(options[offset_OPTION], DEFAULT_OFFSET);

/* PROCESS PARAMETERS */
  dowrite=TRUE;
  iam = _my_nid;

  a = (datatype *)malloc(block);
  b = (datatype *)malloc(block);
  if ( !a || !b )
  	error(fname, EFATAL, "malloc() failed");

/* SET FILE NAME and Do The OPEN */
	t0 = dclock();

	strcpy(filn, filename);

/* Make changes to filename dependent on rank number */
	if ( testOption(options[seperate_OPTION]) )
	{
	    len = strlen(filn);
	    sp = filn + len;
	    sprintf(sp, "%03d", _my_nid);
	}

#if 0
    if (strncmp( filn, "/scsi_", 6) == 0 )
       filn[6]='0'+ (_my_nid & 0x1);
#endif /* 0 */

	vdebug(s, filn);
    fd = open(filn, O_RDWR|O_CREAT|O_TRUNC, 0644 );
	elapsed = dclock()-t0;

  	vdebug(f, elapsed);

	vdebug(d, fd);

	if ( fd == -1)
	{
		vdebug(d, ERRNO);
		perror("open()");
		vdebugl(0, s, filn);
		error(fname, EFATAL, "open() failed: %d", _my_nid);
	}
	debug("open() done");

#ifdef WRITE
/*	DO THE   WRITE */

	count = block/sizeof(datatype);

    last = (block/8)-1;
    tseed = (unsigned int)time(0);
    srand(tseed);
	vdebug(x, tseed);
	vdebug(d, rand());

    for (i=0; i<1001; i++)
       rlist[i] = rand();


	nr=1000;
	t0=dclock();
	lastblock=0;
	if ( !testOption(options[nowrite_OPTION]) )
	{
    	    for ( i=0; i<innerLoop ; i++ )
	    {
		for (intCounter=0; intCounter<count; intCounter++)
	    	    a[intCounter] =  offset * _my_nid + intCounter + lastblock;

		lastblock += intCounter;
		if (dowrite)
		{
			errno = 0;
			debug_level(DEBUG_LEVEL, "Entering write: %d", _my_nid);
			vdebug(d, block);
			rc = write(fd, a, block);
			if ( rc != block)
			{
				vdebug(d, rc);
				vdebug(x, ERRNO);
				perror("write()");
				error(fname, EFATAL, "write() failed: %d",
				_my_nid);
			}
		}

		bytes += block;
		elapsed = dclock() - t0;
#ifdef TIMEOUT
		if ( elapsed  > 30 )
		{
			nr=i+1;
			break;
		}
#endif /* TIMEOUT */

    	    }

  fprintf(stderr, "Done writing: %d %f Megabytes/sec\n",
  	_my_nid, bytes/(elapsed*Mvalue));
	}
#endif /* WRITE */

  close (fd);
  debug("close() done");
  vdebug(d, nr);

/*	ReOPEN the File and READ   and     VERIFY */
	bytes = 0;
    fd = open(filn, O_RDWR );

	vdebug(d, fd);
	if ( fd == -1)
	{
   	    perror("open()");
	    vdebugl(0, s, filn);
	    error(fname, EFATAL, "open() failed: %d", _my_nid);
	}

	debug("open() done");

#ifdef READ
	t0=dclock();
	lastblock = 0;
	if ( !testOption(options[noread_OPTION]) )
	{
	    for ( i=0; i<innerLoop ; i++ )
	    {
		errno = 0;
		debug_level(DEBUG_LEVEL, "Entering read: %d", _my_nid);
		vdebug(d, block);
		rc = read(fd, b, block);
		if ( rc != block)
		{
			vdebug(d, rc);
			vdebug(x, ERRNO);
			perror("read()");
			error(fname, EFATAL, "read() failed: %d", _my_nid);
		}

		bytes += block;
		elapsed = dclock() - t0;
#ifdef TIMEOUT
		if ( elapsed  > 30 )
		{
			nr=i+1;
			break;
		}
#endif /* TIMEOUT */


/* Do a read check */
    	    for (intCounter=0; intCounter<count; intCounter++)
    	    {
    		expect = offset * _my_nid + intCounter + lastblock;
    		if ( b[intCounter] != expect )
       		{
    		    displayError(b, intCounter, expect, 3);
    		    break;
    		}
    	    }

	    lastblock += intCounter;
      	}

	fprintf(stderr, "Done reading: %d %f Megabytes/sec\n",
		_my_nid, bytes/(elapsed*Mvalue));

	}
#endif /* READ */

	close (fd);
	debug("close() done");

	return 0;
}
