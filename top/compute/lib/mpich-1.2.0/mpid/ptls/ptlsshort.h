
#ifndef PTLS_SHORT_H
#define PTLS_SHORT_H

int  MPID_PTLS_Short_send( void *, int, int, int, int, int, int, 
			   struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Short_isend( void *, int, int, int, int, int, int, 
			    MPIR_SHANDLE *, struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Short_bsend( void *, int, int, int, int, int, MPIR_SHANDLE *, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Short_wait_send( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Short_test_send( MPIR_SHANDLE *shandle );
void MPID_PTLS_Short_delete( MPID_Protocol * );
MPID_Protocol *MPID_PTLS_Short_setup();

#endif PTLS_SHORT_H
