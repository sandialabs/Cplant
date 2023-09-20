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
/* $Id: fileHandle.h,v 1.5 2001/02/16 05:44:34 lafisk Exp $ */


#ifndef FILEHANDLE_H
#define FILEHANDLE_H


#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include "rpc_msgs.h"
#include "defines.h"


typedef struct _fileHandle {
    struct 	_fileHandle *fh_next;
    struct 	_fileHandle *fh_prev;
    CHAR	iomode;	
    INT32   	fd;
    CHAR	fname[MAXPATHLEN+1];
    INT32	accessFlags;
    INT32	otherFlags;	
    INT32  	refCount;
    BIGGEST_OFF	curPos;
} fileHandle_t;

extern INT32 fileHandleCount;
 
fileHandle_t *   allocFileHandle(VOID);
VOID             destroyFileHandle( fileHandle_t *fh );

fileHandle_t *   findClosedFileHandle(VOID);
fileHandle_t *   findOpenFileHandle(VOID);

fileHandle_t 	*checkFileHandleList( const CHAR *fname );

fileHandle_t	*cacheFileHandle( fileHandle_t *fh );
fileHandle_t 	*cacheOutAnyFileHandle( void );
int             cacheOutFileHandle( fileHandle_t *fh );
int             cacheInFileHandle( fileHandle_t *fh );


VOID			initStdioFileHandle( FILE *fp );
VOID			initStdioFileHandles(VOID);
VOID 			printFileHandle( fileHandle_t *fh );
VOID 			printFileHandleList(VOID);

#define STDIN_FILE_NAME		"stdin"
#define STDOUT_FILE_NAME	"stdout"
#define STDERR_FILE_NAME	"stderr"

/* subtract 2 off of the total so yod can have a couple of
	fd's, for its  work, and subtract 3 off because we 
	duped stdin, stdout, and stderr   also subtract 2 more as it
        appears that gdb when being used to debug yod also used a couple
        of FD. */

#ifdef LINUX_PORTALS

#define MAX_OPEN_FILES ( FOPEN_MAX - 9 )

#else /* LINUX_PORTALS */

#define MAX_OPEN_FILES ( _NFILE - 9 )

#endif /* LINUX_PORTALS */

#endif /* FILEHANDLE_H */
