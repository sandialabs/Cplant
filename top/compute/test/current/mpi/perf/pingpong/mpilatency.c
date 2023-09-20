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
**  $Id: mpilatency.c,v 1.2 1999/04/20 18:49:50 jotto Exp $
**
**  This is a pingpong test used to calculate 
**  latency and bandwidth for various message 
**  sizes.
**
**  MPI version
*/

#include <stdio.h>
#include "mpi.h"

extern double dclock(void);

#define TRUE            (1)
#define FALSE           (0)

#define SIZE		(10000000)
#define READY		(10)
#define BCAST		(11)
#define LATENCY		(12)
#define DONE		(13)
#define GO_ON		(14)

void doit(int len, double *latency, double *bandwidth);
double aligned_buf[SIZE/sizeof(double)];
char *buf;
int my_node, num_nodes;

int 
main(int argc, char *argv[])
{

extern char *optarg;
extern int optind;
int ch, error;

int len, start_len, end_len, inc, trials, i;
int mega;
double latency, bandwidth;
double tot_latency, tot_bandwidth;
double max_latency, max_bandwidth;
double min_latency, min_bandwidth;
/*
PUMA_Set_send_debug_flag(1);
PUMA_Set_recv_debug_flag(1);
*/
    MPI_Init((int *)&argc,(char ***)&argv);
 
    MPI_Comm_rank( MPI_COMM_WORLD,&my_node );
    MPI_Comm_size( MPI_COMM_WORLD,&num_nodes );

    if (num_nodes < 2)   {
	if (my_node == 0)   {
	    fprintf(stderr, "Need to run on at least two nodes\n");
	}
	exit(-1);
    }

    /* Set the defaults */
    error= FALSE;
    start_len= 0;
    end_len= 1024;
    inc= 16;
    trials= 1000;
    mega= FALSE;
 
    /* check command line args */
    while ((ch= getopt(argc, argv, "i:e:s:n:m")) != EOF)   {
        switch (ch)   {
            case 'i':
		inc= strtol(optarg, (char **)NULL, 0);
		break;
            case 'e':
		end_len= strtol(optarg, (char **)NULL, 0);
		break;
            case 's':
		start_len= strtol(optarg, (char **)NULL, 0);
		break;
            case 'n':
		trials= strtol(optarg, (char **)NULL, 0);
		break;
	    case 'm': 
		mega= TRUE;
		break;
            default:
		error= TRUE;
		break;
        }
    }
 
    if (error)   {
        if (my_node == 0)   {
            fprintf(stderr, "Usage: %s [-s start_length] [-e end_length] [-i inc] [-n trials] [-m]\n", argv[0]);
        }
        exit (-1);
    }



    if (my_node == 0)   {
	printf("\n");
	printf("Results for %d trials each of length %d through %d in increments of %d\n\n", 
	    trials, start_len, end_len, inc);
	printf("Length                  Latency                             Bandwidth\n");
	printf("in bytes            in micro seconds                ");
	if (mega)   {
	    printf("in mega bytes per second\n");
	} else   {
	    printf("in million bytes per second\n");
	}
	printf("            minimum     average     maximum     minimum     average     maximum\n");
    }



    for (len= start_len; len <= end_len; len= len + inc)   {
	buf= (char *)aligned_buf;
	latency= tot_latency= 0.0;
	max_latency= 0.0;
	min_latency= 1000000000.0;
	bandwidth= tot_bandwidth= 0.0;
	max_bandwidth= 0.0;
	min_bandwidth= 1000000000.0;

	for (i= 0; i < trials; i++)   {
	    doit(len, &latency, &bandwidth);
	    tot_latency= tot_latency + latency;
	    if (latency < min_latency)   {
		min_latency= latency;
	    }
	    if (latency > max_latency)   {
		max_latency= latency;
	    }
	    tot_bandwidth= tot_bandwidth + bandwidth;
	    if (bandwidth < min_bandwidth)   {
		min_bandwidth= bandwidth;
	    }
	    if (bandwidth > max_bandwidth)   {
		max_bandwidth= bandwidth;
	    }
	}

	if (my_node == 0)   {
	    printf("%9d  %8.2f    %8.2f    %8.2f    ",
		len, min_latency, tot_latency / trials, max_latency);
	    if (mega)   {
		printf("%8.2f    %8.2f    %8.2f\n", 
		    min_bandwidth / (1024 * 1024),
		    (tot_bandwidth / trials) / (1024 * 1024), 
		    max_bandwidth / (1024 * 1024));
	    } else   {
		printf("%8.2f    %8.2f    %8.2f\n", 
		    min_bandwidth / 1000000.0, 
		    (tot_bandwidth / trials) / 1000000.0, 
		    max_bandwidth / 1000000.0);
	    }
	}
    }

    MPI_Finalize();

}  /* end of main() */


