
#ifndef P31_RECV_H
#define P31_RECV_H

/* prototypes */
int  MPID_P31_Test_recv( MPIR_RHANDLE * );
int  MPID_P31_Wait_recv( MPIR_RHANDLE * );
int  MPID_P31_Test_pulled( MPIR_RHANDLE * );
int  MPID_P31_Wait_pulled( MPIR_RHANDLE * );

#endif /* P31_RECV_H */
