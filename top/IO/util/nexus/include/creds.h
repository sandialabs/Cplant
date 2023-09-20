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
#ifndef _CREDS_H_
#define _CREDS_H_
/*
 * Credentials.
 *
 * $Id: creds.h,v 1.0 1999/11/17 23:51:37 lee Exp $
 */
#include "ztypes.h"

#ifndef XNFS_NGROUPS
#define XNFS_NGROUPS	8
#endif

/*
 * User ID of super-user.
 */
#define SUSER_UID	0

/*
 * User credentials.
 */
struct creds {
	z_uid_t	cr_uid;				/* user ID */
	size_t	cr_ngroups;			/* number of groups */
	z_gid_t cr_groups[XNFS_NGROUPS];	/* groups */
};

#define cr_gid		cr_groups[0]

/*
 * Are credentials those of a super-user?
 */
#define is_suser(crp) \
	((crp)->cr_uid == SUSER_UID)

extern struct creds suser_creds;
#endif /* defined(_CREDS_H_) */
