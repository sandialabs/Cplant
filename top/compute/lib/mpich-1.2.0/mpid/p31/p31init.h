
#ifndef P31_INIT_H
#define P31_INIT_H

MPID_Device *MPID_P31_InitMsgPass( int *, char ***, int, int );
int  MPID_P31_End( MPID_Device * );
int  MPID_P31_Abort( struct MPIR_COMMUNICATOR *, int, char * );
void MPID_P31_Version_name( char * );

#endif /* P31_INIT_H */
