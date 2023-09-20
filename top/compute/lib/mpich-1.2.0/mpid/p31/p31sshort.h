
#ifndef P31_SSHORT_H
#define P31_SSHORT_H

int  MPID_P31_Sshort_send( void *, int, int, int, int, int, int, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_P31_Sshort_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			     struct MPIR_COMMUNICATOR * );
int  MPID_P31_Sshort_wait( MPIR_SHANDLE *shandle );
int  MPID_P31_Sshort_test( MPIR_SHANDLE *shandle );
void MPID_P31_Sshort_delete( MPID_Protocol * );

#endif /* P31_SSHORT_H */

