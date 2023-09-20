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
** $Id: eqsearchtest.c,v 1.1 2001/02/12 22:05:52 rbbrigh Exp $
**
** Test program for testing the EQ search functions
**
*/

#include<stdio.h>
#include"eqsearch.h"

#define P30_CALL( func, name )				\
{							\
    int rc;						\
							\
    if ( (rc = func) != PTL_OK ) {			\
	fprintf(stderr,"%d: (%s:%d) %s failed : %s\n",	\
		rank, __FILE__, __LINE__, #name,	\
		ptl_err_str[rc]);			\
	return -1;					\
    }							\
}

#define PORTAL      14
#define NONBLOCKING  0
#define BLOCKING     1

int                    rank = -1;

void PtlEQDumpEvent( ptl_event_t *event )
{
    fprintf(stderr,"  event =>\n");
    fprintf(stderr,"    type = ");
    switch ( event->type ) {
    case PTL_EVENT_GET   : fprintf(stderr,"PTL_EVENT_GET\n"); break;
    case PTL_EVENT_PUT   : fprintf(stderr,"PTL_EVENT_PUT\n"); break;
    case PTL_EVENT_REPLY : fprintf(stderr,"PTL_EVENT_REPLY\n"); break;
    case PTL_EVENT_ACK   : fprintf(stderr,"PTL_EVENT_ACK\n"); break;
    case PTL_EVENT_SENT  : fprintf(stderr,"PTL_EVENT_SENT\n"); break;
    default              : fprintf(stderr,"UNKNOWN EVENT!\n"); break;
    }
    fprintf(stderr,"  initiator =>\n");
    fprintf(stderr,"     gid    = %d\n", event->initiator.gid);
    fprintf(stderr,"     rid    = %d\n", event->initiator.rid);
    fprintf(stderr,"  portal    = %d\n", event->portal);
    fprintf(stderr,"  match_bits = 0x%lx\n",event->match_bits);
    fprintf(stderr,"  rlength    = %d\n", event->rlength);
    fprintf(stderr,"  mlength    = %d\n", event->mlength);
    fprintf(stderr,"  offset     = %d\n", event->offset);
    fprintf(stderr,"  mem_desc =>\n");
    fprintf(stderr,"    start  = %p\n", event->mem_desc.start);
    fprintf(stderr,"    length = %d\n", event->mem_desc.length);
    fprintf(stderr,"    threshold = %d\n",event->mem_desc.threshold);
    fprintf(stderr,"    options   = %d\n",event->mem_desc.options);
    fprintf(stderr,"    user_ptr  = %p\n",event->mem_desc.user_ptr);
    fprintf(stderr,"    eventq    = %d\n",event->mem_desc.eventq);
    fprintf(stderr,"  hdr_data    = %ld\n",event->hdr_data);
}

int main( int ac, char **av )
{

    ptl_process_id_t       my_id;
    ptl_handle_ni_t        ni_handle;
    ptl_handle_me_t        me_handle;
    ptl_handle_md_t        md_handle;
    ptl_handle_eq_t        eq_handle;
    ptl_handle_eq_search_t eq_search_handle;
    ptl_md_t               md;
    ptl_eq_search_criteria criteria;
    ptl_event_t            event;
    ptl_match_bits_t       ignore_bits = 0xffffffff;
    int                    size;
    int                    rc;
    int                    i;

    P30_CALL( PtlInit(), PtlInit );

    P30_CALL( PtlNIInit( PTL_IFACE_DEFAULT, 64, 64, &ni_handle ), PtlNIInit );

#if defined(DEBUG)
    P30_CALL( PtlNIDebug( ni_handle, 1 ), PtlNIDebug );
#endif

    P30_CALL( PtlGetId( ni_handle, &my_id, &size ), PtlGetId );

    rank = my_id.rid;

    P30_CALL( PtlMEAttach( ni_handle,
			   PORTAL,
			   my_id,
			   0,
			   ignore_bits,
			   PTL_RETAIN,
			   &me_handle ), PtlMEAttach );

    P30_CALL( PtlEQAlloc( ni_handle, 64, &eq_handle ), PtlEQAlloc );

    if ( PtlEQSearchInit( eq_handle, &eq_search_handle ) ) {
	fprintf(stderr,"PtlEQSearchInit() failed\n");
	return -1;
    }

    md.start     = NULL;
    md.length    = 0;
    md.threshold = PTL_MD_THRESH_INF;
    md.options   = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
    md.user_ptr  = NULL;
    md.eventq    = eq_handle;

    P30_CALL( PtlMDAttach( me_handle,
			   md,
			   PTL_RETAIN,
			   &md_handle ), PtlMDAttach );

    for ( i=0; i<16; i++ ) {

	P30_CALL( PtlPut( md_handle,
			  PTL_NOACK_REQ,
			  my_id,
			  PORTAL,
			  0,
			  (ptl_match_bits_t)i,
			  0,
			  0 ), PtlPut );

    }

    criteria.id          = my_id;
    criteria.portal      = PORTAL;
    criteria.type        = PTL_EVENT_PUT;
    criteria.match_bits  = 0;
    criteria.ignore_bits = 0;

    /* get all of the put events */
    for ( i=15; i>=0; i-- ) {

	criteria.match_bits = (ptl_match_bits_t)i;
	
	if ( PtlEQSearch( eq_search_handle, &criteria, BLOCKING, &event ) == 0 ) {
	    printf("Found put # %d\n",i);
	} else {
	    printf("Couldn't find put # %d!!\n",i);
	    exit(1);
	}

    }

    if ( (rc = PtlEQSearch( eq_search_handle, &criteria, NONBLOCKING, &event ) != 1) ) {
	printf("Found another put event!!!\n");
    }

    criteria.type = PTL_EVENT_SENT;

    /* get all of the sent events */
    for ( i=15; i>=0; i-- ) {

	criteria.match_bits = (ptl_match_bits_t)i;

	if ( PtlEQSearch( eq_search_handle, &criteria, BLOCKING, &event ) == 0 ) {
	    printf("Found sent # %d\n",i);
	} else {
	    printf("Couldn't find sent # %d!!\n",i);
	    exit(1);
	}

    }

    criteria.type = PTL_EVENT_ANY;

    if ( (rc = PtlEQSearch( eq_search_handle, &criteria, NONBLOCKING, &event ) != 1) ) {
	printf("Found another event!!!\n");
    }

    (void)PtlEQSearchFini( &eq_search_handle );

    PtlNIFini( ni_handle );

    PtlFini();

    printf("All done\n");

    return 0;

}
