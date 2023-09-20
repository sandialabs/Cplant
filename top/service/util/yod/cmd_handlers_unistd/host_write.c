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
/* $Id: host_write.c,v 1.10 2001/02/16 05:44:26 lafisk Exp $ */


#include "puma.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "host_write.h"
#include "positionFilePtr.h"
#include "host_msg.h"
#include "fileHandle.h"
#include "util.h"
#include "fillWorkBuf.h"
#include "srvr_comm.h"

static ssize_t host_write2(INT32 fd, size_t nbytes, 
                   control_msg_handle *mh, int *hostErrno);

#ifdef SUPPRESS_DEC_FORTRAN_RTL

int rtlmessages=0;

static int
is_forrtl_excess(int len, int bufnum)
{
unsigned char *c;
int excess;

    excess = 0;

    /*
    ** Fortran Applications built under DEC OSF spew
    ** out fortran run time library error messages when
    ** something goes wrong.  Let's not display all of these.
    */
    if (!(DBG_FLAGS(DBG_FORRTL)) && (len > 7)){

	c = (unsigned char *)workBufData(bufnum);

	if (( *c++ == 'f') &&
	    ( *c++ == 'o') &&
	    ( *c++ == 'r') &&
	    ( *c++ == 'r') &&
	    ( *c++ == 't') &&
            ( *c++ == 'l')    ) {

	    if (rtlmessages > 0){  /* do display the first one */
		 excess = 1;  
            }
	    rtlmessages++;
        }
    }
    
    return excess;
}

#endif
/******************************************************************************/

ssize_t
host_write(fileHandle_t *fh, control_msg_handle *mh, int *hostErrno) 
{
#undef CMD
#define CMD     ( cmd->info.writeCmd )

size_t nbytes; 
BIGGEST_OFF startPos;
UINT32 fileStatusFlags; 
int nid, pid;
ssize_t numWritten;
hostCmd_t *cmd;

    nid = SRVR_HANDLE_NID(*mh);
    pid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    nbytes = CMD.nbytes;
    fileStatusFlags = CMD.fileStatusFlags;
    startPos = CMD.curPos;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "host_write(");
	fprintf(stderr, " fh %p,", fh);
	fprintf(stderr, " nbytes %li,", nbytes);
#if defined (__i386__)
	    fprintf(stderr, " startPos %lli,", startPos);
#else
	    fprintf(stderr, " startPos %li,", startPos);
#endif __i386__
	fprintf(stderr, " fileStatusFlags 0x%x,", fileStatusFlags);
	fprintf(stderr, " nid %i,", nid);
	fprintf(stderr, " pid %i,", pid);
    }

    /* if the node that is currently writing opened this file
		in append mode move to the end of the file */
    if ((fileStatusFlags & O_APPEND) == O_APPEND) {
#if defined (__i386__) & !defined (LINUX_PORTALS)
	    fh->curPos = _eseek(fh->fd, (off64_t)0, SEEK_END);
#else
	    fh->curPos = lseek(fh->fd, 0L, SEEK_END);
#endif
    } else {
	fh->curPos = positionFilePtr(fh, startPos, nid, pid);
    }

    if ((INT32) fileStatusFlags != fh->otherFlags) {
	fh->otherFlags = fileStatusFlags;
	fcntl(fh->fd, F_SETFL, fh->otherFlags);
    }

    numWritten = host_write2(fh->fd, nbytes, mh, hostErrno);

    if (numWritten > 0){
        fh->curPos += numWritten;
    }

    if ((numWritten == -1) && DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "host_write(): host_write2 failed\n");
    }

    return(numWritten);

}  /* end of host_write() */

/******************************************************************************/

static ssize_t
host_write2(INT32 fd, size_t nbytes, control_msg_handle *mh, int *hostErrno)
{

size_t	transferSize;
ssize_t	numWritten, remaining;
INT32	offset = 0;
INT32	bufnum;
int     nblocks, rc;

    *hostErrno = 0;

    bufnum = get_work_buf();

    if (bufnum < 0){
        numWritten = -1;
	ioErrno = ERESOURCE;
	goto done;
    }

    nblocks = NIOBLOCKS(nbytes);

    if (nblocks == 1){

        rc = srvr_comm_put_reply(mh, workBufData(bufnum), nbytes);

	if (rc){
            ioErrno = CPerrno;
	    numWritten = -1;
	    goto done;
	}

#ifdef SUPPRESS_DEC_FORTRAN_RTL

        if ((fd==6) && is_forrtl_excess(nbytes, bufnum)){
	    numWritten = nbytes;   /* just skip it */
	}
	else{
            numWritten = write(fd, workBufData(bufnum), nbytes);
	}
#else
        numWritten = write(fd, workBufData(bufnum), nbytes);
#endif

        if (numWritten < 0){
	    *hostErrno = errno;
	}

	goto done;
    }

    offset = 0;
    remaining = nbytes;
    numWritten = 0;

    while (nblocks){

        transferSize = ((remaining > IOBUFSIZE) ? IOBUFSIZE : remaining);

	rc = srvr_comm_put_reply_partial(mh,
	              workBufData(bufnum), transferSize, offset);

        if (rc < 0){
	    ioErrno = CPerrno;
	    numWritten = -1;
	    goto done;
	}

        rc = write(fd, workBufData(bufnum), transferSize);

	if (rc == transferSize){
	    numWritten += rc;
	}
	else{
	    numWritten = -1;
	    *hostErrno = errno;
	    break;
	}

        offset += transferSize;
	remaining -= transferSize;
	nblocks--;
    }

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    if ((numWritten < 0) && (*hostErrno == 0)){
        *hostErrno = EIO;
    }

    return numWritten;

}  /* end of host_write2() */

