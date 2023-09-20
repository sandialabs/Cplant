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
** $Id: eqsearch.h,v 1.2 2001/02/12 22:04:24 rbbrigh Exp $
**
** Portals 3.0 utility functions for searching an event queue
**
*/

#ifndef _EQSEARCH_H
#define _EQSEARCH_H

#include<stdio.h>
#include<stdlib.h>

#include"p30.h"

#define PTL_EQ_SEARCH_CHUNK_SIZE 64

#define PTL_EVENT_ANY (-1)
#define PTL_INDEX_ANY (-1)

typedef struct event_node {
    struct event_node *next;
    struct event_node *prev;
    ptl_event_t        event;
} ptl_event_node_t;

typedef struct {
    ptl_handle_eq_t   eq_handle;
    ptl_event_node_t *event_nodes;
    ptl_event_node_t *search_list_head;
    ptl_event_node_t *search_list_tail;
    ptl_event_node_t *free_list;
} ptl_eq_search_t;

typedef struct {
    ptl_process_id_t id;
    ptl_pt_index_t   portal;
    ptl_event_kind_t type;
    ptl_match_bits_t match_bits;
    ptl_match_bits_t ignore_bits;
} ptl_eq_search_criteria;

typedef ptl_eq_search_t *ptl_handle_eq_search_t;

/* external prototypes */
void PtlEQSearchChunkSize( int );
int  PtlEQSearchInit( ptl_handle_eq_t, ptl_handle_eq_search_t * );
int  PtlEQSearchFini( ptl_handle_eq_search_t * );
int  PtlEQSearch( ptl_handle_eq_search_t, ptl_eq_search_criteria *, int, ptl_event_t * );

#endif

