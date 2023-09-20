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
#include"mpi.h"

void user_function( void *v1, void *v2, int *i1, MPI_Datatype *dt1 )
{
    return;
}

main( int ac, char **av )
{
    MPI_Comm        comm;
    MPI_Datatype    datatype;
    MPI_Status      status;
    MPI_Request     request;
    MPI_Group       group;
    MPI_Op          op;

#ifdef MPI2
    MPI_Info        info;
#endif

    void           *buf;
    int             count;
    int             dest;
    int             src;
    int             tag;
    int             flag;
    void           *aint;
    int             range[1][3];
    int            *dims;
    char           *name;

    if ( ac < 32767 ) exit(0);

    MPI_Init( &ac, &av );

    MPI_Send( buf, count, datatype, dest, tag, comm);

    MPI_Recv( buf, count, datatype, src, tag, comm, &status );

    MPI_Get_count( &status, datatype, &count );

    MPI_Bsend( buf, count, datatype, dest, tag, comm);

    MPI_Ssend( buf, count, datatype, dest, tag, comm);

    MPI_Rsend( buf, count, datatype, dest, tag, comm);

    MPI_Buffer_attach( buf, count );

    MPI_Buffer_detach( buf, &count );

    MPI_Isend( buf, count, datatype, dest, tag, comm, &request );

    MPI_Ibsend( buf, count, datatype, dest, tag, comm, &request );

    MPI_Issend( buf, count, datatype, dest, tag, comm, &request );

    MPI_Irsend( buf, count, datatype, dest, tag, comm, &request );

    MPI_Irecv( buf, count, datatype, dest, tag, comm, &request );

    MPI_Wait( &request, &status );

    MPI_Test( &request, count, &status );

    MPI_Request_free( &request );

    MPI_Waitany( count, &request, flag, &status );

    MPI_Testany( count, &request, &flag, &count, &status );

    MPI_Waitall( count, &request, &status );

    MPI_Testall( count, &request, &flag, &status );

    MPI_Waitsome( count, &request, &flag, &count, &status );

    MPI_Testsome( count, &request, &flag, &count, &status );

    MPI_Iprobe( dest, tag, comm, &flag, &status );

    MPI_Probe( dest, tag, comm, &status );

    MPI_Cancel( &request );

    MPI_Test_cancelled( &status, &flag);

    MPI_Send_init( buf, count, datatype, dest, tag, comm, &request );

    MPI_Bsend_init( buf, count, datatype, dest, tag, comm, &request );

    MPI_Ssend_init( buf, count, datatype, dest, tag, comm, &request );

    MPI_Rsend_init( buf, count, datatype, dest, tag, comm, &request );

    MPI_Recv_init( buf, count, datatype, dest, tag, comm, &request );

    MPI_Start( &request );

    MPI_Startall( count, &request );

    MPI_Sendrecv( buf, count, datatype, dest, tag,
                  buf, count, datatype, src, tag, comm, &status );

    MPI_Sendrecv_replace( buf, count, datatype, dest, tag, src, tag,
			  comm, &status );

    MPI_Type_contiguous( count, datatype, &datatype );

    MPI_Type_vector( count, count, count, datatype, &datatype );

    MPI_Type_hvector( count, count, aint, datatype, &datatype );

    MPI_Type_indexed( count, &count, &count, datatype, &datatype );

    MPI_Type_hindexed( count, &count, aint, datatype, &datatype );

    MPI_Type_struct( count, &count, aint, &datatype, &datatype );

    MPI_Address( buf, aint );

    MPI_Type_extent( datatype, aint );

    MPI_Type_size( datatype, &count );

    /* MPI_Type_count( datatype, &count ); */

    MPI_Type_lb( datatype, aint );

    MPI_Type_ub( datatype, aint );

    MPI_Type_commit( &datatype );

    MPI_Type_free( &datatype );

    MPI_Get_elements( &status, datatype, &count );

    MPI_Pack( buf, count, datatype, buf, count, &count,  comm );

    MPI_Unpack( buf, count, &count, buf, count, datatype, comm);

    MPI_Pack_size( count, datatype, comm, &count );

    MPI_Barrier( comm );

    MPI_Bcast( buf, count, datatype, count, comm );

    MPI_Gather( buf, count, datatype, buf, count, datatype, count, comm); 

    MPI_Gatherv( buf, count, datatype, buf, &count, &count, datatype,
		 count, comm); 

    MPI_Scatter( buf, count, datatype, buf, count, datatype, count, comm);

    MPI_Scatterv( buf, &count, &count,  datatype, buf, count, datatype,
		  count, comm);

    MPI_Allgather( buf, count, datatype, buf, count, datatype, comm);

    MPI_Allgatherv( buf, count, datatype, buf, &count, &count, datatype,
		    comm );

    MPI_Alltoall( buf, count, datatype, buf, count, datatype, comm );

    MPI_Alltoallv( buf, &count, &count, datatype, buf, &count, &count,
		   datatype, comm);

    MPI_Reduce( buf, buf, count, datatype, op, count, comm);

    MPI_Op_create( user_function, count, &op );

    MPI_Op_free( &op );

    MPI_Allreduce( buf, buf, count, datatype, op, comm);

    MPI_Reduce_scatter( buf, buf, &count, datatype, op, comm);

    MPI_Scan( buf, buf, count, datatype, op, comm );

    MPI_Group_size( group, &count );

    MPI_Group_rank( group, &count );

    MPI_Group_translate_ranks( group, count, &count, group, &count );

    MPI_Group_compare( group, group, &count );

    MPI_Comm_group( comm, &group );

    MPI_Group_union( group, group, &group );

    MPI_Group_intersection( group, group, &group );

    MPI_Group_difference( group, group, &group );

    MPI_Group_incl( group, count, &count, &group );

    MPI_Group_excl( group, count, &count, group );

    MPI_Group_range_incl( group, count, range, &group );

    MPI_Group_range_excl( group, count, range, &group );

    MPI_Group_free( &group );

    MPI_Comm_size( comm, &count );

    MPI_Comm_rank( comm, &count );

    MPI_Comm_compare( comm, comm, &count );

    MPI_Comm_dup( comm, &comm );

    MPI_Comm_create( comm, group, &comm );

    MPI_Comm_split( comm, count, count, comm );

    MPI_Comm_free( &comm );

    MPI_Comm_test_inter( comm, &count );

    MPI_Comm_remote_size( comm, &count );

    MPI_Comm_remote_group( comm, &group );

    MPI_Intercomm_create( comm, count, comm, count, count, &comm );

    MPI_Intercomm_merge( comm, count, &comm );

    MPI_Keyval_create( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &count, buf );

    MPI_Keyval_free( &count );

    MPI_Attr_put( comm, count, buf );

    MPI_Attr_get( comm, count, buf, &count );

    MPI_Attr_delete( comm, count );

    MPI_Topo_test( comm, count );

    MPI_Cart_create( comm, count, &count, &count, count, &comm );

    MPI_Dims_create( count, count, &count );

    MPI_Graph_create( comm, count, &count, &count, count, &comm );

    MPI_Graphdims_get( comm, &count, &count );

    MPI_Graph_get( comm, count, count, &count, &count );

    MPI_Cartdim_get( comm, &count );

    MPI_Cart_get( comm, count, dims, dims, dims );

    MPI_Cart_rank( comm, dims, &dest );

    MPI_Cart_coords( comm, count, count, &count );

    MPI_Graph_neighbors_count( comm, count, &count );

    MPI_Graph_neighbors( comm, count, count, &count );

    MPI_Cart_shift( comm, count, count, &count, &count );

    MPI_Cart_sub( comm, &count, comm );

    MPI_Cart_map( comm, count, &count, &count, &count );

    MPI_Graph_map( comm, count, &count, &count, &count );

    MPI_Get_processor_name( name, &count );

    MPI_Errhandler_create( MPI_ERRORS_RETURN, MPI_ERRORS_RETURN );

    MPI_Errhandler_set( comm, MPI_ERRORS_RETURN );

    MPI_Errhandler_get( comm, MPI_ERRORS_RETURN );

    MPI_Errhandler_free( MPI_ERRORS_RETURN );

    MPI_Error_string( count, name, &count );

    MPI_Error_class( count, &count );

    (void)MPI_Wtime();

    (void)MPI_Wtick();

    MPI_Initialized( &count );

    MPI_Abort(comm, count );

#ifdef MPI2
    MPI_Get_version( &count, &count );

    MPI_Comm_set_name( comm, name );

    MPI_Comm_get_name( comm, name, &count );

    MPI_Status_f2c( &count, &status );

    MPI_Status_c2f( &status, &count );

    MPI_Finalized( &count );

    MPI_Type_create_indexed_block(count, count, &count, datatype, 
				  &datatype );

    MPI_Type_get_envelope(datatype, &count, &count, &count, &count); 

    MPI_Type_get_contents(datatype, count, count, count, &count, 
			  &count, &datatype );

    MPI_Type_create_subarray(count, &count, &count, &count, count, 
			     datatype, &datatype );

    MPI_Type_create_darray(count, count, count, &count, &count, &count, &count, 
			   count, datatype, &datatype );

    MPI_Info_create( &info );

    MPI_Info_set( &info, name, name );

    MPI_Info_delete( &info, name );

    MPI_Info_get( info, name, count, name, &count );

    MPI_Info_get_valuelen( info, name, &count, &flag);

    MPI_Info_get_nkeys( info, &count );

    MPI_Info_get_nthkey( info, count, name );

    MPI_Info_dup( info, &info );

    MPI_Info_free( &info );

    MPI_Info_c2f( info );

    MPI_Info_f2c( count );

    MPI_Request_c2f( request );

#endif

    MPI_Finalize();
}
