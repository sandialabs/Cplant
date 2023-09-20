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
/* $Id: mmap.c,v 1.10 2001/04/12 09:13:24 lafisk Exp $ */

#include <sys/types.h>
#include "protocol_switch.h"
#include "fdTable.h"
#include "errno.h"

/*
** $Id: mmap.c,v 1.10 2001/04/12 09:13:24 lafisk Exp $
*/

/******************************************************************************/

void *
mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
int protocol;

    protocol=fd2io_proto(fd);

    FD_MMAP_BUFFER(fd) = 
        io_ops[protocol]->mmap(start, length, prot, flags, fd, offset);

    return FD_MMAP_BUFFER(fd);
}
int
munmap(void *start, size_t length)
{
int protocol, i, fd;

    fd = -1;

    for (i = 0; i < _NFILE ; i++){

        if (FD_ENTRY(i) != NULL){
             if (FD_MMAP_BUFFER(i) == start){
                 fd = i;
                 break;
             }
        }
    }

    if (fd == -1){
	errno = EINVAL;
        return -1;
    }

    FD_MMAP_BUFFER(fd) = NULL;

    protocol=fd2io_proto(fd);

    return io_ops[protocol]->munmap(start, length);
}
#ifdef __osf__
void *
__mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    return mmap(start, length, prot, flags, fd, offset);
}
int
__munmap(void *start, size_t length)
{
    return munmap(start, length);
}
#endif
