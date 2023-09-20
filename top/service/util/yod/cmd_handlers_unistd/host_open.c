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
/* $Id: host_open.c,v 1.7 2002/02/18 17:37:19 rklundt Exp $ */


#ifndef LINUX_PORTALS

#undef __COUGAR__
#include <sys/syslimits.h>	/* make sure we get the correct OPEN_MAX */
#define __COUGAR__

#endif LINUX_PORTALS

#ifdef LINUX_PORTALS
#include <puma_io.h>
#else
#include <nx.h>
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "host_open.h"
#include "fileHandle.h"
#include "util.h"

#ifdef LINUX_PORTALS
#include "host_msg.h"
#else
#include "fyodYod.h"
#endif

/* does not seem to be declared any where */  
extern int setegid(gid_t egid); 

CHAR   _UScwd[MAXPATHLEN];         /* current working directory */

static fileHandle_t *host_open_1st(CHAR *fname, INT32 flags, INT32 mode);
static fileHandle_t *host_open_otherwise(fileHandle_t *fh, INT32 flags,
	    INT32 mode);
static CHAR * getRelativePathName(CHAR *fname);
INT32 getAccessRight(CHAR *name);

/******************************************************************************/

static int
prefix(CHAR *s1, CHAR *s2)

{

INT32 i;

    i= 0;
    while ((*s1 != '\0') && (*s2 != '\0') && (*s1 == *s2))   {
        s1++;
        s2++;
        i++;
    }
    return i;
}

/******************************************************************************/

fileHandle_t *
host_open(CHAR *fname, INT32 flags, INT32 mode, INT32 uid, INT32 gid)
{

    fileHandle_t *fh;


    if (DBG_FLAGS(DBG_IO_1)) {
        fprintf(stderr, "host_open(");
	fprintf(stderr, " fname %s,",fname);
	fprintf(stderr, " flags 0x%x,", flags);
	fprintf(stderr, " mode 0%o,", mode);
	fprintf(stderr, " uid %i,", uid);
	fprintf(stderr, " gid %i, ", gid);
	fprintf(stderr, " euid %i,", geteuid());
	fprintf(stderr, " egid %i)\n", getegid());
    }

    if (iAmFyod()) {
	if (setegid(gid) == -1) {
	    fprintf(stderr, "host_open(): failed to set egid to %i, ", gid);
	    perror("");
	}
	if (setreuid(-1, uid) == -1) {
	    fprintf(stderr, "host_open(): failed to set euid to %i, ", uid);	
	    perror("");
	}
    } 

    fname = getRelativePathName(fname);

    /* check to see if the file is already open */ 
    if ((fh = checkFileHandleList(fname)) == NULL) {	
	fh = host_open_1st(fname, flags, mode);
    } else {
	fh = host_open_otherwise(fh, flags, mode);
    }

    if (iAmFyod()) {
	setreuid(-1, getuid());
	setegid(getgid());
    }
	
    if (DBG_FLAGS(DBG_IO_1)) {
    /*
	printFileHandleList();
    */
        fprintf(stderr, "host_open(): returning euid %i egid %i\n",
				geteuid(), getegid());
    }

    return(fh);

}  /* end of host_open() */

/******************************************************************************/

static fileHandle_t * 
host_open_1st(CHAR *fname, INT32 flags, INT32 mode)
{

fileHandle_t	*fh;
fileHandle_t	*fh_out; 
INT32		failed = FALSE;


    fh_out = cacheOutAnyFileHandle();

    if ((fh = allocFileHandle()) == NULL) {
	errno = ENFILE;
	return (fh);
    }

    if ((fh->fd = open(fname, flags, mode)) >= 0) {
	strcpy(fh->fname, fname);
#if defined (__i386__) & !defined(LINUX_PORTALS) 
	    fh->curPos = _eseek(fh->fd, (off64_t)0, SEEK_CUR);
#else
	    fh->curPos = lseek(fh->fd, 0L, SEEK_CUR);
#endif __i386__
	fh->otherFlags = flags & ~O_ACCMODE; 
	fh->refCount = 1;
	fh->iomode = M_UNIX;

	/*
	    Now we are going to close this file and open it again with
	    only O_RDONLY, O_WRONLY or O_RDWR flags set.

	    We do this for two reasons: 
	    
	    The first is that we want the file open for as much as the
	    user has a right (even if the user only requested O_RDONLY
	    but has O_RDWR rights, we are going to open it O_RDWR). The
	    nodes will screen read and write's based on the open flag.

	    The second is we don't want flags like O_NONBLOCK, applied
	    for all reads and write's because some nodes may not want
	    these, these flags will be taken care of during individual
	    reads and write
	*/

	close(fh->fd);

	if ((fh->accessFlags = getAccessRight(fh->fname)) >= 0) {
	    /*
	    ** If otherFlags has a combination of O_CREAT and
	    ** O_EXCL then this next open will fail because the
	    ** file has already been created. So get rid of O_CREAT
	    ** flag from otherFlags. 
	    */
	    fh->otherFlags &= ~O_CREAT;

	    if ((fh->fd = open(fname, fh->otherFlags | fh->accessFlags)) == -1){
		failed = TRUE;
	    }	

	    /*
	    ** if the original open had O_APPEND set we should move to the
	    ** correct position, because the last open did not have O_APPEND set
	    */
#if defined (__i386__) & !defined(LINUX_PORTALS)
		_eseek(fh->fd, fh->curPos, SEEK_SET);
#else
		lseek(fh->fd, fh->curPos, SEEK_SET);
#endif __i386__
	} else {
	    /* if this is negative it means we don't have read or
	    ** write priv. how the first open succeeded, I don't
	    ** know but we will consider this a failure
	    */
	    failed = TRUE;
	}
    } else {
	failed = TRUE;
    }

    /* something went wrong */ 
    if (failed) {
	if (fh_out) {
	    cacheInFileHandle(fh_out);
	}
	destroyFileHandle(fh);
	fh = NULL;
    }	

    return(fh);
}

