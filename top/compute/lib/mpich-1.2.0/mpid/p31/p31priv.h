
#ifndef P31_PRIV_H
#define P31_PRIV_H

#define P31_MAX_EVENTS      3

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

typedef struct {
    void                   *start;
    ptl_handle_me_t         me_handle;
    ptl_handle_md_t         md_handle;
    int                     bytes_copied;
} mpi_unex_block;

extern double dclock();

/***************************************************************************

  Global variables

  ***************************************************************************/

extern unsigned int      _mpi_p31_default_me_size;

extern ptl_process_id_t  _mpi_p31_my_id;
extern ptl_id_t          _mpi_p31_my_size;

extern ptl_handle_ni_t   _mpi_p31_ni_handle;

extern ptl_pt_index_t    _mpi_p31_recv_portal;
extern ptl_pt_index_t    _mpi_p31_read_portal;
extern ptl_pt_index_t    _mpi_p31_ack_portal;

extern ptl_handle_me_t   _mpi_p31_no_match_me_handle;
extern mpi_unex_block    _mpi_p31_short_unex_block[];
extern ptl_handle_me_t   _mpi_p31_long_me_handle;
extern ptl_handle_me_t   _mpi_p31_ack_me_handle;
extern ptl_handle_me_t   _mpi_p31_read_me_handle;

extern ptl_handle_eq_t   _mpi_p31_unex_eq_handle;

extern unsigned int      _mpi_p31_short_size; /* short protocol cutoff */

extern eq_handle_list_t *_mpi_p31_eq_list_head;

extern mpi_unex_list_t  *_mpi_p31_unex_list_head;
extern mpi_unex_list_t  *_mpi_p31_unex_list_tail;
extern mpi_unex_list_t  *_mpi_p31_unex_list_free_head;

extern ptl_handle_md_t  _mpi_p31_ack_request_md_handle;

extern ptl_handle_me_t  _mpi_p31_dummy_me_handle;
extern ptl_handle_md_t  _mpi_p31_dummy_md_handle;

extern int              _mpi_p31_errno;

extern int              _mpi_p31_drop_count;

extern int              _mpi_unex_block_size;

/* functions */
int               MPID_P31_init( int *, char ** );
void              MPID_P31_finalize( void );
void              MPID_Node_name( char *, int );
void              MPID_P31_Wtick( double *tick );
eq_handle_list_t *MPID_P31_Get_eq_handle( void );
void              MPID_P31_Free_eq_handle( eq_handle_list_t * );
mpi_unex_list_t  *MPID_P31_Get_unex( void );
void              MPID_P31_Free_unex( mpi_unex_list_t * );

#endif /* P31_PRIV_H */
