
#ifndef P30_INIT_H
#define P30_INIT_H

MPID_Device *MPID_P30_InitMsgPass( int *, char ***, int, int );
int  MPID_P30_End( MPID_Device * );
int  MPID_P30_Abort( struct MPIR_COMMUNICATOR *, int, char * );
void MPID_P30_Version_name( char * );

#endif /* P30_INIT_H */
