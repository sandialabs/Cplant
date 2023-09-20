/* 
 *   $Id: ad_nfs_wait.c,v 1.1 2000/05/10 21:42:38 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

void ADIOI_NFS_ReadComplete(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
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
	return;
    }

    if (((*request)->next != ADIO_REQUEST_NULL) && ((*request)->queued != -1))
	/* the second condition is to take care of the ugly hack in
            ADIOI_Complete_async */
	ADIOI_NFS_ReadComplete(&((*request)->next), status, error_code);
    /* currently passing status and error_code here, but something else
       needs to be done to get the status and error info correctly */

#ifdef __AIO_SUN
    if ((*request)->queued) {  /* dequeue it */
	tmp = (aio_result_t *) (*request)->handle;
	while (tmp->aio_return == AIO_INPROGRESS) usleep(1000); 
	/* sleep for 1 ms., until done. Is 1 ms. a good number? */
	/* when done, dequeue any one request */
	result = (aio_result_t *) aiowait(0);
	*error_code = ((int) result == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
        /* the error could have been for some other request, since we
           don't know which request is being dequeued, but might as well
           flag it here. */

/* aiowait only dequeues a request. The completion of a request can be
   checked by just checking the aio_return flag in the handle passed
   to the original aioread()/aiowrite(). Therefore, I need to ensure
   that aiowait() is called exactly once for each previous
   aioread()/aiowrite(). This is also taken care of in ADIOI_xxxDone */
    }
    else *error_code = MPI_SUCCESS;
#endif
    
#ifdef __AIO_HANDLE_IN_AIOCB
/* IBM */
    if ((*request)->queued) {
	do {
	    err = aio_suspend(1, (struct aiocb **) &((*request)->handle));
	} while ((err < 0) && (errno == EINTR));
	tmp1 = (struct aiocb *) (*request)->handle;
	if (!err) nbytes = aio_return(tmp1->aio_handle);
	else nbytes = 0;

/* on DEC, it is required to call aio_return to dequeue the request.
   IBM man pages don't indicate what function to use for dequeue.
   I'm assuming it is aio_return! */

	*error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
    }
    else *error_code = MPI_SUCCESS;

#elif (!defined(__NO_AIO) && !defined(__AIO_SUN))
/* DEC, SGI IRIX 5 and 6 */
    if ((*request)->queued) {
	do {
	    err = aio_suspend((const aiocb_t **) &((*request)->handle), 1, 0);
	} while ((err < 0) && (errno == EINTR));
	if (!err) nbytes = aio_return((struct aiocb *) (*request)->handle); 
	else nbytes = 0;
	/* also dequeues the request, at least on DEC */ 
	*error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
    }
    else *error_code = MPI_SUCCESS;
#endif

#ifndef __NO_AIO
    if ((*request)->queued != -1) {

	/* queued = -1 is an internal hack used when the request must
	   be completed, but the request object should not be
	   freed. This is used in ADIOI_Complete_async, because the user
	   will call MPI_Wait later, which would require status to
	   be filled. Ugly but works. queued = -1 should be used only
	   in ADIOI_Complete_async. 
           This should not affect the user in any way. */

	/* if request is still queued in the system, it is also there
           on ADIOI_Async_list. Delete it from there. */
	if ((*request)->queued) ADIOI_Del_req_from_list(request);

	(*request)->fd->async_count--;
	if ((*request)->handle) ADIOI_Free((*request)->handle);
	ADIOI_Free_request((ADIOI_Req_node *) (*request));
	*request = ADIO_REQUEST_NULL;
    }

#else

/* HP, FreeBSD, Linux */
    (*request)->fd->async_count--;
    ADIOI_Free_request((ADIOI_Req_node *) (*request));
    *request = ADIO_REQUEST_NULL;
    *error_code = MPI_SUCCESS;
#endif    

/* status to be filled */
}


void ADIOI_NFS_WriteComplete(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    ADIOI_NFS_ReadComplete(request, status, error_code);
}
