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
** $Id: fdTable.h,v 1.11 2001/02/16 05:36:16 lafisk Exp $
*/

#ifndef FDTABLE
#define FDTABLE
#include "puma.h"
#include "rpc_msgs.h"

#include <stdio.h>

#ifndef __osf__
#define _NFILE    FOPEN_MAX
#endif

TITLE(fdTable_h, "@(#) $Id: fdTable.h,v 1.11 2001/02/16 05:36:16 lafisk Exp $");

typedef struct
{
    INT32        _where;       /* where to send/recv the requests  */
                               /* This will be obsolete with the new */
                               /* host protocol. Use _srvr_nid, */
                               /* _srvr_pid, and _srvr_ptl instead */
    off64_t         _hostFileIndex;    /* the srvr's file descriptor */
    int             _srvr_nid;
    int             _srvr_pid;
    int             _srvr_ptl;
    uchar        _is_tty;
    uchar           _refCount; /* how many fd's reference the entry */
    BIGGEST_OFF    _curPos;    /* what is our current position in */ 
                               /* the file */
    int            _protocol;
} fdTableEntry_t; 


typedef struct
{
    INT32           _closeOnExecFlag;
    INT32           _fileStatusFlags;
    void            *_mmap_buf;
    fdTableEntry_t  *_entry;
} fdTable_t;



#define FD_CLOSE_ON_EXEC_FLAG( fd )            _fdTable[fd]._closeOnExecFlag
#define FD_FILE_STATUS_FLAGS( fd )            _fdTable[fd]._fileStatusFlags
#define FD_MMAP_BUFFER( fd )                  _fdTable[fd]._mmap_buf
#define FD_ENTRY( fd )                        _fdTable[fd]._entry

#define FD_ENTRY_PROTOCOL( fd )             _fdTable[fd]._entry->_protocol
#define FD_ENTRY_SRVR_NID( fd )             _fdTable[fd]._entry->_srvr_nid
#define FD_ENTRY_SRVR_PID( fd )             _fdTable[fd]._entry->_srvr_pid
#define FD_ENTRY_SRVR_PTL( fd )             _fdTable[fd]._entry->_srvr_ptl 
#define FD_ENTRY_HOST_ID( fd )                 _fdTable[fd]._entry->_where 
#define FD_ENTRY_HOST_FILE_INDEX( fd )         _fdTable[fd]._entry->_hostFileIndex 
#define FD_ENTRY_CURPOS( fd )                 _fdTable[fd]._entry->_curPos
#define FD_ENTRY_IS_TTY( fd )                 _fdTable[fd]._entry->_is_tty
#define FD_ENTRY_REFCOUNT( fd )             _fdTable[fd]._entry->_refCount
#define FD_ENTRY_REFCOUNT_INC( fd )         ++_fdTable[fd]._entry->_refCount
#define FD_ENTRY_REFCOUNT_DEC( fd )         --_fdTable[fd]._entry->_refCount

extern fdTable_t _fdTable[];

fdTableEntry_t * 
createFdTableEntry( void ); 

VOID 
destroyFdTableEntry( fdTableEntry_t *entry  );

int validFd( int fd );
int mmappedFd( void *buf);
int availFd(void);
int fcntlDup( int fd, int new_fd );

enum{ FYOD_IO_PROTO, YOD_IO_PROTO, ENFS_IO_PROTO, DUMMY_IO_PROTO,
      DUMMY1_IO_PROTO, DUMMY2_IO_PROTO, LAST_IO_PROTO };

#endif
