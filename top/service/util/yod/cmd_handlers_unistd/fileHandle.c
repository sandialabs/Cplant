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
/* $Id: fileHandle.c,v 1.11 2001/04/23 16:02:35 lafisk Exp $ */

#ifdef LINUX_PORTALS
#include "puma_io.h"
#else
#include <nx.h>
#endif

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileHandle.h"
#include "util.h"


fileHandle_t *fh_list_head = NULL;
fileHandle_t *fh_list_tail = NULL;

INT32 	fileHandleCount = 0;

/******************************************************************************/

fileHandle_t *
cacheOutAnyFileHandle()
{
    fileHandle_t *fh_out = NULL;

    if (fileHandleCount >= MAX_OPEN_FILES) {
	fh_out = findOpenFileHandle();
	if (fh_out) {
	    cacheOutFileHandle(fh_out);
	} else {
	    fprintf(stderr,
		"FATAL: cacheOutAnyFileHandle(): file Handle list corrupted\n");

#ifdef LINUX_PORTALS
                ioErrno = EOHHELL;
		return NULL;
#else
		kill_process();
#endif LINUX_PORTALS
	}
    }

    return(fh_out);
}

/******************************************************************************/

fileHandle_t *
cacheFileHandle(fileHandle_t *fh)
{
    fileHandle_t *fh_out = fh;

    if (!fh) {
	fprintf(stderr, "cacheFileHandle(): passed NULL file handle\n");
	ioErrno = EINVAL;
	return(fh_out);
    }

    /* if the file handle is not in the cache */
    if (fh->fd == -1) {
	/* find a file handle that is in the cache */
	if ((fh_out = findOpenFileHandle())) {
	    cacheOutFileHandle(fh_out);

	    /* if this cacheIn fails bring in the one we took out */
	    /* this can happend if the file goes away via unlink */
	    /* while we were cached out */
	    if (cacheInFileHandle(fh) == -1) {
		cacheInFileHandle(fh_out);
		ioErrno = EPROTOCOL;
		fh_out = NULL;
	    }
	} else {
	    fprintf(stderr,
		"FATAL: cacheFileHandle(): file Handle list corrupted\n");
	    printFileHandleList();

#ifdef LINUX_PORTALS
                ioErrno = EOHHELL; 
		return NULL;
#else
		kill_process();
#endif LINUX_PORTALS
	}
    }
    return(fh_out);
}

/******************************************************************************/

fileHandle_t *
checkFileHandleList(const CHAR *fname)
{

    fileHandle_t *fh;

    fh = fh_list_head;

    while (fh) {
	if (strcmp(fname, fh->fname) == 0 ) {
	    break;
	}
	fh = fh->fh_next;
    }

    return(fh);
}

/******************************************************************************/

fileHandle_t *
findOpenFileHandle(VOID)
{

    fileHandle_t *fh;

    fh = fh_list_tail;

    while (fh) {
	if (fh->fd >= 0) {
	   break;
	}
	fh = fh->fh_prev;
    }

    return(fh);
}

/******************************************************************************/

fileHandle_t *
findClosedFileHandle(VOID)
{

    fileHandle_t *fh;

    fh = fh_list_head;

    while (fh) {
	if (fh->fd == -1) {
	   break;
	}
	fh = fh->fh_next;
    }

    return(fh);
}

/*****************************************************************************/

fileHandle_t *
allocFileHandle(VOID)
{

    fileHandle_t *fh;

    fh = (fileHandle_t *) malloc(sizeof(fileHandle_t));


    if (fh == NULL) {
	fprintf(stderr, "FATAL: allocFileHandle(): out of file handles\n");
#ifdef LINUX_PORTALS
	    return NULL;
#else
	    kill_process();
#endif LINUX_PORTALS
    }

    if (DBG_FLAGS(DBG_IO_1) || DBG_FLAGS(DBG_MEMORY)) {
	fprintf(stderr, "memory: allocFileHandle(): fh %p (%d) \n", fh,sizeof(fileHandle_t));
    }

    fh->fh_next = NULL;

    if (fh_list_head == NULL) {
	/*
	** list is totally empty
	*/
	fh->fh_prev = NULL;
	fh_list_head = fh;
	fh_list_tail = fh;
    } else {
	/*
	** list not empty, just add to end of list
	*/
	fh->fh_prev = fh_list_tail;
	fh_list_tail->fh_next = fh;
	fh_list_tail = fh;
    }

    ++fileHandleCount;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "allocFileHandle(): fileHandleCount %i\n",
	    fileHandleCount);
    }
    return(fh);
}

/*****************************************************************************/

VOID
destroyFileHandle(fileHandle_t *fh)
{

    if (!fh) {
	fprintf(stderr, "destroyFileHandle(): passed NULL file handle\n");
	return;
    }

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "destroyFileHandle(fh %p)\n", fh);
    }

    if (fh->fh_prev) {
	fh->fh_prev->fh_next = fh->fh_next;
    } else {
	/* first in the list */
	fh_list_head = fh->fh_next;
    }

    if (fh->fh_next) {
	fh->fh_next->fh_prev = fh->fh_prev;
    } else {
	/* last in the list */
	fh_list_tail = fh->fh_prev;
    }
    --fileHandleCount;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "destroyFileHandle(): fileHandleCount %i\n",
	    fileHandleCount);
    }

    free(fh);

    if (DBG_FLAGS(DBG_MEMORY)){
	fprintf(stderr,"memory: %p destroyFileHandle FREED\n", fh);
    }
}

