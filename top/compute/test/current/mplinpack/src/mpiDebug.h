
/*
** DEBUG_COMM defines - turn on to trace MPI calls
**   define DEBUG_COMM also
**
#define PRINTF     printf
#define PRINTF   if (me == 0) printf
#define DEBUG_SEND
#define DEBUG_RECV
#define DEBUG_WAIT
#define DEBUG_BCAST
#define DEBUG_BCAST
#define DEBUG_WAIT
#define DEBUG_IRECV
*/

/*
** turn on MPI_ERR_CHECK to test MPI error return codes
*/
#ifdef MPI_ERR_CHECK
extern void showMPIerr(int errnum);
static int mrc;
#define TEST_MRC(n) {if (mrc!=MPI_SUCCESS){showMPIerr(mrc); exit(n);}}
#endif

