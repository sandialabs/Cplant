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
$Id: fdTable.c,v 1.8 2001/02/16 05:39:25 lafisk Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "fdTable.h"

TITLE(fdTable_c, "@(#) $Id: fdTable.c,v 1.8 2001/02/16 05:39:25 lafisk Exp $");

/* table to hold process file descriptors */
fdTable_t   _fdTable[_NFILE];

fdTableEntry_t * createFdTableEntry( void )
{
    fdTableEntry_t *entry = (fdTableEntry_t *) malloc(sizeof(fdTableEntry_t));
    return( entry ); 
}

VOID destroyFdTableEntry( fdTableEntry_t *entry  )
{
    free( entry );
}
/*
** must return lowest numbered free file descriptor due
** to some libc function semantics.
*/
int availFd(void)
{
int fd, i ;

    fd = -1;

    for (i = 0; i < _NFILE ; i++){

        if (FD_ENTRY(i) == NULL){
	    fd = i;
	    break;
	}
    }

    return fd;
}
int validFd( int fd )
{
    if ( ( ( fd < 0 ) || ( fd >= _NFILE ) ) || ( FD_ENTRY( fd ) == NULL ) )
    {
        return( 0 );
    }

    return( 1 );
}
int
fcntlDup( int fd, int new_fd )
{
    if ( (new_fd < 0) || (new_fd >= _NFILE)){
       errno = EBADF;
       return -1;
    }
    if (!validFd(fd)){
       errno = EBADF;
       return -1;
    }
    /*
    ** if the new fd is in use, close it
    */
    if ( FD_ENTRY( new_fd ) ) {
        close(new_fd);
    }
    FD_CLOSE_ON_EXEC_FLAG( new_fd ) = FD_CLOSE_ON_EXEC_FLAG( fd );
    FD_FILE_STATUS_FLAGS( new_fd ) = FD_FILE_STATUS_FLAGS( fd );
    FD_ENTRY( new_fd ) = FD_ENTRY( fd );
    FD_ENTRY_REFCOUNT_INC(new_fd);  /* either fd will do; added
                                   *to fix closing with an existing dup */
 
    return( new_fd );
}

