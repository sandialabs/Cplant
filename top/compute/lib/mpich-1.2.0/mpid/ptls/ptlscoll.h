

#ifndef PTLS_COLL_H
#define PTLS_COLL_H

/* macro for managing collective workspace buffer */
#define PTLS_COLL_WORK_MALLOC(bytes)                                          \
((bytes) < ptls_coll_work_size) ?                                             \
(ptls_coll_work) :                                                            \
(ptls_coll_work = (VOID *)realloc(ptls_coll_work, (bytes) ))

/*
** Counters are needed on a per communicator basis to
** separate collective operations from each other on
** a per communicator basis.  It is assumed that:
**     1) The members of the communicator group will
**        be participating in at most 1 collective
**        communication at any given time.
**     2) Leaf nodes of a fanin operation such as
**        an MPI_Gather() or MPI_Reduce() may submit
**        their contributions and move on to yet another
**        fanin operation.  In this case, it is assumed
**        that the parents of the leaf nodes will handle
**        this extra flow with no more than COLL_CNTR_MAX 
**        outstanding submissions by the leaf nodes.
*/
#define COLL_CNTR_MAX		(0xff) 
#define COLL_TAG( cntxt, cntr )						    \
    ( (((cntr) = ((cntr)<COLL_CNTR_MAX ? (cntr)+1 : 0)) < 0) ? 0 : 	    \
      ((int)(((unsigned int)(cntxt)<<16) | (unsigned int)(cntr) )) )

/* global variables */
extern CHAR                 **ptls_data_ptrs;
extern VOID                  *ptls_coll_work;
extern int                    ptls_coll_work_size;
extern struct _MPIR_COLLOPS   ptls_collops;
extern struct _MPIR_COLLOPS  *default_collops;

/* functions */
int PTLS_Barrier( struct MPIR_COMMUNICATOR *comm );
int PTLS_Bcast( void* buffer, int count, struct MPIR_DATATYPE *datatype, int root, 
	       struct MPIR_COMMUNICATOR *comm );
int PTLS_Gather( void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		int recvcount, struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm);
int PTLS_Gatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		 int *recvcounts, int *displs, struct MPIR_DATATYPE *recvtype, int root, 
		 struct MPIR_COMMUNICATOR *comm);
int PTLS_Scatter(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		 int recvcount, struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm);
int PTLS_Scatterv(void* sendbuf, int *sendcounts, int *displs, struct MPIR_DATATYPE *sendtype, 
		  void* recvbuf, int recvcount, struct MPIR_DATATYPE *recvtype, int root, 
		  struct MPIR_COMMUNICATOR *comm);
int PTLS_Allgather(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		   int recvcount, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm);
int PTLS_Allgatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		    int *recvcounts, int *displs, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm);
int PTLS_Alltoall(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		  int recvcount, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm);
int PTLS_Alltoallv(void* sendbuf, int *sendcounts, int *sdispls, struct MPIR_DATATYPE *sendtype, 
		   void* recvbuf, int *recvcounts, int *rdispls, struct MPIR_DATATYPE *recvtype, 
		   struct MPIR_COMMUNICATOR *comm);
int PTLS_Reduce(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
		MPI_Op op, int root, struct MPIR_COMMUNICATOR *comm);
int PTLS_Allreduce(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
		   MPI_Op op, struct MPIR_COMMUNICATOR *comm);
int PTLS_Reduce_scatter(void* sendbuf, void* recvbuf, int *recvcounts, 
			struct MPIR_DATATYPE *datatype, MPI_Op op, struct MPIR_COMMUNICATOR *comm);
int PTLS_Scan(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
	      MPI_Op op, struct MPIR_COMMUNICATOR *comm );
int  PTLS_Choose_Bcast( void*, int, struct MPIR_DATATYPE *, int, 
		        struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Gather( void*, int, struct MPIR_DATATYPE *, void*, int, 
			 struct MPIR_DATATYPE * , int, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Gatherv( void*, int, struct MPIR_DATATYPE *, void*, int *, int *, 
			  struct MPIR_DATATYPE *, int, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Scatter( void*, int, struct MPIR_DATATYPE *, void*, int, 
			  struct MPIR_DATATYPE *, int, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Scatterv( void*, int *, int *, struct MPIR_DATATYPE *, void*, int, 
			   struct MPIR_DATATYPE *, int, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Allgather( void*, int, struct MPIR_DATATYPE *, void*, int, 
			    struct MPIR_DATATYPE *, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Allgatherv( void*, int, struct MPIR_DATATYPE *, void*, int *, int *, 
			     struct MPIR_DATATYPE *, struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Reduce( void*, void*, int, struct MPIR_DATATYPE *, MPI_Op, int, 
			 struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Allreduce( void*, void*, int, struct MPIR_DATATYPE *, MPI_Op, 
			    struct MPIR_COMMUNICATOR * );
int  PTLS_Choose_Reduce_scatter( void*, void*, int *, struct MPIR_DATATYPE *, 
				 MPI_Op, struct MPIR_COMMUNICATOR * );

#endif PTLS_COLL_H
