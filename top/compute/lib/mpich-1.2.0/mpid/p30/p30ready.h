

#ifndef P30_READY_H
#define P30_READY_H

int  MPID_P30_Ready_send( void *, int, int, int, int, int, int, 
			   struct MPIR_COMMUNICATOR * );
int  MPID_P30_Ready_isend( void *, int, int, int, int, int, int, MPIR_SHANDLE *, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_P30_Ready_test_send( MPIR_SHANDLE *);
int  MPID_P30_Ready_wait_send( MPIR_SHANDLE *);
void MPID_P30_Ready_delete( MPID_Protocol * );
MPID_Protocol *MPID_P30_Ready_setup( void );

#endif /* P30_READY_H */
