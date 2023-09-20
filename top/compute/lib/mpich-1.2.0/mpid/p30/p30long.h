
#ifndef P30_LONG_H
#define P30_LONG_H

int  MPID_P30_Long_send( void *, int, int, int, int, int, int, 
			  struct MPIR_COMMUNICATOR * );
int  MPID_P30_Long_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			   struct MPIR_COMMUNICATOR * );
void MPID_P30_Long_send_cleanup( MPIR_SHANDLE *shandle );
int  MPID_P30_Long_wait_send( MPIR_SHANDLE *shandle );
int  MPID_P30_Long_test_send( MPIR_SHANDLE *shandle );
int  MPID_P30_Long_wait_send_read( MPIR_SHANDLE *shandle );
int  MPID_P30_Long_test_send_read( MPIR_SHANDLE *shandle );
void MPID_P30_Long_delete( MPID_Protocol * );
MPID_Protocol *MPID_P30_Long_setup( void );

#endif /* P30_LONG_H */
