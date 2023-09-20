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
** $Id: sys_limits.h,v 1.9 2000/10/26 12:50:27 dwdoerf Exp $
*/

#ifndef SYS_LIMITSH
#define SYS_LIMITSH

#define MAX_PROC_PER_GROUP           2048	/* max processes in app group */
#define MAX_NODES                    2048	/* max nodes in machine */
#define MAX_PROC_PER_GROUP_PER_NODE  1		/* max launched on one node */
#define MAX_GROUPS_PER_NODE          1	/* max groups launched on one node */

/*
 * The two values below are arbitrary.  They are used to help the pct determine
 * if the data in the load_data_buffer structure is sane.
 */
#define MAX_ARG_LENGTH     256              
#define MAX_ENV_LENGTH   20480

#define MAX_PCTS_PER_NODE      1

#define MAX_ARGV            128
#define MAX_ENVP            256

#endif /* SYS_LIMITSH */
