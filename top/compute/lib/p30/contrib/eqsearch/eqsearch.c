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
** $Id: eqsearch.c,v 1.2 2001/02/12 22:04:24 rbbrigh Exp $
**
** Portals 3.0 utility functions for searching an event queue
**
**
*/

#include"eqsearch.h"

static int chunk_size = PTL_EQ_SEARCH_CHUNK_SIZE;

/*
** PtlEQSearchChunkSize - set the chunk size to something greater than the default
**  new_size - number of new event nodes to allocate when we run out
*/
void PtlEQSearchChunkSize( int new_size )
{
    chunk_size = ( (new_size >= 0) && (new_size > chunk_size) ) ? new_size : chunk_size;
}

/*
** PtlEQSearchAllocFreeNodes - allocates more free nodes for holding old events
**   eq_handle        - Portals 3.0 event queue handle of the event queue being searched
**
**   Returns
**   0                - on success
**   -1               - otherwise
*/
static int PtlEQSearchAllocFreeNodes( ptl_eq_search_t *eq_search )
{
    ptl_event_node_t *new_node;
    ptl_event_node_t *last_node;
    int               i;
    
    for ( i=0; i<chunk_size; i++ ) {
	/* allocate a new node */
	if ( (new_node = (ptl_event_node_t *)malloc( sizeof(ptl_event_node_t) )) == NULL ) {
	    return -1;
	}
	/* add it to the free list */
	if ( eq_search->free_list == NULL ) {
	    eq_search->free_list = new_node;
	    new_node->prev       = NULL;
	} else {
	    last_node->next = new_node;
	    new_node->prev  = last_node;
	}
	new_node->next = NULL;
	last_node      = new_node;
    }

    return 0;
}


/*
** PtlEQSearchInit - initialization function
**   eq_handle        - Portals 3.0 event queue handle of the event queue being searched
**   eq_search_handle - returned handle for searching an event queue
**
**   Returns
**   0                - on success
**   -1               - otherwise
*/
int PtlEQSearchInit( ptl_handle_eq_t eq_handle, ptl_handle_eq_search_t *eq_search_handle )
{
    ptl_eq_search_t  *eq_search;
    
    if ( eq_search_handle == NULL ) {
	return -1;
    }

    /* get a new search descriptor */
    if ( (eq_search = (ptl_eq_search_t *)malloc( sizeof(ptl_eq_search_t) ) ) == NULL ) {
	return -1;
    }

    eq_search->eq_handle = eq_handle;

    if ( PtlEQSearchAllocFreeNodes( eq_search ) ) {
	free( eq_search );
	return -1;
    }

    /* initialize the search list */
    eq_search->search_list_head = NULL;
    eq_search->search_list_tail = NULL;

    /* return a handle to the search descriptor */
    *eq_search_handle = eq_search;

    return 0;
}


/*
** PtlEQSearchFinit - cleanup function
**   eq_search_handle - previously allocated handle for searching an event queue
**
**   Returns
**   0                - on success
**   -1               - otherwise
*/
int PtlEQSearchFini( ptl_handle_eq_search_t *eq_search_handle )
{
    ptl_eq_search_t  *eq_search = *eq_search_handle;
    ptl_event_node_t *node,*next;
    int               i;

    /* free all of the nodes on the free list and search list */
    node = eq_search->free_list;
    for ( i=0; i<2 ; i++ ) {
	while ( node ) {
	    next = node->next;
	    free( node );
	    node = next;
	}
	node = eq_search->search_list_head;
    }

    /* free the search handle */
    free( eq_search );

    *eq_search_handle = NULL;

    return 0;
}


