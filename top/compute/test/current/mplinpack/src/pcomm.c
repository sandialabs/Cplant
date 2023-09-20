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

#include <math.h>
#include <stdio.h>
#ifdef MPI
#include <mpi.h>
#endif
#ifdef OSF
#   include <nx.h>
#endif
#include "../include/defines.h"
#include "BLAS_prototypes.h"
#include "macros.h"

#define DEBUG1 0   
/*  define variables to avoid compiler error    */
BLAS_INT one = 1;
double d_one = 1.;

int ringnext,ringprev,hbit,rmbit,my_col_id,my_row_id;
int ringnex2,ringpre2,ringnex3,ringpre3,ringnex4,ringpre4;
typedef struct {
  DATA_TYPE entry;
  DATA_TYPE current;
  int row;
} pivot_type;

void initcomm(){
  extern int nprocs_col, nprocs_row, me, hbit, my_col_id, my_row_id, rmbit;
  extern int ringnext,ringprev,ringnex2,ringpre2,ringnex3,ringpre3,ringnex4,ringpre4;
  int col_id,hbit_ind,bit;

  my_col_id = mesh_col(me);
  my_row_id = mesh_row(me);

  
  col_id = my_col_id + 1;
  if (col_id >= nprocs_row) col_id = 0;
  ringnext = proc_num(my_row_id,col_id);

  col_id = my_col_id + 2;
  if (col_id >= nprocs_row) col_id -= nprocs_row;
  ringnex2 = proc_num(my_row_id,col_id);

  col_id = my_col_id + 3;
  if (col_id >= nprocs_row) col_id -= nprocs_row;
  ringnex3 = proc_num(my_row_id,col_id);

  col_id = my_col_id + 4;
  if (col_id >= nprocs_row) col_id -= nprocs_row;
  ringnex4 = proc_num(my_row_id,col_id);

  col_id = my_col_id - 1;
  if (col_id < 0) col_id = nprocs_row - 1;
  ringprev = proc_num(my_row_id,col_id);

  col_id = my_col_id - 2;
  if (col_id < 0) col_id += nprocs_row;
  ringpre2 = proc_num(my_row_id,col_id);

  col_id = my_col_id - 3;
  if (col_id < 0) col_id += nprocs_row;
  ringpre3 = proc_num(my_row_id,col_id);

  col_id = my_col_id - 4;
  if (col_id < 0) col_id += nprocs_row;
  ringpre4 = proc_num(my_row_id,col_id);

  /* calculate first power of two bigger or equal to the number of rows,
     and low order one bit in own name*/
  for (hbit = 1; nprocs_col > hbit ; hbit = hbit << 1);

  rmbit = 0;
  for (bit = 1; bit < hbit; bit = bit << 1) {
    if ((my_row_id & bit) == bit) {
      rmbit = bit; break;}
  }

#if (DEBUG1 > 0)
  printf("In initcomm, node %d: my_col_id = %d, my_row_id = %d, hbit = %d, rmbit = %d, ringnext = %d, ringprev = %d\n",me,my_col_id,my_row_id,hbit,rmbit,ringnext,ringprev);
#endif
}

void recv_msg(int me, int dest, char *buf, int bytes, int type) {

  int rcr, wanted;
# ifdef MPI
  MPI_Status recv_stat;
# endif

  if (me != dest) {
#   ifdef MPI
      MPI_Recv(buf, bytes, MPI_CHAR, MPI_ANY_SOURCE, type, MPI_COMM_WORLD, &recv_stat);
#   endif
#   ifdef COUGAR
      printf( " me %d waiting for type %d of length %d \n", me,type,bytes);
      crecv(type, buf, bytes);
      printf( " me %d received type %d of length %d \n", me,type,bytes);
#   endif
/*  #   ifdef SUNMOS
      wanted = bytes;
      rcr = _nrecv(buf, &bytes, &dest, &type, NULL, NULL);
      if (rcr != wanted) {
        fprintf(stderr,"Incomplete recv of type %d in lu, \
node %d wanted %d bytes from node %d and recvd only %d bytes\n"
,type,me,wanted,dest,rcr);
      }
#   endif   */
    }
}


void send_msg(int me, int dest, char *buf, int bytes, int type) {

  int rcw;

  if (me != dest) {
#   ifdef MPI
      MPI_Send(buf, bytes, MPI_CHAR, dest, type, MPI_COMM_WORLD);
#   endif
#   ifdef COUGAR
      csend(type, buf, bytes, dest, 0);
#   endif
#   ifdef SUNMOS
      rcw= _nsend(buf, bytes, dest, type, NULL, 0);
      if (rcw != 0){
          fprintf(stderr,"Unsuccessful send in lu, from = %d, to %d, \
                  type = %d\n",me,dest,type);
      }
#   endif
    }
}


