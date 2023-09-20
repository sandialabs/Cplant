
#ifndef PTLS_SSHORT_H
#define PTLS_SSHORT_H

int  MPID_PTLS_Sshort_send( void *, int, int, int, int, int, int, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Sshort_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			     struct MPIR_COMMUNICATOR * );
int  MPID_PTLS_Sshort_wait_send( MPIR_SHANDLE *shandle );
int  MPID_PTLS_Sshort_test_send( MPIR_SHANDLE *shandle );
void MPID_PTLS_Sshort_delete( MPID_Protocol * );

#endif PTLS_SSHORT_H

