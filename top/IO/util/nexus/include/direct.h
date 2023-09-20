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
 * File system independent directory support.
 *
 * $Id: direct.h,v 1.1 2000/02/16 01:11:11 smorgan Exp $
 */

#ifndef _DIRECT_H
#define _DIRECT_H

/*
 * Format of a file system independent directory entry.
 */
struct directory_entry {
	z_ino_t	de_fileid;				/* file ident */
	z_off_t	de_off;					/* entry offset */
	unsigned short de_namlen;			/* entry name length */
	char	de_name[1];				/* entry name */
};

/*
 * Calculate directory entry length, given length of the name string.
 */
#define directory_record_length(namlen) \
	(rndup(sizeof(struct directory_entry) + (namlen), sizeof(int)))
#endif
