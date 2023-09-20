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
#include <stdio.h>
#include <p30.h>
#include <p30/internal.h>

static void test_fail( const char *str, int rc )
{
	fprintf( stderr, "FAILED: %s: error=%s (%d)\n",
		str, ptl_err_str[rc], rc );
	exit(-1);
}

int main( int argc, char *argv[] )
{
	int rc;
	ptl_handle_ni_t		ni;
	ptl_handle_me_t		me, me2, me3;
	ptl_md_t		md_real;
	ptl_handle_md_t		md, md2;
	ptl_handle_eq_t		eq;
	ptl_event_t		ev;
	ptl_id_t		nodes;
	ptl_process_id_t	my_id, id = {
		PTL_ADDR_NID,
		37, 14
	};

	if( (rc=PtlInit()) )
		test_fail( "PtlInit", rc );

	if( (rc=PtlNIInit( PTL_IFACE_DEFAULT, 32, 4, &ni )) ) {
		fprintf( stderr,
			"\n\n\a-------------\n"
			"You should start this with the fork command to\n"
			"ensure that all of the environment variables are\n"
			"properly initialized\n"
			"\a-------------\n\n\n"
		);
		test_fail( "PtlNIInit", rc );
	}

	if( (rc=PtlGetId( ni, &my_id, &nodes ) ) )
		test_fail( "PtlGetId", rc );

	printf( "%d: gid=%d out of %d nodes\n", my_id.rid, my_id.gid, nodes );
		
	printf( "going to attach ni=%d rc=%d\n", ni, rc );

	if( (rc= PtlMEAttach( ni, 4, id,
		0xDEADBEEF, 0xBADBABE,
		PTL_RETAIN, &me
	)) )
		test_fail( "PtlMEAttach 1", rc );
	
	if( (rc= PtlMEInsert( me, id,
		0xDEADBEEF, 0xBADBAFE,
		PTL_RETAIN, PTL_INS_AFTER, &me2
	)) )
		test_fail( "PtlMEInsert 2", rc );



	if(( rc = PtlMEInsert( me, id,
		0xDEADBEEF, 0xBADBAFE,
		PTL_RETAIN, PTL_INS_BEFORE, &me3
	) ))
		test_fail( "PtlMEInsert 3", rc );

		

	if( (rc = PtlMEUnlink( me )) )
		test_fail( "PtlMEUnlink 0", rc );



	if( (rc = PtlMEUnlink( me2 )) )
		test_fail( "PtlMEUnlink 1", rc );


	if( (rc = PtlEQAlloc( ni, 128, &eq )) )
		test_fail( "PtlEQAlloc 0", rc );

	/*
	 * Fill in the MD and attach it
	 */
	md_real.start		= (void*) 0xDEADBEEF;
	md_real.length		= 1024;
	md_real.threshold	= 2;
	md_real.max_offset      = md_real.length;
	md_real.options		= PTL_MD_OP_PUT | PTL_MD_OP_GET;
	md_real.user_ptr	= NULL;
	md_real.eventq		= eq;

	if( (rc = PtlMDAttach( me3, md_real, PTL_RETAIN, &md )) )
		test_fail( "PtlMDAttach 0", rc );


	printf( "We have md %d\n", md );
	md_real.eventq		= PTL_EQ_NONE;
	if( (rc = PtlMDInsert( md, md_real, PTL_UNLINK, PTL_INS_BEFORE, &md2 )))
		test_fail( "PtlMDInsert 1", rc );


	if( (rc = PtlEQGet( eq, &ev) ) != PTL_EQ_EMPTY )
		test_fail( "PtlEQGet", rc );

	id.addr_kind = PTL_ADDR_GID;
	id.rid = ( id.rid + 1 ) % nodes;

	if( (rc = PtlPut( md2, PTL_NOACK_REQ, id , 4, 0xDEADBEEF, 0xDEADBEEF, 0, 0 ) ) ) 
		test_fail( "PtlPut", rc );

	PtlEQFree( eq );
	printf( "exiting...\n");
	return 0;
}


