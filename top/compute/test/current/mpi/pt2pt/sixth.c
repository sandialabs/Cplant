/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
 
/*
 *  $Id: sixth.c,v 1.1 1997/10/29 22:45:27 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include "mpi.h"
#ifdef __STDC__
#include <stdlib.h>
#else
extern char *malloc();
#endif

typedef struct _table {
  int references;
  int length;
  int *value;
} Table;

/* These are incorrect...*/
int copy_table (oldcomm, keyval, extra_state, attr_in, attr_out, flag)
MPI_Comm oldcomm;
int      keyval;
void     *extra_state;
void     *attr_in;
void     *attr_out;
int      *flag;
{
  Table *table = (Table *)attr_in;;

  table->references++;
  (*(void **)attr_out) = attr_in;
  (*flag) = 1;
  (*(int *)extra_state)++;
  return (MPI_SUCCESS);
}

void create_table ( num, values, table_out )
int  num;
int *values;
Table **table_out;
{
  int i;
  (*table_out) = (Table *)malloc(sizeof(Table));
  (*table_out)->references = 1;
  (*table_out)->length = num;
  (*table_out)->value = (int *)malloc(sizeof(int)*num);
  for (i=0;i<num;i++) 
    (*table_out)->value[i] = values[i];
}

int delete_table (comm, keyval, attr_val, extra_state)
MPI_Comm comm;
int      keyval;
void     *attr_val;
void     *extra_state;
{
  Table *table = (Table *)attr_val;

  if ( table->references == 1 )
	free(table);
  else
	table->references--;
  (*(int *)extra_state)--;
  return MPI_SUCCESS;
}

int main ( argc, argv )
int argc;
char **argv;
{
  int i, rank, size;
  Table *table;
  MPI_Comm new_comm;
  int table_key;
  int values[3]; 
  int table_copies = 1;
  int found;
  int errors = 0;

  MPI_Init ( &argc, &argv );
  MPI_Comm_rank ( MPI_COMM_WORLD, &rank );
  MPI_Comm_size ( MPI_COMM_WORLD, &size );

  values[0] = 1; values[1] = 2; values[2] = 3;
  create_table(3,values,&table);
  
  MPI_Keyval_create ( copy_table, delete_table, &table_key, 
		      (void *)&table_copies );
  MPI_Attr_put ( MPI_COMM_WORLD, table_key, (void *)table );
  MPI_Comm_dup ( MPI_COMM_WORLD, &new_comm );
  MPI_Attr_get ( new_comm, table_key, (void **)&table, &found );

  if (!found)
	errors++;

  if ((table_copies != 2) && (table->references != 2)) 
    errors++;

  MPI_Comm_free ( &new_comm );

  if ((table_copies != 1) && (table->references != 1)) 
    errors++;

  MPI_Attr_delete ( MPI_COMM_WORLD, table_key );

  if ( table_copies != 0 )
    errors++;
  if (errors)
    printf("[%d] OOPS.  %d errors!\n",rank,errors);

  MPI_Keyval_free ( &table_key );
  Test_Waitforall( );
  MPI_Finalize();
}
