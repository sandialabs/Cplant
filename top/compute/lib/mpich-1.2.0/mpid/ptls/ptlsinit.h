
#ifndef PTLS_INIT_H
#define PTLS_INIT_H

MPID_Device *MPID_PTLS_InitMsgPass( int *, char ***, int, int );
int  MPID_PTLS_End( MPID_Device * );
int  MPID_PTLS_Abort( struct MPIR_COMMUNICATOR *, int, char * );
void MPID_PTLS_Version_name( char * );

#endif PTLS_INIT_H