#ifndef MPI


void bcast_all(int me, int root, char *buf, int bytes, int type){
  extern int nprocs_cube;
  int dest;

  dest = me - 1;
  if (dest < 0) dest = nprocs_cube-1;
  if (me != root) {
    recv_msg(me, dest, buf, bytes, type);
  }

  dest = me + 1;
  if (dest >= nprocs_cube) dest = 0;
  if (dest != root) {
    send_msg(me, dest, buf, bytes, type);
  }
}


void bcast_row(int me, int root, char *buf, int bytes, int type){
  extern int nprocs_col, nprocs_row,ringprev,ringnext;
  if (me != root) {
    recv_msg(me, ringprev, buf, bytes, type);
  }
  if (ringnext != root) {
    mac_send_msg(ringnext, buf, bytes, type);
  }
}

void bcast_col(int me, int root, char *buf, int bytes, int type){
  extern int nprocs_col, nprocs_row, hbit, my_row_id, my_col_id;
  int dest, dest_row, root_row;
  int mask;

  root_row = mesh_row(root);

  if (hbit == nprocs_col) {	/* power of two nodes in column use simple version */
    for (mask=1; mask < hbit; mask = mask<<1) 
      if ((my_row_id & mask) != (root_row & mask)) break;

    if (my_row_id != root_row) {
      dest_row = my_row_id ^ mask;
      dest = proc_num(dest_row, my_col_id);
      recv_msg(me, dest, buf, bytes, type);
    }

    for (mask = mask >> 1; mask != 0; mask = mask >> 1) {
      dest_row = my_row_id ^ mask;
      dest = proc_num(dest_row, my_col_id);
      mac_send_msg(dest, buf, bytes, type);
    }
  }else{
    /* if the root is in the upper half, move root to lower half */
    if ((root_row & (hbit >> 1)) != 0) {
      root_row = root_row ^ (hbit >> 1);
      if (me == root) {
	dest = proc_num(root_row, my_col_id);
	mac_send_msg(dest, buf, bytes, type);
      }
      if (my_row_id == root_row) {
	dest_row = mesh_row(root);
	dest = proc_num(dest_row, my_col_id);
	recv_msg(me, dest, buf, bytes, type);
      }
    }

    /* now do a fanout on lower half */

    if (my_row_id < (hbit >> 1)) {
      for (mask=1; mask < (hbit >> 1); mask = mask<<1)
	if ((my_row_id & mask) != (root_row & mask)) break;

      if (my_row_id != root_row) {
	dest_row = my_row_id ^ mask;
	dest = proc_num(dest_row, my_col_id);
	recv_msg(me, dest, buf, bytes, type);
      }

      for (mask = mask >> 1; mask != 0; mask = mask >> 1) {
	dest_row = my_row_id ^ mask;
	dest = proc_num(dest_row, my_col_id);
	mac_send_msg(dest, buf, bytes, type);
      }
    }

    /* send info from lower half to upper half */

    dest_row = my_row_id ^ (hbit >> 1);
    dest = proc_num(dest_row, my_col_id);
    if ((my_row_id & (hbit >>1)) == 0) {
      if (dest_row < nprocs_col) {
	mac_send_msg(dest, buf, bytes, type);
      }
    } else {
      recv_msg(me, dest, buf, bytes, type);
    }
  }
}


/* each processor begins with a candidate pivot and its row.
   The processor with the current row supplies this as well and others give 0.
   At completion each knows the max. value of candidate pivots and its row and the 
   value of the current row.
   */