/*
** PtlEQSearchCompare - checks to see if an event matches the matching criteria
** 
** Internal function called by g_slist_find_custom()
**
** Returns
** 0 - on a match
** 1 - otherwise
*/
static int PtlEQSearchCompare( ptl_event_t *event, ptl_eq_search_criteria *criteria )
{

    /* check for a matching process id */
    if ( (criteria->id.addr_kind == PTL_ADDR_NID) || (criteria->id.addr_kind == PTL_ADDR_BOTH) ) {

	if ( criteria->id.nid != PTL_ID_ANY ) {
	    if ( criteria->id.nid != event->initiator.nid ) {
		return 1;
	    }
	}

	if ( criteria->id.pid != PTL_ID_ANY ) {
	    if ( criteria->id.pid != event->initiator.pid ) {
		return 1;
	    }
	}
    } else {

	if ( criteria->id.gid != PTL_ID_ANY ) {
	    if ( criteria->id.gid != event->initiator.gid ) {
		return 1;
	    }
	}

	if ( criteria->id.rid != PTL_ID_ANY ) {
	    if ( criteria->id.rid != event->initiator.rid ) {
		return 1;
	    }
	}
    }

    /* check portal index */
    if ( criteria->portal != PTL_INDEX_ANY ) {
	if ( criteria->portal != event->portal ) {
	    return -1;
	}
    }

    /* check for matching type of event */
    if ( criteria->type != PTL_EVENT_ANY ) {
	if ( criteria->type != event->type ) {
	    return 1;
	}
    }

    /* check matching bits */
    if ( (criteria->match_bits & ~criteria->ignore_bits) !=
	 (event->match_bits & ~criteria->ignore_bits) ) {
	return 1;
    }

    /* passed everything, so this matches */
    return 0;

}


/*
** PtlEQSearch - find a matching event in an event queue
**   eq_search_handle - previously allocated handle for searching an event queue
**   criteria         - event matching criteria
**   blocking         - whether or not to block waiting for an event
**   event            - matching event if found
**
**   Returns
**   0                - on a successful find
**   1                - on an successful find
**   -1               - otherwise
*/
int PtlEQSearch( ptl_handle_eq_search_t eq_search_handle, ptl_eq_search_criteria *criteria,
		 int blocking, ptl_event_t *event )
{
    ptl_eq_search_t  *eq_search = eq_search_handle;
    ptl_event_node_t *event_node;
    int              done = 0;
    int              rc;

    /* arg checking */
    if ( (criteria == NULL) || (event == NULL) ) {
	return -1;
    }

    /* look for matching event on the search list */
    event_node = eq_search->search_list_head;
    while ( event_node ) {

	if ( PtlEQSearchCompare( &event_node->event, criteria ) == 0 ) {

	    /* copy the event values */
	    *event                 = event_node->event;

	    /* remove this event from the search list */
	    if ( event_node->prev ) {
		event_node->prev->next = event_node->next;
	    }
	    if ( event_node->next ) {
		event_node->next->prev = event_node->prev;
	    }
	    if ( eq_search->search_list_head == event_node ) {
		eq_search->search_list_head = event_node->next;
	    }

	    /* add it to the free list */
	    event_node->next = eq_search->free_list;
	    event_node->prev = NULL;
	    eq_search->free_list = event_node;

	    return 0;
	}

	event_node = event_node->next;

    }

    /* didn't find it, so look for it */
    while ( ! done ) {

	if ( blocking ) {

	    /* wait for an event */
	    if ( PtlEQWait( eq_search->eq_handle, event ) != PTL_OK ) {
		return -1;
	    }

	} else {
	    /* check for an event */
	    rc = PtlEQGet( eq_search->eq_handle, event );

	    if ( rc == PTL_EQ_EMPTY ) {
		return 1;
	    }
	    if ( rc != PTL_OK ) {
		return -1;
	    }
	}

	/* see if it's the one we want */
	if ( PtlEQSearchCompare( event, criteria ) == 0 ) {

	    /* found it */
	    done = 1;
	} else {

	    /* not the one we want, so put it on the tail of the search list */
	    /* get a free event (remove the head element) */
	    if ( (event_node = eq_search->free_list) == NULL ) {
		/* allocate some more nodes */
		if ( PtlEQSearchAllocFreeNodes( eq_search ) ) {
		    return -1;
		}
		event_node = eq_search->free_list;
	    }

	    eq_search->free_list = event_node->next;

	    event_node->event = *event;

	    /* add the event to the past event list */
	    if ( eq_search->search_list_head == NULL ) {
		eq_search->search_list_head = event_node;
		eq_search->search_list_tail = event_node;
		event_node->prev = NULL;
	    } else {
		eq_search->search_list_tail->next = event_node;
		event_node->prev = eq_search->search_list_tail;
		eq_search->search_list_tail = event_node;
	    }
	    event_node->next = NULL;
	}
    }
    
    return 0;
}
