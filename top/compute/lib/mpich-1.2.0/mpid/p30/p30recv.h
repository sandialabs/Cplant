
#ifndef P30_RECV_H
#define P30_RECV_H

/* prototypes */
int  MPID_P30_Test_recv( MPIR_RHANDLE * );
int  MPID_P30_Wait_recv( MPIR_RHANDLE * );
int  MPID_P30_Test_pulled( MPIR_RHANDLE * );
int  MPID_P30_Wait_pulled( MPIR_RHANDLE * );

#endif /* P30_RECV_H */
