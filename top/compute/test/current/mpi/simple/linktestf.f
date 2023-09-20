
      program linktest
      
      include"mpif.h"

      integer ierr
      
      integer comm
      integer datatype
      integer status(MPI_STATUS_SIZE)
      integer request
      integer group
      integer op

      external user_function

      real*8  d
      integer buf
      integer count
      integer dest
      integer src
      integer tag
      logical flag
      integer aint
      integer range(1)
      integer dims(1)
      character*(MPI_MAX_PROCESSOR_NAME) name

      integer ac

      ac = iargc()

      if ( ac.lt.32767 ) then
         call exit( 0 )
      endif

      call MPI_Init( ac, av, ierr )

      call MPI_Send( buf, count, datatype, dest, tag, comm,
     +               ierr )
      
      call MPI_Recv( buf, count, datatype, src, tag, comm,
     +               status, ierr )

      call MPI_Get_count( status, datatype, count, ierr )

      call MPI_Bsend( buf, count, datatype, dest, tag, comm, ierr)

      call MPI_Ssend( buf, count, datatype, dest, tag, comm, ierr)

      call MPI_Rsend( buf, count, datatype, dest, tag, comm, ierr)

      call MPI_Buffer_attach( buf, count, ierr )

      call MPI_Buffer_detach( buf, count, ierr )

      call MPI_Isend( buf, count, datatype, dest, tag, comm,
     +                request, ierr )

      call MPI_Ibsend( buf, count, datatype, dest, tag, comm,
     +                 request, ierr )

      call MPI_Issend( buf, count, datatype, dest, tag, comm,
     +                 request, ierr )

      call MPI_Irsend( buf, count, datatype, dest, tag, comm,
     +                 request, ierr )

      call MPI_Irecv( buf, count, datatype, dest, tag, comm,
     +                request, ierr )

      call MPI_Wait( request, status, ierr )

      call MPI_Test( request, count, status, ierr )

      call MPI_Request_free( request, ierr )

      call MPI_Waitany( count, request, flag, status, ierr )

      call MPI_Testany( count, request, flag, count, status,
     +                  ierr )

      call MPI_Waitall( count, request, status, ierr )

      call MPI_Testall( count, request, flag, status, ierr )

      call MPI_Waitsome( count, request, flag, count, status,
     +                   ierr )

      call MPI_Testsome( count, request, flag, count, status,
     +                   ierr )

      call MPI_Iprobe( dest, tag, comm, flag, status, ierr )

      call MPI_Probe( dest, tag, comm, status, ierr )

      call MPI_Cancel( request, ierr )

      call MPI_Test_cancelled( status, flag)

      call MPI_Send_init( buf, count, datatype, dest, tag, comm,
     +                    request, ierr )

      call MPI_Bsend_init( buf, count, datatype, dest, tag, comm,
     +                     request, ierr )

      call MPI_Ssend_init( buf, count, datatype, dest, tag, comm,
     +                     request, ierr )

      call MPI_Rsend_init( buf, count, datatype, dest, tag, comm,
     +                     request, ierr )

      call MPI_Recv_init( buf, count, datatype, dest, tag, comm,
     +                    request, ierr )

      call MPI_Start( request, ierr )

      call MPI_Startall( count, request, ierr )

      call MPI_Sendrecv( buf, count, datatype, dest, tag,
     +                   buf, count, datatype, src, tag, comm,
     +                   status, ierr )

      call MPI_Sendrecv_replace( buf, count, datatype, dest, tag,
     +                           src, tag, comm, status, ierr )

      call MPI_Type_contiguous( count, datatype, datatype, ierr )

      call MPI_Type_vector( count, count, count, datatype,
     +                      datatype, ierr )

      call MPI_Type_hvector( count, count, aint, datatype,
     +                       datatype, ierr )

      call MPI_Type_indexed( count, count, count, datatype,
     +                       datatype, ierr )

      call MPI_Type_hindexed( count, count, aint, datatype,
     +                        datatype, ierr )

      call MPI_Type_struct( count, count, aint, datatype,
     +                      datatype, ierr )

      call MPI_Address( buf, aint, ierr )

      call MPI_Type_extent( datatype, aint, ierr )

      call MPI_Type_size( datatype, count, ierr )

