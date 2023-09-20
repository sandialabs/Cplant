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
#ifndef _ZTYPES_H_
#define _ZTYPES_H_
/*
 * Basic system types.
 *
 * Basically, some attempt is made to ease portability by abstracting critical
 * types.
 *
 * $Id: ztypes.h,v 1.1.4.1 2002/03/13 22:13:18 rklundt Exp $
 */
#include <sys/types.h>

typedef u_int64_t z_off_t;
#define FMT_Z_OFF_T	"%Lu"

typedef u_int32_t z_uid_t;
#define FMT_Z_UID_T	"%lu"
typedef u_int32_t z_gid_t;
#define FMT_Z_GID_T	"%lu"

/*
 * When we quit messing around with these, the conditional compilation should
 * be removed. To be pedantic, we should chenge the RPC version numbers in our
 * server side NFS implementation every time we change either of these.
 *
 * In other words, let's settle on what this is really going to be
 * very quickly.
 */
#undef Z_DEV_T_IS_64
#define Z_INO_T_IS_64	1

#if defined(Z_DEV_T_IS_64)
typedef u_int64_t z_dev_t;
#define FMT_Z_DEV_T	"0x%llx"
#else /* !defined(Z_DEV_T_IS_64) */
typedef u_int32_t z_dev_t;
#define FMT_Z_DEV_T	"0x%lx"
#endif /* defined(Z_DEV_T_IS_64) */
#if defined(Z_INO_T_IS_64)
typedef u_int64_t z_ino_t;
#define FMT_Z_INO_T	"0x%llx"
#else /* !defined(Z_INO_T_IS_64) */
typedef u_int32_t z_ino_t;
#define FMT_Z_INO_T	"0x%lx"
#endif /* defined(Z_INO_T_IS_64) */

typedef u_int16_t z_mode_t;
#define FMT_Z_MODE_T	"0%ho"
typedef u_int16_t z_nlink_t;
#define FMT_Z_NLINK_T	"%hu"
#endif /* defined(_ZTYPES_H_) */
