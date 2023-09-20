
#ifndef P30_SSHORT_H
#define P30_SSHORT_H

int  MPID_P30_Sshort_send( void *, int, int, int, int, int, int, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_P30_Sshort_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			     struct MPIR_COMMUNICATOR * );
int  MPID_P30_Sshort_wait( MPIR_SHANDLE *shandle );
int  MPID_P30_Sshort_test( MPIR_SHANDLE *shandle );
void MPID_P30_Sshort_delete( MPID_Protocol * );

#endif /* P30_SSHORT_H */