void xchg_col_pivot(int me, pivot_type *buf, int bytes, int type){
  extern int nprocs_row, nprocs_col, hbit, my_row_id, my_col_id;
  static pivot_type temp;

  int mask;
  int dest, dest_row;


  if (hbit == nprocs_col) {	/* power of two nodes in column use simple version */
    /* do a binary exchange */
    for (mask = 1; mask < hbit; mask = mask << 1) {
      printf(" %d for mask %d  bit = %d \n",me,mask,hbit);
      dest_row = my_row_id ^ mask;
      dest = proc_num(dest_row, my_col_id);
      printf(" %d sending to %d a message of length %d \n",me,dest,bytes);
      /* mac_send_msg(dest, (char *) buf, bytes, type);   */
      csend(type,(char *) buf,bytes,dest,0);
      printf(" %d finished sending to %d a message of length %d \n",me,dest,bytes);
      recv_msg(me, dest, (char *) &temp, bytes, type);
       printf(" %d finished receiving in xchg for mask = \n",me,mask);
#ifdef COMPLEX
      if ((fabs((*buf).entry.r) + fabs((*buf).entry.i)) < 
          (fabs(temp.entry.r) + fabs(temp.entry.i))) {
	(*buf).entry = temp.entry;
	(*buf).row = temp.row;
      }
      else if (((fabs((*buf).entry.r) + fabs((*buf).entry.i)) == 
               (fabs(temp.entry.r) + fabs(temp.entry.i))) && (me < dest)){
	(*buf).entry = temp.entry;
	(*buf).row = temp.row;
      }
      if ((temp.current.r != 0.0) || (temp.current.i != 0.0))
        (*buf).current = temp.current;
#else
      if (fabs((*buf).entry) < fabs(temp.entry)){
	(*buf).entry = temp.entry;
	(*buf).row = temp.row;
      }
      else if ((fabs((*buf).entry) == fabs(temp.entry)) && (me < dest)){
	(*buf).entry = temp.entry;
	(*buf).row = temp.row;
      }
      if (temp.current != 0.0) (*buf).current = temp.current;
#endif
    }
  }
  else {
    /* upper half of column processors send their info to the lower half */
    dest_row = my_row_id ^ (hbit >> 1);
    dest = proc_num(dest_row, my_col_id);
    if ((my_row_id & (hbit >> 1)) == 0) {
      if (dest_row < nprocs_col) {
	recv_msg(me, dest, (char *) &temp, bytes, type);
#ifdef COMPLEX
        if ((fabs((*buf).entry.r) + fabs((*buf).entry.i)) < 
            (fabs(temp.entry.r) + fabs(temp.entry.i))) {
	  (*buf).entry = temp.entry;
  	(*buf).row = temp.row;
        }
        else if (((fabs((*buf).entry.r) + fabs((*buf).entry.i)) == 
                 (fabs(temp.entry.r) + fabs(temp.entry.i))) && (me < dest)){
          (*buf).entry = temp.entry;
          (*buf).row = temp.row;
        }
        if ((temp.current.r != 0.0) || (temp.current.i != 0.0))
          (*buf).current = temp.current;
#else
	if (fabs((*buf).entry) < fabs(temp.entry)){
	  (*buf).entry = temp.entry;
	  (*buf).row = temp.row;
	}
	else if ((fabs((*buf).entry) == fabs(temp.entry)) && (me < dest)){
	  (*buf).entry = temp.entry;
	  (*buf).row = temp.row;
	}
	if (temp.current != 0.0) (*buf).current = temp.current;
#endif
      }
    } else {
      mac_send_msg(dest, (char *) buf, bytes, type);
    }

    /* now do a binary exchange on lower half */

    if ((my_row_id & (hbit >> 1)) == 0) {
      for (mask = 1; mask < (hbit >> 1); mask = mask << 1) {
	dest_row = my_row_id ^ mask;
	if (dest_row < nprocs_col) {
	  dest = proc_num(dest_row, my_col_id);
	  mac_send_msg(dest, (char *) buf, bytes, type);
	  recv_msg(me, dest, (char *) &temp, bytes, type);
#ifdef COMPLEX
          if ((fabs((*buf).entry.r) + fabs((*buf).entry.i)) < 
              (fabs(temp.entry.r) + fabs(temp.entry.i))) {
	    (*buf).entry = temp.entry;
  	  (*buf).row = temp.row;
          }
          else if (((fabs((*buf).entry.r) + fabs((*buf).entry.i)) == 
                   (fabs(temp.entry.r) + fabs(temp.entry.i))) && (me < dest)){
            (*buf).entry = temp.entry;
            (*buf).row = temp.row;
          }
          if ((temp.current.r != 0.0) || (temp.current.i != 0.0))
            (*buf).current = temp.current;
#else
	  if (fabs((*buf).entry) < fabs(temp.entry)){
	    (*buf).entry = temp.entry;
	    (*buf).row = temp.row;
	  }
	  else if ((fabs((*buf).entry) == fabs(temp.entry)) && (me < dest)){
	    (*buf).entry = temp.entry;
	    (*buf).row = temp.row;
	  }
	  if (temp.current != 0.0) (*buf).current = temp.current;
#endif
	}
      }
    }

    /* send info from lower half to upper half */

    dest_row = my_row_id ^ (hbit >> 1);
    dest = proc_num(dest_row, my_col_id);
    if ((my_row_id & (hbit >> 1)) == 0) {
      if (dest_row < nprocs_col) {
	mac_send_msg(dest, (char *) buf, bytes, type);
      }
    } else {
      recv_msg(me, dest, (char *) buf, bytes, type);
    }
  }
}

