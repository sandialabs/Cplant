
#ifndef PTLS_PRIV_H
#define PTLS_PRIV_H


extern int                ptls_myid;
extern int                ptls_nnodes;
extern int                ptls_gid;
       
extern int                ptls_match_list_size;
extern int                ptls_heap_size;
extern int                ptls_short_msg_size;
extern int                ptls_read_match_list_size;

extern PORTAL_INDEX       ptls_recv_portal;
extern PORTAL_INDEX       ptls_ack_portal;
extern PORTAL_INDEX       ptls_read_portal;

extern PORTAL_DESCRIPTOR *ack_ptl_desc;

extern MATCH_DESC_TYPE   *recv_desc_list;
extern MATCH_DESC_TYPE   *ack_desc_list;
extern MATCH_DESC_TYPE   *read_desc_list;
       
extern MATCH_DESC_TYPE   *recv_head_desc;
extern MATCH_DESC_TYPE   *ack_head_desc;
extern MATCH_DESC_TYPE   *read_head_desc;

extern MATCH_DESC_TYPE   *recv_short_catchall_desc;
extern MATCH_DESC_TYPE   *recv_long_catchall_desc;

extern UINT32             recv_short_catchall_match_index;

extern MATCH_DESC_TYPE   *recv_last_reg_desc;
extern int                recv_last_reg_index;

extern int               *recv_free_list,
                          recv_first_free;
extern int               *ack_free_list,
                          ack_first_free;
extern int               *read_free_list,
                          read_first_free;
       
extern int               *recv_previous_list;

extern IND_MD_BUF_DESC   *ack_ind_bufs;
extern IND_MD_BUF_DESC   *recv_ind_bufs;
       
extern void              *dynamic_heap;

extern COLL_STRUCT       g_coll_struct;

extern double dclock();

/* functions */
int  MPID_PTLS_init( int *, char ** );
void MPID_PTLS_finalize( void );
void MPID_Node_name( char *, int );
void MPID_PTLS_Wtick( double *tick );

#endif PTLS_PRIV_H
