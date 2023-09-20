

#ifndef PTLS_READY_H
#define PTLS_READY_H

int  MPID_PTLS_Ready_send( void *, int, int, int, int, int, int, 
			   struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Ready_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Ready_test_send( MPIR_SHANDLE *);
int  MPID_PTLS_Ready_wait_send( MPIR_SHANDLE *);
void MPID_PTLS_Ready_delete( MPID_Protocol * );
MPID_Protocol *MPID_PTLS_Ready_setup( void );

#endif PTLS_READY_H
