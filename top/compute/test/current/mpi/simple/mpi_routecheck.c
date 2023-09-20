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
/*****************************************************************************
 $Id: mpi_routecheck.c,v 1.1 2002/03/07 19:00:53 jbogden Exp $
 
 name:		mpi_routecheck.c

 purpose:	Performs a simple (brute force?) all to all message passing
            pattern between all nodes. It's primary purpose is to 
            test the route and links between all nodes.

 author: 	d. w. doerfler

 parameters:	

 returns:	

 comments:	

 revisions:	

*****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <mpi.h>
#undef MCPCHECK
#ifdef MCPCHECK
#include <mcpget.h>
#endif

#define MIN_DATA_XFER   8
#define MAX_DATA_XFER   8192
#define MAX_BURST       16

int data_size[MAX_BURST];
MPI_Status status[MAX_BURST];
MPI_Request request[MAX_BURST];
int mpi_size, my_rank, dealer_node, dest_node;
static void *msg = NULL;
int burst_size = 1;
int burst_index = 0;
long min_data_xfer = MIN_DATA_XFER;
long max_data_xfer = MAX_DATA_XFER;
long interval = 0;
int verbose = 1;

/* getopt stuff */
extern char* optarg;
static struct option opts[] = {
    {"verbose",     no_argument,        0,  'v'},
    {"help",        no_argument,        0,  'h'},
    {"usage",       no_argument,        0,  'U'},
    {"interval",    required_argument,  0,  'i'}, /* loop time in seconds */
    {"smin",        required_argument,  0,  's'}, /* min message size */
    {"emax",        required_argument,  0,  'e'}, /* max message size */
    {"msgsize",     required_argument,  0,  'm'}, /* min == max msg size */
    {"burst",       required_argument,  0,  'b'}, /* burst mode and burst size */
    {0, 0, 0, 0}
};


void usage(char *name) {
    if (my_rank == 0) {
        fprintf(stderr, "Usage: %s \n\
  -v verbose\n\
  -interval <loop time in seconds>  default is disabled\n\
  -smin <min_msg_size>  default = %d bytes\n\
  -emax <max_msg_size>  default = %d bytes\n\
  -m msg_size  min == max\n\
  -burst <size>  number of sends to do at once (sort of)\n",
        name, MIN_DATA_XFER, MAX_DATA_XFER);
  
        MPI_Finalize();
        exit(0);
    }
}


