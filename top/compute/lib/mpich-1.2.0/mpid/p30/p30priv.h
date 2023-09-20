
#ifndef P30_PRIV_H
#define P30_PRIV_H

#define P30_MAX_EVENTS 3

typedef struct eq_handle_list_t {
    struct eq_handle_list_t *next;
    struct eq_handle_list_t *prev;
    ptl_handle_eq_t          eq_handle;
} eq_handle_list_t;

typedef struct mpi_unex_list_t {
    struct mpi_unex_list_t *next;
    struct mpi_unex_list_t *prev;
    ptl_event_t             event;
} mpi_unex_list_t;

extern double dclock();

/***************************************************************************

  Global variables

  ***************************************************************************/

extern unsigned int      _mpi_p30_default_me_size;

extern ptl_process_id_t  _mpi_p30_my_id;
extern ptl_id_t          _mpi_p30_my_size;

extern ptl_handle_ni_t   _mpi_p30_ni_handle;

extern ptl_pt_index_t    _mpi_p30_recv_portal;
extern ptl_pt_index_t    _mpi_p30_read_portal;
extern ptl_pt_index_t    _mpi_p30_ack_portal;

extern ptl_handle_me_t   _mpi_p30_no_match_me_handle;
extern ptl_handle_me_t   _mpi_p30_short_unex_me_handle;
extern ptl_handle_me_t   _mpi_p30_long_me_handle;
extern ptl_handle_me_t   _mpi_p30_ack_me_handle;
extern ptl_handle_me_t   _mpi_p30_read_me_handle;

extern ptl_handle_eq_t   _mpi_p30_unex_eq_handle;

extern void             *_mpi_p30_unex_start;
extern unsigned int      _mpi_p30_short_size; /* short protocol cutoff */

extern eq_handle_list_t *_mpi_p30_eq_list_head;

extern mpi_unex_list_t  *_mpi_p30_unex_list_head;
extern mpi_unex_list_t  *_mpi_p30_unex_list_tail;
extern mpi_unex_list_t  *_mpi_p30_unex_list_free_head;

extern ptl_handle_md_t  _mpi_p30_ack_request_md_handle;

extern ptl_handle_md_t  _mpi_p30_dummy_md_handle;

extern int              _mpi_p30_errno;

extern int              _mpi_p30_drop_count;

/* functions */
int               MPID_P30_init( int *, char ** );
void              MPID_P30_finalize( void );
void              MPID_Node_name( char *, int );
void              MPID_P30_Wtick( double *tick );
eq_handle_list_t *MPID_P30_Get_eq_handle( void );
void              MPID_P30_Free_eq_handle( eq_handle_list_t * );
mpi_unex_list_t  *MPID_P30_Get_unex( void );
void              MPID_P30_Free_unex( mpi_unex_list_t * );
void              MPID_P30_Check_for_drop( void );

#endif /* P30_PRIV_H */