/*****************************************************************************/

INT32
cacheOutFileHandle(fileHandle_t *fh)
{
     INT32 retVal;

     if (! fh) {
	fprintf(stderr, "cacheOutFileHandle(): passed NULL file handle\n");
	return -1;
     }
     if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "cacheOutFileHandle( %p )\n", fh);
     }

     retVal = close(fh->fd);
     fh->fd = -1;
     return(retVal);
}

/*****************************************************************************/

INT32
cacheInFileHandle(fileHandle_t *fh)
{
    if (!fh) {
	fprintf(stderr, "cacheInFileHandle(): passed NULL file handle\n");
	return -1;
    }

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"cacheInFileHandle( %p )\n", fh);
    }

    /* if the open fails so will the seek, so what? */
    fh->fd = open(fh->fname, fh->accessFlags);

    /* note that this file may have been changed since this file
        was closed, we may be seeking past the end of the file,
        but if the file was changed does it really matter?,
    	I think it's the programers problem
    */
#if defined (__i386__) && !defined(LINUX_PORTALS)
	fh->curPos = _eseek(fh->fd, fh->curPos, SEEK_SET);
#else
	fh->curPos = lseek(fh->fd, fh->curPos, SEEK_SET);
#endif __i386__

    return(fh->fd);
}

/*****************************************************************************/

VOID
initStdioFileHandles()
{
    initStdioFileHandle(stdin);
    initStdioFileHandle(stdout);
    initStdioFileHandle(stderr);
}

/*****************************************************************************/

VOID
initStdioFileHandle(FILE *fp)
{

INT32		fd;
fileHandle_t 	*fh = allocFileHandle();

    if (!fp) {
	fprintf(stderr, "initStdioFileHandle(): passed NULL file pointer\n");
#ifdef LINUX_PORTALS
	    exit(0);
#else
	    kill_process();
#endif LINUX_PORTALS
    }

    if (!fh) {
	fprintf(stderr, "initStdioFileHandle(): alloc returned NULL handle\n");
#ifdef LINUX_PORTALS
	    exit(0);
#else
	    kill_process();
#endif LINUX_PORTALS
    }

    /* get a new fd for stdio, this will keep yods stdio from
    ** getting closed
    */
    fd = dup(fileno(fp));

    if (fd == -1) {
	fprintf(stderr, "initStdioFileHandle(): dup or fileno return -1\n");
#ifdef LINUX_PORTALS
	    exit(0);
#else
	    kill_process();
#endif LINUX_PORTALS
    }

    fh->fd = fd;

    /* we don't really need to do this because we ignore position
    ** on stdio files
    */
#if defined (__i386__) && !defined(LINUX_PORTALS)
	fh->curPos = _eseek(fh->fd, (off64_t)0, SEEK_CUR);
#else
	fh->curPos = lseek(fh->fd, 0L, SEEK_CUR);
#endif /* __i386__ */
    fh->otherFlags = 0;
    fh->refCount = 1;
    fh->accessFlags = fcntl(fh->fd, F_GETFL, 0) & O_ACCMODE;

    /*
    ** The following keeps us from moving the pointer, which is a problem,
    ** you get garbled output.
    */
    fh->iomode = M_LOG;

    if (fp == stdin) {
	strcpy(fh->fname, STDIN_FILE_NAME);
    } else if (fp == stdout) {
	strcpy(fh->fname, STDOUT_FILE_NAME);
    } else if (fp == stderr) {
	strcpy(fh->fname, STDERR_FILE_NAME);
    }
}

/******************************************************************************/

VOID
printFileHandle(fileHandle_t *fh)
{
    fprintf(stderr, "\nfileHandle address %p:\n", fh);

    if (fh) {
	fprintf(stderr, "\tname      = %s\n", fh->fname);
	fprintf(stderr, "\tfd        = %i\n", fh->fd);
	fprintf(stderr, "\taccess flags = 0x%x\n", fh->accessFlags);
	fprintf(stderr, "\tother flags  = 0x%x\n", fh->otherFlags);
#if defined (__i386__)
	    fprintf(stderr, "\tcurrpos   = %lli\n", fh->curPos);
#else
	    fprintf(stderr, "\tcurrpos   = %li\n", fh->curPos);
#endif  __i386__
	fprintf(stderr, "\trefCount  = %i\n", fh->refCount);
	fprintf(stderr, "\tfh_prev   = %p\n", fh->fh_prev);
	fprintf(stderr, "\tfh_next   = %p\n", fh->fh_next);
    } else {
	fprintf(stderr, "\terror NULL pointer\n");
    }
}

/******************************************************************************/

VOID
printFileHandleList()
{

    fileHandle_t *fh = fh_list_head;

    fprintf(stderr, "\nfileHandle list start:\n");

    while (fh) {
	printFileHandle(fh);
	fh = fh->fh_next;
    }

    fprintf(stderr, "\nfileHandle list end:\n");
}