void sum_row(int me, double *buf, double *temp, int n, int type) {
  extern int nprocs_row;
  extern int nprocs_col;
  
  int hbit;
  int mask;
  int bytes;
  int row_id, col_id;
  int dest, dest_col;

  col_id = mesh_col(me);
  row_id = mesh_row(me);

  bytes = n*sizeof(double);

  /* upper half of column processors send their info to the lower half */

  for (hbit = 0; (nprocs_row >> hbit) != 1; hbit++);
  dest_col = col_id ^ (1<<hbit);
  dest = proc_num(row_id, dest_col);
  if ((col_id & (1<<hbit)) == 0) {
    if (dest_col < nprocs_row) {
      recv_msg(me, dest, (char *) temp, bytes, type);
      daxpy(n, d_one, temp, one, buf, one);
    }
  } else {
    send_msg(me, dest, (char *) buf, bytes, type);
  }

  /* now do a binary exchange on lower half */

  if ((col_id & (1<<hbit)) == 0) {
    for (mask = 1; mask < (1<<hbit); mask = mask << 1) {
      dest_col = col_id ^ mask;
      if (dest_col < nprocs_row) {
        dest = proc_num(row_id, dest_col);
        send_msg(me, dest, (char *) buf, bytes, type);
        recv_msg(me, dest, (char *) temp, bytes, type);
        daxpy(n, d_one, temp, one, buf, one);
      }
    }
  }

  /* send info from lower half to upper half */

  dest_col = col_id ^ (1<<hbit);
  dest = proc_num(row_id, dest_col);
  if ((col_id & (1<<hbit)) == 0) {
    if (dest_col < nprocs_row) {
      send_msg(me, dest, (char *) buf, bytes, type);
    }
  } else {
    recv_msg(me, dest, (char *) buf, bytes, type);
  }
}

void sum_col(int me, double *buf, double *temp, int n, int type) {
  extern int nprocs_row;
  extern int nprocs_col;
  
  int hbit;
  int mask;
  int bytes;
  int row_id, col_id;
  int dest, dest_row;

  col_id = mesh_col(me);
  row_id = mesh_row(me);

  bytes = n*sizeof(double);

  /* upper half of column processors send their info to the lower half */

  for (hbit = 0; (nprocs_col >> hbit) != 1; hbit++);
  dest_row = row_id ^ (1<<hbit);
  dest = proc_num(dest_row, col_id);
  if ((row_id & (1<<hbit)) == 0) {
    if (dest_row < nprocs_col) {
      recv_msg(me, dest, (char *) temp, bytes, type);
      daxpy(n, d_one, temp, one, buf, one);
    }
  } else {
    send_msg(me, dest, (char *) buf, bytes, type);
  }

  /* now do a binary exchange on lower half */

  if ((row_id & (1<<hbit)) == 0) {
    for (mask = 1; mask < (1<<hbit); mask = mask << 1) {
      dest_row = row_id ^ mask;
      if (dest_row < nprocs_col) {
        dest = proc_num(dest_row, col_id);
        send_msg(me, dest, (char *) buf, bytes, type);
        recv_msg(me, dest, (char *) temp, bytes, type);
        daxpy(n, d_one, temp,  one, buf, one);
      }
    }
  }

  /* send info from lower half to upper half */

  dest_row = row_id ^ (1<<hbit);
  dest = proc_num(dest_row, col_id);
  if ((row_id & (1<<hbit)) == 0) {
    if (dest_row < nprocs_col) {
      send_msg(me, dest, (char *) buf, bytes, type);
    }
  } else {
    recv_msg(me, dest, (char *) buf, bytes, type);
  }
}

#endif
/* code above here not needed with MPI */

double 
max_all(double buf, int type)
{

    double maxval;
    double dtemp;

    maxval = buf;

#   ifdef MPI
    MPI_Allreduce(&buf,&maxval,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);
#   else
    gdhigh(&maxval, 1L, &dtemp);
#   endif

    return maxval;
}

