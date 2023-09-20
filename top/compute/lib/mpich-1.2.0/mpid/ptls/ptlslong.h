
#ifndef PTLS_LONG_H
#define PTLS_LONG_H

int  MPID_PTLS_Long_send( void *, int, int, int, int, int, int, 
			  struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Long_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			   struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Long_wait_send( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Long_test_send( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Long_wait_send_ack( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Long_test_send_ack( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Long_wait_send_read( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Long_test_send_read( MPIR_SHANDLE *shandle );
void MPID_PTLS_Long_delete( MPID_Protocol * );
MPID_Protocol *MPID_PTLS_Long_setup( void );

#endif PTLS_LONG_H
