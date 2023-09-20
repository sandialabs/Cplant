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
 * $Id: eq.c,v 1.20 2001/02/05 01:49:37 lafisk Exp $
 *
 * api-p30/eq.c
 *
 * User-level event queue management routines
 *
 * PtlMDUpdate is here so that it can access the per-eventq
 * structures.
 */

#include <p30.h>
#include <p30/internal.h>
#include <p30/nal.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Lots of POSIX dependencies to support PtlEQWait_timeout */
#include <signal.h>
#include <setjmp.h>
#include <time.h>

int ptl_eq_init( void )
{
	/* Nothing to do anymore... */
	return PTL_OK;
}

void ptl_eq_fini( void )
{
	/* Nothing to do anymore... */
}

int ptl_eq_ni_init( nal_t *nal )
{
	int i;

	nal->ni.eq = malloc( sizeof( ptl_eq_t ) * MAX_EQS );
	if( !nal->ni.eq )
		return PTL_NOSPACE;

	for( i=0 ; i<MAX_EQS ; i++ ) {
		nal->ni.eq[i].inuse = 0;
	}

	return PTL_OK;
}

void ptl_eq_ni_fini( nal_t *nal )
{
	free( nal->ni.eq );
}

int PtlEQAlloc(
	ptl_handle_ni_t	interface,
	ptl_size_t	count,
	ptl_handle_eq_t	*handle_out
)
{
	ptl_eq_t		*eq = NULL;
	ptl_event_t		*ev = NULL;
	ptl_handle_eq_t		handle;
	int			current,
				rc,
				i;
	nal_t			*nal;

	nal = ptl_interfaces[ (interface>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;

	ev = malloc( count * sizeof( ptl_event_t ) );
	if( !ev )
		return PTL_NOSPACE;

	for( i=0 ; i<count ; i++ )
		ev[i].sequence = 0;

	rc = nal->validate( nal, ev, count * sizeof( ptl_event_t ) );
	if( rc )
		goto fail;

	rc = PtlEQAlloc_internal( interface,
		count, ev, count * sizeof( ptl_event_t), &handle );
	if( rc )
		goto fail;

	current = handle & 0xFFFF;
	eq = &nal->ni.eq[ current ];

	if( current < 0 || current > MAX_EQS || eq->inuse ) {
		fprintf( stderr,
			"PtlEQAlloc: eq %d is in use or out of range?\n",
			current );
		rc = PTL_NOSPACE;
		goto fail;
	}
		
	eq->inuse	= 1;
	eq->sequence	= 1;
	eq->size	= count;
	eq->base	= ev;

	*handle_out = handle;
	return PTL_OK;


fail:
	free( ev );
	return PTL_SEGV;
}


int PtlEQFree(
	ptl_handle_eq_t		eventq
)
{
	int 			current		= eventq & 0xFFFF;
	ptl_eq_t		*eq;
	int			rc;
	nal_t			*nal;

	nal = ptl_interfaces[ (current>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;

	eq = &nal->ni.eq[ current ];
	if( current < 0 || current > MAX_EQS || !eq->inuse )
		return PTL_INV_EQ;

	rc = PtlEQFree_internal( eventq );

	/*
	 * The library will have already invalidated this region
	 * as a result of the EQFree call.
	 */
	eq->inuse = 0;
	free( eq->base );

	return rc;
}


int PtlEQGet(
	ptl_handle_eq_t		eventq,
	ptl_event_t		*ev
)
{
	int 			current		= eventq & 0xFFFF;
	ptl_eq_t		*eq;
	int			rc, new_index;
	ptl_event_t		*new_event;
	nal_t			*nal;

	nal = ptl_interfaces[ (current>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;

	eq = &nal->ni.eq[ current ];
	if( current < 0 || current > MAX_EQS || !eq->inuse )
		return PTL_INV_EQ;

	new_index = eq->sequence % eq->size;
	new_event = &eq->base[ new_index ];
	if( eq->sequence > new_event->sequence )
		return PTL_EQ_EMPTY;

	*ev = *new_event;
	if( eq->sequence != new_event->sequence )
		rc = PTL_EQ_DROPPED;
	else
		rc = PTL_OK;

	eq->sequence = new_event->sequence+1;
	return rc;
}


int PtlEQWait(
	ptl_handle_eq_t		eventq_in,
	ptl_event_t		*event_out
)
{
	int	rc;
	int	current	= eventq_in & 0xFFFF;
	nal_t	*nal	= ptl_interfaces[ (current>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;


	while( (rc = PtlEQGet( eventq_in, event_out )) == PTL_EQ_EMPTY ) {
		if( nal->yield )
			nal->yield( nal );
	}

	return rc;
}

static jmp_buf eq_jumpbuf;

static void eq_timeout( int signal )
{
	longjmp( eq_jumpbuf, -1 );
}

int PtlEQWait_timeout(
	ptl_handle_eq_t		eventq_in,
	ptl_event_t		*event_out,
	int			timeout
)
{
	static void	(*prev)(int);
	static int	left_over;
	time_t		time_at_start;
	int		rc;

	if( setjmp( eq_jumpbuf ) ) {
		signal( SIGALRM, prev );
		alarm( left_over - timeout );
		return PTL_EQ_EMPTY;
	}

	left_over = alarm( timeout );
 	prev = signal( SIGALRM, eq_timeout );
	time_at_start = time( NULL );
	if( left_over < timeout )
		alarm( left_over );
	
	rc = PtlEQWait( eventq_in, event_out );

	signal( SIGALRM, prev );
	alarm( left_over );	/* Should compute how long we waited */

	return rc;
}

int PtlEQCount(
	ptl_handle_eq_t		eventq,
	ptl_size_t		*count
)
{
	int 			current		= eventq & 0xFFFF;
	ptl_eq_t		*eq;
	int			i;
	ptl_event_t		*new_event;
	nal_t			*nal;

	nal = ptl_interfaces[ (eventq>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;

	eq = &nal->ni.eq[ current ];
	if( current < 0 || current > MAX_EQS || !eq->inuse )
		return PTL_INV_EQ;

	new_event = eq->base;
	*count = 0;
	for (i= 0; i < eq->size; i++)   {
		if (new_event->sequence >= eq->sequence)   {
			*count= *count + 1;
		}
		new_event++;
	}

	return PTL_OK;
}


int PtlMDUpdate(
	ptl_handle_md_t		md_in,
	ptl_md_t		*old_inout,
	ptl_md_t		*new_inout,
	ptl_handle_eq_t		testq_in
)
{
	int			rc;
	int 			testq		= testq_in & 0xFFFF;
	ptl_eq_t		*eq;
	ptl_seq_t		sequence = -1;
	nal_t			*nal;

	nal = ptl_interfaces[ (md_in>>16) & 0xF ];
	if( !nal )
		return PTL_NOINIT;

	eq = &nal->ni.eq[ testq ];

	if( testq_in != PTL_EQ_NONE ) {
		if( testq < 0 || testq > MAX_EQS || !eq->inuse )
			return PTL_INV_EQ;
		sequence = eq->sequence;
	}

	/*
	 * Validating the addresseses here is a little tricky.
	 * We should validate them now, before performing the update.
	 * That way there is no race condition if a message comes in after
	 * the update but before we validate.
	 *
	 * Cleaning up afterwards is a little tricky, too.
	 */
	if( new_inout ) {
		rc = nal->validate( nal, new_inout->start, new_inout->length );
		if( rc )
			return PTL_SEGV;
	}

	rc = PtlMDUpdate_internal(
		md_in,
		old_inout, new_inout,
		testq_in, sequence
	);

	/*
	 * We do not need to invalidate anything here.  If the memory
	 * descriptor is not updated the library will invalidate the
	 * buffer.
	 */
	return rc;
}
