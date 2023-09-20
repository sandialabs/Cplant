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
** $Id: myrnal.h,v 1.5 2001/03/31 00:36:36 pumatst Exp $
*/

#ifndef MYRNAL_H
#define MYRNAL_H

#define MAX_ARGS_LEN		(256)
#define MAX_RET_LEN		(128)
#define MYRNAL_MAX_ACL_SIZE	(64)
#define MYRNAL_MAX_PTL_SIZE	(64)

#define P3CMD			(100)
#define P3SYSCALL		(200)

enum {PTL_MLOCKALL};

typedef struct   {
    void *args;
    size_t args_len;
    void *ret;
    size_t ret_len;
    int p3cmd;
} myrnal_forward_t;

#endif /* MYRNAL_H */
