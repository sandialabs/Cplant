#ifdef CPLANT
#include </usr/include/time.h>
#endif

double	seconds(double start);
double	timing(double secs, int type);

#ifdef MPI
#define TIMER MPI_Wtime
#else
extern double dclock();
#define TIMER dclock
#endif
