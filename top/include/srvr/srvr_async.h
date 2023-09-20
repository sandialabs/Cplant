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
/* $Id: srvr_async.h,v 1.1 2002/03/06 21:35:11 ktpedre Exp $ */

void init_pending_buf_list();
void add_buf(char *buf, ptl_handle_md_t md, ptl_handle_eq_t eventq, 
             int num_targets, int *target_nids, int *wait_array);
void prune_bufs();
int  check_nid(int nid);

/* debugging functions */
void print_bufs();
void print_target_nids();