void
doit(int len, double *latency, double *bandwidth)
{

int msgsrc, msglen, msgtype;
int latencysrc, latencylen, latencytype;
MPI_Request flag, latencyflag;
MPI_Status status;
int old, i;
double start, end, delta;
int rc = 0;


    if (my_node == 0)   {
	/* Post receive */
	if (len == 0) {
	    latencylen= 0;
	    latencysrc= 1;
	    latencytype= LATENCY;
	    /*latencyflag= FALSE;*/
	    rc |= MPI_Irecv(buf,latencylen,MPI_BYTE,latencysrc,LATENCY,MPI_COMM_WORLD,&latencyflag);
	    /*_nrecv(NULL, &latencylen, &latencysrc, &latencytype, &latencyflag, NULL);*/
	} else {
	    latencylen= len;
	    latencysrc= 1;
	    latencytype= LATENCY;
	    /*latencyflag = FALSE;*/
	    rc |= MPI_Irecv(buf,latencylen,MPI_BYTE,latencysrc,LATENCY,MPI_COMM_WORLD,&latencyflag);
	    /*_nrecv(buf, &latencylen, &latencysrc, &latencytype, &latencyflag, NULL);*/
	}

	/* Wait for ACK from node 1 */
	msglen= 0;
	msgsrc= 1;
	msgtype= READY;
	rc |= MPI_Recv(buf,msglen,MPI_BYTE,msgsrc,READY,MPI_COMM_WORLD,&status);
	/*_nrecv(NULL, &msglen, &msgsrc, &msgtype, NULL, NULL);*/
	if (len == 0) {
	    start = dclock();
	    rc |= MPI_Rsend(buf,0,MPI_BYTE,1,LATENCY,MPI_COMM_WORLD);
	    /*_nsend(NULL, 0, 1, LATENCY, 0, 0);*/
	} else {
	    start = dclock();
	    rc |= MPI_Rsend(buf,len,MPI_BYTE,1,LATENCY,MPI_COMM_WORLD);
	    /*_nsend(buf, len, 1, LATENCY, 0, 0);*/
	}

        /*
	while ( !latencyflag ) 
	   ; 
	*/
	rc |= MPI_Wait( &latencyflag,&status );
        end = dclock();
	delta = end - start;
	*latency= delta * 1000000.0 / 2.0;
	if ( delta != 0.0 ) {
	  *bandwidth= len / (end - start) * 2.0;
        }
	else {
	  *bandwidth = 0;
	}

	for (msgsrc= 1; msgsrc < num_nodes; msgsrc++)   {
	    rc |= MPI_Send(buf,0,MPI_BYTE,msgsrc,GO_ON,MPI_COMM_WORLD);
	    /*_nsend(NULL, 0, msgsrc, GO_ON, 0, 0);*/
	}

    } else if (my_node == 1)   {
	if (len == 0)   {
	    latencylen= 0;
	    latencysrc= 0;
	    latencytype= LATENCY;
	    /*latencyflag= FALSE;*/
	    rc |= MPI_Irecv(buf,latencylen,MPI_BYTE,latencysrc,LATENCY,MPI_COMM_WORLD,&latencyflag);
	    /*_nrecv(NULL, &latencylen, &latencysrc, &latencytype, &latencyflag, NULL);*/
	} else   {
	    latencylen= len;
	    latencysrc= 0;
	    latencytype= LATENCY;
	    /*latencyflag= FALSE;*/
	    rc |= MPI_Irecv(buf,latencylen,MPI_BYTE,latencysrc,LATENCY,MPI_COMM_WORLD,&latencyflag);
	    /*_nrecv(buf, &latencylen, &latencysrc, &latencytype, &latencyflag, NULL);*/
	}
	rc |= MPI_Send(buf,0,MPI_BYTE,0,READY,MPI_COMM_WORLD);
	/*_nsend(NULL, 0, 0, READY, 0, 0);*/

	/*
	while (!latencyflag)
	    ;
	*/
	rc |= MPI_Wait( &latencyflag,&status );
	rc |= MPI_Get_count(&status, MPI_BYTE, &latencylen);
	latencysrc = status.MPI_SOURCE;
	latencytype = status.MPI_TAG;
	rc |= MPI_Rsend(buf,latencylen,MPI_BYTE,latencysrc,latencytype,MPI_COMM_WORLD);
	/*_nsend(buf, latencylen, latencysrc, latencytype, 0, 0);*/

	msglen= 0;
	msgsrc= 0;
	msgtype= GO_ON;
	rc |= MPI_Recv(buf,msglen,MPI_BYTE,msgsrc,GO_ON,MPI_COMM_WORLD,&status);
	/*_nrecv(NULL, &msglen, &msgsrc, &msgtype, NULL, NULL);*/
    } else   {
	/* We're not involved, just consume the GO_ON message */
	msglen= 0;
	msgsrc= 0;
	msgtype= GO_ON;
	rc |= MPI_Recv(buf,msglen,MPI_BYTE,msgsrc,GO_ON,MPI_COMM_WORLD,&status);
	/*_nrecv(NULL, &msglen, &msgsrc, &msgtype, NULL, NULL);*/
    }
}  /* end of doit() */