int main(int argc, char **argv)
{
    double start;
    double delta;
    int ch = 0;
    int index = 0;
      
#ifdef MCPCHECK
    mcpshmem_t *mcp_shmem, mcp_state1, mcp_state2;
    int type;
#endif

#ifdef MCPCHECK
    if (!mcpopen(&mcp_shmem, &type))
    {
      printf("Unable to open MCP\n");
      exit(1);
    }
#endif

    /**************************************************************
      Initialize transport method
    **************************************************************/
    if ( MPI_Init( &argc, &argv ) != MPI_SUCCESS )
    {
        printf("Unable to initialize MPI\n");
        exit(1);
    }

    MPI_Comm_size( MPI_COMM_WORLD, &mpi_size );
    MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );
    MPI_Barrier( MPI_COMM_WORLD );

    if ( mpi_size == 1 ) {
        printf("This program requires more than 1 process\n");
        MPI_Finalize();
        exit(1);
    }

    if (my_rank == 0  &&  verbose >= 2)
    {
        printf("sizeof(MPI_Status) = %d bytes\n",sizeof(MPI_Status));
        printf("sizeof(MPI_Request) = %d bytes\n",sizeof(MPI_Request));
    }

    /**************************************************************
      Parse the Command Args
    **************************************************************/
    while ((ch = getopt_long_only(argc, argv, "", opts, &index)) != EOF) {
        switch (ch) {
            case 'v':
                verbose++;
                break;
            case 'h':
            case 'U':
                usage(argv[0]);
                break;
            case 'i':
                interval = strtol(optarg, (char**)NULL, 10);
                break;
            case 'b':
                burst_size = strtol(optarg, (char**)NULL, 10);
                if (my_rank == 0)
                    printf("BURST MODE: size = %d\n",burst_size);
                break;
            case 's':
                min_data_xfer = strtol(optarg, (char**)NULL, 10);
                break;
            case 'e':
                max_data_xfer = strtol(optarg, (char**)NULL, 10);
                break;
            case 'm':
                min_data_xfer = max_data_xfer = strtol(optarg, (char**)NULL, 10);
                break;
            default:
                usage(argv[0]);
        }
    }
    
    /**************************************************************
      Allocate message
    **************************************************************/
    if (msg == NULL)
    {
        if ((msg = malloc(max_data_xfer)) == NULL)
        {
            printf("Unable to allocate %d bytes of memory\n",max_data_xfer);
            exit(1);
        }
    }

    /**************************************************************
      Announce parameters
    **************************************************************/
    if (my_rank == 0)
    {
        printf("Timing resolution is %f seconds\n", (float)MPI_Wtick());
        if (min_data_xfer == max_data_xfer)
          printf("Message size = %d bytes\n", min_data_xfer);
        else
          printf("Min msg = %d bytes, Max = %d bytes\n", 
          min_data_xfer, max_data_xfer);

        fflush(stdout);
    }

    /**************************************************************
      Loop through using every node as a dealer node
    **************************************************************/
    start = MPI_Wtime();
    do 
    {
    for (dealer_node = 0; dealer_node < mpi_size; dealer_node++)
    {
      if (my_rank == dealer_node)
      {
        if (verbose)
        {
          printf("dealer node is now rank %d\n",dealer_node);            
          fflush(stdout);
        }

        for (dest_node = 0; dest_node < mpi_size; ++dest_node)
        {
          if (dest_node == dealer_node) continue;

#ifdef MCPCHECK
          mcpget(mcp_shmem, &mcp_state1, &type);
#endif

          /**************************************************************
            Loop through different data sizes
          **************************************************************/
          data_size[0] = min_data_xfer;
          do
          {
            for (burst_index=0; burst_index < burst_size; burst_index++)
            {
              if (MPI_Irecv( msg,
                             max_data_xfer,
                             MPI_BYTE,
                             dest_node,
                             1,
                             MPI_COMM_WORLD,
                             &request[burst_index] ) != MPI_SUCCESS )
              {
                printf("Error posting return message\n");
                break;
              }
            }

            if (verbose >= 2)
              printf("Sending to node %d of size %d\n", dest_node, data_size[0]);

            for (burst_index=0; burst_index < burst_size; burst_index++)
            {
              if ( MPI_Send( msg,
	                     data_size[0],
	                     MPI_BYTE,
	                     dest_node,
	                     1,
	                     MPI_COMM_WORLD ) != MPI_SUCCESS )
              {
                printf("Unable to send message\n");
                break;
              }
            }

            for (burst_index=0; burst_index < burst_size; burst_index++)
            {
              MPI_Wait(&request[burst_index],&status[burst_index]);
            }

            if (verbose >= 2)
              printf("Received confirmation from node %d\n", dest_node);

            data_size[0] *= 2;
          } while( data_size[0] <= max_data_xfer );
#ifdef MCPCHECK
          mcpget(mcp_shmem, &mcp_state2, &type);
          {
            char buf[80];
            sprintf(buf, "snd: ranks %d <-> %d", my_rank, dest_node);
            mcpcmp(&mcp_state1, &mcp_state2, &type, buf);
	    fflush(stdout);
          }
#endif
        }

        /* send kill message to slave node */
        for (dest_node = 0; dest_node < mpi_size; ++dest_node)
        {
          if (dest_node == dealer_node) continue;

          if ( MPI_Send( NULL,
                         0,
	                 MPI_BYTE,
		         dest_node,
		         1,
		         MPI_COMM_WORLD ) != MPI_SUCCESS )
          {
            printf("Unable to send KILL message to node %d\n", dest_node);
          }
        }

        if (verbose >= 2)
          printf("dealer node %d done\n", my_rank);

      } /* end of dealer node */

      /**************************************************************
        Slave node
      **************************************************************/
      else
      {
        if (verbose >= 2)
          printf("I am node %d and I am a slave\n", my_rank);

#ifdef MCPCHECK
        mcpget(mcp_shmem, &mcp_state1, &type);
#endif



        /* Process all the incoming burst data */
        memset(data_size,0,sizeof(data_size));
        for (burst_index=0; burst_index < burst_size; burst_index++)
        {

          /* receive data */
          if (MPI_Irecv( msg,
                         max_data_xfer,
                         MPI_BYTE,
                         MPI_ANY_SOURCE,
                         1,
                         MPI_COMM_WORLD,
                         &request[burst_index]) != MPI_SUCCESS )
          {
             printf("Error receiving message data\n");
             break;
          }
        }

        if (verbose >= 2)
          printf("Node %d posted all burst receives\n",my_rank);

        for (burst_index=0; burst_index < burst_size; burst_index++)
        {

          MPI_Wait(&request[burst_index], &status[burst_index]);
          MPI_Get_count(&status[burst_index], MPI_BYTE, &data_size[burst_index]);

          /* send data back */
          if (data_size[burst_index] != 0)
          {
            if ( MPI_Send( msg,
                           data_size[burst_index],
	                   MPI_BYTE,
	                   status[burst_index].MPI_SOURCE,
	                   1,
	                   MPI_COMM_WORLD ) != MPI_SUCCESS )
            {
              printf("Unable to send return data\n");
            }
          }
        } 

        if (verbose >= 2)
          printf("Node %d finished receiving burst data\n",my_rank);

        /* Process any remaining incoming packets including the kill packet */
        do 
        {
          memset(data_size,0,sizeof(data_size));
          burst_index = 0;

          /* receive data */
          if (MPI_Irecv( msg,
                         max_data_xfer,
                         MPI_BYTE,
                         MPI_ANY_SOURCE,
                         1,
                         MPI_COMM_WORLD,
                         &request[burst_index]) != MPI_SUCCESS )
          {
             printf("Error receiving message data\n");
             break;
          }

          MPI_Wait(&request[burst_index], &status[burst_index]);
          MPI_Get_count(&status[burst_index], MPI_BYTE, &data_size[burst_index]);

          /* send data back */
          if (data_size[burst_index] != 0)
          {
            if ( MPI_Send( msg,
                           data_size[burst_index],
	                   MPI_BYTE,
	                   status[burst_index].MPI_SOURCE,
	                   1,
	                   MPI_COMM_WORLD ) != MPI_SUCCESS )
            {
              printf("Unable to send return data\n");
            }
          }

        } while (data_size[burst_index] != 0);

#ifdef MCPCHECK
	    mcpget(mcp_shmem, &mcp_state2, &type);
	    {
	      char buf[80];
	      sprintf(buf, "rcv: ranks %d <-> %d", status.MPI_SOURCE, my_rank);
	      mcpcmp(&mcp_state1, &mcp_state2, &type, buf);
	      fflush(stdout);
	    }
#endif

        if (verbose >= 2)
          printf("Slave node %d done\n", my_rank);

      } /* end of slave node */

      if (my_rank == 0)
      {
        if (interval)
        {
          if ((MPI_Wtime() - start) > (double)interval)
	  {
	    printf("Aborting due to time interval expiration\n");
	    MPI_Abort(MPI_COMM_WORLD, 0);
          }
        }
      }

    } /* end of looping through dealer nodes */

    } while (interval);

    delta = MPI_Wtime() - start;
    if (my_rank == 0)
    {
      printf("Total time = %lf\n", delta);
      printf("Avg loop time = %lf\n", delta / mpi_size);
    }

    if (verbose >= 2)
      printf("Node %d calling finalize\n", my_rank);

    MPI_Finalize();
    free (msg);
    exit(0);
}
