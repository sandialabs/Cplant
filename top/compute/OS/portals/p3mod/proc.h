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
** $Id: proc.h,v 1.5 2001/08/22 16:51:31 pumatst Exp $
** Portals 3.0 module file that generates the files in /proc/cplant
*/

#ifndef P3_PROC_H
#define P3_PROC_H

#include <asm/uaccess.h>

int p3_read_debug_proc_indirect(   char *buf, char **start, off_t off, int len, int unused);
int p3_read_dev_proc_indirect(     char *buf, char **start, off_t off, int len, int unused);
int p3_read_nal_proc_indirect(     char *buf, char **start, off_t off, int len, int unused);
int p3_read_versions_proc_indirect(char *buf, char **start, off_t off, int len, int unused);

int p3_read_debug_proc(   char *buf, char **start, off_t off, int count, int *eof, void *data);
int p3_read_dev_proc(     char *buf, char **start, off_t off, int count, int *eof, void *data);
int p3_read_nal_proc(     char *buf, char **start, off_t off, int count, int *eof, void *data);
int p3_read_versions_proc(char *buf, char **start, off_t off, int count, int *eof, void *data);

#endif /* P3_PROC_H */
