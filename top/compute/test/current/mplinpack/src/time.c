/*
Copyright (c) 1993 Sandia National Laboratories
 
  Redistribution and use in source and binary forms are permitted
  provided that source distributions retain this entire copyright
  notice.  Neither the name of Sandia National Laboratories nor
  the name of the author may be used to endorse or promote products
  derived from this software without specific prior written permission.
 
Warranty:
  This software is provided "as is" and without any express or
  implied warranties, including, without limitation, the implied
  warranties of merchantability and fitness for a particular
  purpose.

The README file contains addition information
*/

#include <stdio.h>
#ifdef MPI
#include <mpi.h>
#endif
#ifdef OSF
#include <nx.h>
#endif
#include "time.h"

extern double dclock();


double 
seconds(double start)
{
    double time;		/* total seconds */
    time = TIMER();
    time = time - start;

    return (time);
}

/*
** Exchange and calculate average timing information
**
** secs:        number of ticks for this processor
** type:        type of message to collect
*/
double 
timing(double secs, int type)
{

    extern int me;		/* current processor number */
    extern int nprocs_cube;

    double maxtime;
    double avgtime;
    double dtemp;
    int maxproc;
    int itemp;

#ifdef MPI
    struct {
      double val;
      int proc;
    } max_in, max_out;

    max_in.val = secs;
    max_in.proc = me;
    MPI_Allreduce(&max_in,&max_out,1,MPI_DOUBLE_INT,MPI_MAXLOC,MPI_COMM_WORLD);

    MPI_Allreduce(&secs,&avgtime,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
    avgtime /= nprocs_cube;

    if (me == 0) {
	fprintf(stderr, "%.4f (avg), %.4f (max on processor %d).\n",
		avgtime, max_out.val, max_out.proc);
    }
#else
    maxtime = secs;
    gdhigh(&maxtime, 1L, &dtemp);
    if (secs == maxtime) maxproc = me; else maxproc = 0;
    gihigh(&maxproc, 1L, &itemp);
    
    avgtime = secs;
    gdsum(&avgtime, 1L, &dtemp);
    avgtime /= nprocs_cube;

    if (me == 0) {
	fprintf(stderr, "%.4f (avg), %.4f (max on processor %d).\n",
		avgtime, maxtime, maxproc);
    }
#endif
    
    return avgtime;
}

void showtime(char *label, double value)
{

    extern int me;		/* current processor number */
    extern int nprocs_cube;

    double avgtime;

#ifdef MPI
    struct {
      double val;
      int proc;
    } max_in, max_out, min_in, min_out;

    max_in.val = value;
    max_in.proc = me;
    MPI_Allreduce(&max_in,&max_out,1,MPI_DOUBLE_INT,MPI_MAXLOC,MPI_COMM_WORLD);
    min_in.val = value;
    min_in.proc = me;
    MPI_Allreduce(&min_in,&min_out,1,MPI_DOUBLE_INT,MPI_MINLOC,MPI_COMM_WORLD);
   
    MPI_Allreduce(&value,&avgtime,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);

    avgtime /= nprocs_cube;

    if (me == 0) {
	fprintf(stderr, "%s = %.4f (on proc %d), %.4f, %.4f (on proc %d).\n",
		label,min_out.val,min_out.proc,avgtime, max_out.val,max_out.proc);
    }
#else
    double maxtime,mintime;
    double dtemp;
    int maxproc;
    int itemp;

    maxtime = value;
    gdhigh(&maxtime, 1L, &dtemp);
    if (value == maxtime) maxproc = me; else maxproc = 0;
    gihigh(&maxproc, 1L, &itemp);
    mintime = value;
    gdlow(&mintime, 1L, &dtemp);
    
    avgtime = value;
    gdsum(&avgtime, 1L, &dtemp);
    avgtime /= nprocs_cube;

    if (me == 0) {
	fprintf(stderr, "%s = (%.4f, %.4f, %.4f) (max on proc %d).\n",
		label,mintime,avgtime, maxtime, maxproc);
    }
#endif
  }

