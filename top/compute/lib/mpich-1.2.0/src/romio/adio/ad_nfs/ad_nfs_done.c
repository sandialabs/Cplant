/* 
 *   $Id: ad_nfs_done.c,v 1.1 2000/05/10 21:42:37 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

int ADIOI_NFS_ReadDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    int done=0;
#ifndef __NO_AIO
#ifdef __AIO_SUN 
    aio_result_t *result=0, *tmp;
#else
    int err, nbytes;
#endif
#ifdef __AIO_HANDLE_IN_AIOCB
    struct aiocb *tmp1;
#endif
#endif

    if (*request == ADIO_REQUEST_NULL) {
	*error_code = MPI_SUCCESS;
	return 1;
    }

    if ((*request)->next != ADIO_REQUEST_NULL) {
	done = ADIOI_NFS_ReadDone(&((*request)->next), status, error_code);
    /* currently passing status and error_code here, but something else
       needs to be done to get the status and error info correctly */
	if (!done) {
	   *error_code = MPI_SUCCESS;
	   return done;
	}
    }

#ifdef __NO_AIO
/* HP, FreeBSD, Linux */
    (*request)->fd->async_count--;
    ADIOI_Free_request((ADIOI_Req_node *) (*request));
    *request = ADIO_REQUEST_NULL;
    *error_code = MPI_SUCCESS;
    return 1;
#endif    

#ifdef __AIO_SUN
    if ((*request)->queued) {
	tmp = (aio_result_t *) (*request)->handle;
	if (tmp->aio_return == AIO_INPROGRESS) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    result = (aio_result_t *) aiowait(0); /* dequeue any one request */
	    done = 1;
	    *error_code = ((int) result == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
	    /* the error could have been for some other request, since we
	       don't know which request is being dequeued, but might as well
	       flag it here. */
	}
    }
    else {
	/* ADIOI_Complete_Async completed this request, but request object
           was not freed. */
	done = 1;
	*error_code = MPI_SUCCESS;
    }
#endif

#ifdef __AIO_HANDLE_IN_AIOCB
/* IBM */
    if ((*request)->queued) {
	tmp1 = (struct aiocb *) (*request)->handle;
	err = aio_error(tmp1->aio_handle);
	if (err == EINPROG) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    nbytes = aio_return(tmp1->aio_handle);

/* on DEC, it is required to call aio_return to dequeue the request.
   IBM man pages don't indicate what function to use for dequeue.
   I'm assuming it is aio_return! */
	
	    done = 1;
	    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
	    /* status to be filled */
	}
    }
    else {
	done = 1;
	*error_code = MPI_SUCCESS;
    }
#elif (!defined(__NO_AIO) && !defined(__AIO_SUN))
/* DEC, SGI IRIX 5 and 6 */
    if ((*request)->queued) {
	err = aio_error((const struct aiocb *) (*request)->handle);
	if (err == EINPROGRESS) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    nbytes = aio_return((struct aiocb *) (*request)->handle); 
	    /* also dequeues the request*/ 
	    /*  if (err) printf("error in testing completion of nonblocking I/O\n");*/
	    done = 1;
	    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
	    /* status to be filled */
	}
    }
    else {
	done = 1;
	*error_code = MPI_SUCCESS;
    }
#endif

#ifndef __NO_AIO
    if (done) {
	/* if request is still queued in the system, it is also there
           on ADIOI_Async_list. Delete it from there. */
	if ((*request)->queued) ADIOI_Del_req_from_list(request);

	(*request)->fd->async_count--;
	if ((*request)->handle) ADIOI_Free((*request)->handle);
	ADIOI_Free_request((ADIOI_Req_node *) (*request));
	*request = ADIO_REQUEST_NULL;
	/* status to be filled */
    }
    return done;
#endif

}


int ADIOI_NFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    return ADIOI_NFS_ReadDone(request, status, error_code);
} 
