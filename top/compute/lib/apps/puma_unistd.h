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
/* $Id: puma_unistd.h,v 1.4 2001/02/16 05:29:51 lafisk Exp $ */

#ifndef PUMA_UNISTD_H
#define PUMA_UNISTD_H

#include "unistd.h"
#include "defines.h"

INT32 _service_open(const CHAR *fname, INT32 flags, INT32 mode,
    UINT16 nid, UINT16 pid);
INT32 _service_close(INT32 fd);
ssize_t _service_read(int fd, CHAR *buff, size_t nbytes);
ssize_t _service_write(int fd, CHAR *buff, size_t nbytes);

#endif /* PUMA_UNISTD_H */
