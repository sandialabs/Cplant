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
/* $Id: positionFilePtr.c,v 1.4 2001/02/16 05:44:26 lafisk Exp $ */


#include <sys/types.h>
#include <unistd.h>

#ifdef LINUX_PORTALS
#include <puma_io.h>
#else
#include <nx.h>
#endif

#include "util.h"
#include "fileHandle.h"
#include "positionFilePtr.h"

/* we are passing in the nid and pid incase some of the unimplemented
	modes are implemented, they need to be able to sync */ 

BIGGEST_OFF 
positionFilePtr(fileHandle_t *fh, BIGGEST_OFF nodeCurPos, UINT16 nid,
	UINT16 pid)
{
    BIGGEST_OFF retval = nodeCurPos;

    if (DBG_FLAGS(DBG_IO_1)) {
#if defined (__i386__)
	    fprintf(stderr, "positionFilePtr(%p, %lli, %i, %i): ", fh,
		nodeCurPos, nid, pid);
	    fprintf(stderr, "iomode %i, ", fh->iomode);
	    fprintf(stderr, "fh->curPos %lli\n", fh->curPos);
#else
	    fprintf(stderr, "positionFilePtr(%p, %li, %i, %i): ", fh,
		nodeCurPos, nid, pid);
	    fprintf(stderr, "iomode %i, ", fh->iomode);
	    fprintf(stderr, "fh->curPos %li\n", fh->curPos);
#endif __i386__
    }


    switch (fh->iomode) {

	case M_UNIX:
	    if (fh->curPos != nodeCurPos) {
#if defined (__i386__) & !defined (LINUX_PORTALS)
		    retval = _eseek(fh->fd, nodeCurPos, SEEK_SET);
#else
		    retval = lseek(fh->fd, nodeCurPos, SEEK_SET);
#endif __i386__
	    }
	    break;

	case M_LOG:
	    /* all process share the same file pointer, so
				there is no need to move */
	    retval = fh->curPos;
	    break;

#ifndef LINUX_PORTALS

	case M_SYNC:
	    fprintf(stderr, "host_write(): iomode M_SYNC not implemented\n");
	    break;

	case M_RECORD:
	    fprintf(stderr, "host_write(): iomode M_RECORD not implemented\n");
	    break;

	case M_GLOBAL:
            fprintf(stderr, "host_write(): iomode M_GLOBAL not implemented\n");
	    break;

	case M_ASYNC:
	    fprintf(stderr, "host_write(): iomode M_ASYNC not implemented\n");
	    break;

#endif LINUX_PORTALS

        default:
	    fprintf(stderr, "host_write(): invalid iomode %i\n", fh->iomode);
    }

    return(retval);

}  /* end of positionFilePtr() */
