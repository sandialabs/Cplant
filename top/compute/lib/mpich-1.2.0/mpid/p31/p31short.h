
#ifndef P31_SHORT_H
#define P31_SHORT_H

int  MPID_P31_Short_send( void *, int, int, int, int, int, int, 
			   struct MPIR_COMMUNICATOR * );
int  MPID_P31_Short_isend( void *, int, int, int, int, int, int, 
			    MPIR_SHANDLE *, struct MPIR_COMMUNICATOR * );
int  MPID_P31_Short_bsend( void *, int, int, int, int, int, MPIR_SHANDLE *, 
			    struct MPIR_COMMUNICATOR * );
int  MPID_P31_Short_wait_send( MPIR_SHANDLE *shandle );
int  MPID_P31_Short_test_send( MPIR_SHANDLE *shandle );
void MPID_P31_Short_delete( MPID_Protocol * );
MPID_Protocol *MPID_P31_Short_setup();

#endif /* P31_SHORT_H */
