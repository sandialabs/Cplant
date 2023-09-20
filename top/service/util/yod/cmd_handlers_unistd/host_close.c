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
/* $Id: host_close.c,v 1.5 2001/02/16 05:44:25 lafisk Exp $ */

#include <unistd.h> 
#include "fileHandle.h"
#include "util.h"
#include "host_close.h"

extern int open_file_cnt;

/******************************************************************************/

INT32
host_close(fileHandle_t *fh, INT32 uid, INT32 gid)
{

INT32  rc = 0;


    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "host_close(");
	fprintf(stderr, " fh %p,", fh);
	fprintf(stderr, " uid %i,", uid );
	fprintf(stderr, " gid %i)\n", gid);
    }

    if (fh->refCount == 1) {
	/*
	** if the file we are closing is in the cache (i.e. open) 
	** and there are others that are not
	*/
	if (fh->fd >= 0) {
	    rc = close(fh->fd);
	    if (fileHandleCount > MAX_OPEN_FILES) {
		fileHandle_t *fh_in = findClosedFileHandle();

		if (fh_in) {
		    cacheInFileHandle(fh_in);
		} else {
		    fprintf(stderr, "FATAL: host_close(): \n"); 
		    printFileHandleList(); 
	            return -1;	
		}
	    }
	}

	destroyFileHandle(fh);
    } else {
	--fh->refCount;
    }

	
    if (DBG_FLAGS(DBG_IO_1))   {
    /*
	printFileHandleList();
	*/
        printf("host_close returning %p, %d, %d\n",
                               fh, uid, gid);
    }

    return(rc);

}  /* end of host_close() */
