/*
** Definitions for paragon comm.c
*/

typedef struct {
    DATA_TYPE entry;
    DATA_TYPE current;
    int row;
} pivot_type;

void send_msg_old(int me, int dest, char *buf, int bytes, int type);
void recv_msg(int me, int dest, char *buf, int bytes, int type);
void bcast_all(int me, int root, char *buf, int bytes, int type);
void bcast_row(int me, int root, char *buf, int bytes, int type);
void bcast_col(int me, int root, char *buf, int bytes, int type);
void xchg_col_pivot(int me, pivot_type *buf, int bytes, int type);
void sum_row(int me, double *buf, double *temp, int n, int type);
void sum_col(int me, double *buf, double *temp, int n, int type);
double max_all(double buf, int type);