C      call MPI_Type_count( datatype, count, ierr )

      call MPI_Type_lb( datatype, aint, ierr )

      call MPI_Type_ub( datatype, aint, ierr )

      call MPI_Type_commit( datatype, ierr )

      call MPI_Type_free( datatype, ierr )

      call MPI_Get_elements( status, datatype, count, ierr )

      call MPI_Pack( buf, count, datatype, buf, count, count,
     +               comm, ierr )

      call MPI_Unpack( buf, count, count, buf, count, datatype,
     +                 comm, ierr )

      call MPI_Pack_size( count, datatype, comm, count, ierr )

      call MPI_Barrier( comm, ierr )

      call MPI_Bcast( buf, count, datatype, count, comm, ierr )

      call MPI_Gather( buf, count, datatype, buf, count,
     +                 datatype, count, comm, ierr ) 

      call MPI_Gatherv( buf, count, datatype, buf, count, count,
     +                  datatype, count, comm, ierr)

      call MPI_Scatter( buf, count, datatype, buf, count,
     +                  datatype, count, comm, ierr )

      call MPI_Scatterv( buf, count, count,  datatype, buf, count,
     +                   datatype, count, comm, ierr )

      call MPI_Allgather( buf, count, datatype, buf, count,
     +                    datatype, comm, ierr )

      call MPI_Allgatherv( buf, count, datatype, buf, count, count,
     +                     datatype, comm, ierr )

      call MPI_Alltoall( buf, count, datatype, buf, count, datatype,
     +                   comm, ierr )

      call MPI_Alltoallv( buf, count, count, datatype, buf, count,
     +                    count, datatype, comm, ierr )

      call MPI_Reduce( buf, buf, count, datatype, op, count, comm,
     +                 ierr)

      call MPI_Op_create( user_function, count, op, ierr )

      call MPI_Op_free( op, ierr )

      call MPI_Allreduce( buf, buf, count, datatype, op, comm, ierr)

      call MPI_Reduce_scatter( buf, buf, count, datatype, op,
     +                         comm, ierr)

      call MPI_Scan( buf, buf, count, datatype, op, comm, ierr )

      call MPI_Group_size( group, count, ierr )

      call MPI_Group_rank( group, count, ierr )

      call MPI_Group_translate_ranks( group, count, count, group,
     +                                count, ierr )

      call MPI_Group_compare( group, group, count, ierr )

      call MPI_Comm_group( comm, group, ierr )

      call MPI_Group_union( group, group, group, ierr )

      call MPI_Group_intersection( group, group, group, ierr )

      call MPI_Group_difference( group, group, group, ierr )

      call MPI_Group_incl( group, count, count, group, ierr )

      call MPI_Group_excl( group, count, count, group, ierr )

      call MPI_Group_range_incl( group, count, range, group, ierr )

      call MPI_Group_range_excl( group, count, range, group, ierr )

      call MPI_Group_free( group, ierr )

      call MPI_Comm_size( comm, count, ierr )

      call MPI_Comm_rank( comm, count, ierr )

      call MPI_Comm_compare( comm, comm, count, ierr )

      call MPI_Comm_dup( comm, comm, ierr )

      call MPI_Comm_create( comm, group, comm, ierr )

      call MPI_Comm_split( comm, count, count, comm, ierr )

      call MPI_Comm_free( comm, ierr )

      call MPI_Comm_test_inter( comm, count, ierr )

      call MPI_Comm_remote_size( comm, count, ierr )

      call MPI_Comm_remote_group( comm, group, ierr )

      call MPI_Intercomm_create( comm, count, comm, count, count,
     +                           comm, ierr )

      call MPI_Intercomm_merge( comm, count, comm, ierr )

      call MPI_Keyval_create( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,
     +                        count, buf, ierr )

      call MPI_Keyval_free( count, ierr )

      call MPI_Attr_put( comm, count, buf, ierr )

      call MPI_Attr_get( comm, count, buf, count, ierr )

      call MPI_Attr_delete( comm, count, ierr )

      call MPI_Topo_test( comm, count, ierr )

      call MPI_Cart_create( comm, count, count, count, count,
     +                      comm, ierr )

      call MPI_Dims_create( count, count, count, ierr )

      call MPI_Graph_create( comm, count, count, count, count,
     +                       comm, ierr )

      call MPI_Graphdims_get( comm, count, count, ierr )

      call MPI_Graph_get( comm, count, count, count, count, ierr )

      call MPI_Cartdim_get( comm, count, ierr )

      call MPI_Cart_get( comm, count, dims, dims, dims, ierr )

      call MPI_Cart_rank( comm, dims, dest, ierr )

      call MPI_Cart_coords( comm, count, count, count, ierr )

      call MPI_Graph_neighbors_count( comm, count, count, ierr )

      call MPI_Graph_neighbors( comm, count, count, count, ierr )

      call MPI_Cart_shift( comm, count, count, count, count, ierr )

      call MPI_Cart_sub( comm, count, comm, ierr )

      call MPI_Cart_map( comm, count, count, count, count, ierr )

      call MPI_Graph_map( comm, count, count, count, count, ierr )

      call MPI_Get_processor_name( name, count, ierr )

      call MPI_Errhandler_create( MPI_ERRORS_RETURN, 
     +                            MPI_ERRORS_RETURN, ierr )

      call MPI_Errhandler_set( comm, MPI_ERRORS_RETURN, ierr )

      call MPI_Errhandler_get( comm, MPI_ERRORS_RETURN, ierr )

      call MPI_Errhandler_free( MPI_ERRORS_RETURN, ierr )

      call MPI_Error_string( count, name, count, ierr )

      call MPI_Error_class( count, count, ierr )

      d = MPI_Wtime()

      d = MPI_Wtick()

      call MPI_Initialized( count, ierr )

      call MPI_Abort(comm, count, ierr )

      call MPI_Finalize()

      stop

      end

      subroutine user_function
      
      return
      end