/******************************************************************************/

static fileHandle_t *
host_open_otherwise(fileHandle_t *fh, INT32 flags, INT32 mode)
{

INT32		failed = FALSE;
fileHandle_t	*fh_out;


    /* 
    ** allow many nodes to open with O_EXCL, the first one gets
    ** a good file descriptor and the rest get an appropriate 
    ** failure code and do not destroy the original file handle.
    */
    if ((fh->otherFlags & O_EXCL) == O_EXCL) {
        errno = EEXIST;
        return(NULL);
    }


    if ((fh_out = cacheFileHandle(fh)) == NULL) {
	return(NULL);
    }

    /*
    ** close this file so we can open it and check it against the
    ** requested flags and mode
    */
    close(fh->fd);	

    /*
    ** open the file with the flags and mode of the new request, so we
    ** can verify it, I chose to do it this way becuse some of the flags
    ** may be O_TRUNC or O_CREAT which will effect the currently file,
    ** and this is the easyest way implementing these flags
    */
    if ((fh->fd = open(fh->fname, flags, mode)) >= 0) {
	/*
	** save the current position it may be at the end of the
	** file if O_APEND was set
	*/
#if defined (__i386__) & !defined(LINUX_PORTALS)
	    fh->curPos = _eseek(fh->fd, (off64_t)0, SEEK_CUR);
#else
	    fh->curPos = lseek(fh->fd, 0L, SEEK_CUR);
#endif
	fh->otherFlags = flags & ~O_ACCMODE; 

	/*
	** get the access right again they may have changed since the
	** last time they were saved
	*/
	fh->accessFlags = getAccessRight(fh->fname);

	/*
	** now we are going to close this file and open it with only the
	** access mode flags, we don't want it opened with flags like
	** O_NONBLOCK, because some processes may not want these so we
	** take care of these flags during individual reads and writes
	*/
	close( fh->fd );

	if ((fh->accessFlags = getAccessRight(fh->fname)) >= 0) {
	    fh->fd = open(fh->fname, fh->otherFlags | fh->accessFlags);
	    if (fh->fd == -1) {
		failed = TRUE;
	    } else {
		/*
		** if the original open had O_APPEND set we should move
		** to the correct position, because the last open did not
		** have O_APPEND set
		*/
#if defined (__i386__) & !defined(LINUX_PORTALS)
		    _eseek(fh->fd, fh->curPos, SEEK_SET);
#else
		    lseek(fh->fd, fh->curPos, SEEK_SET);
#endif __i386__

		/*
		** we are open for sure now we can increment the
		** reference count
		*/
		++fh->refCount;
	    }
	} else {
	    /*
	    ** if this is negative it means we don't have read or write 
	    ** priv. how the first open succeeded, I don't know but we 
	    ** will consider this a failure
	    */
	    fh->fd = -1;
	    failed = TRUE;
	}
    } else {
	/* this open() request failed */ 
	failed = TRUE;
    }

    /* something went wrong */ 
    if (failed) {
	if (fh != fh_out) {
	    cacheInFileHandle(fh_out);
	}
	destroyFileHandle(fh);
	fh = NULL;
    }
    return(fh);
}

/******************************************************************************/

static CHAR *
getRelativePathName(CHAR *fname)
{

INT32 i;


    /*
    ** If it's in the current working directory, then open a relative
    ** path name instead of an absolute one.
    */
    i = prefix(_UScwd, fname);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "getRelativePathName(fname %s)\n", fname);
    }

    return(fname);

    /**
    *** The code below is not needed????? Can somebody explain this?
    **/



    if (i == 1) {
	/* Only the leading slash matches, Use absolute pathname */
	i = 0;                  
    } else if (fname[i] != '/')	{
	/* _UScwd is not a parent of this pathname */
	i = 0;
    } else {
	/* Bypass the slash */
	i++;                    
    }

    if (i > 0) {
	fname += i; 
    } else {
	fname = 0;
    }

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "getRelativePathName() is %s\n", fname);
    }

    return(fname);

}  /* end of getRelativePathName() */

/******************************************************************************/

INT32
getAccessRight(CHAR *name)
{

INT32 flag = -1;


    if (access( name, R_OK | W_OK) == 0) {
	flag = O_RDWR;
    } else if (access(name, R_OK) == 0) { 
	flag = O_RDONLY;
    } else if (access(name, W_OK) == 0) {
	flag = O_WRONLY;
    }

    return(flag);
}
