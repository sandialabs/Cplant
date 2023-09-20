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
** $Id: getInfo.h,v 1.4 2001/11/24 23:32:44 lafisk Exp $
*/

#ifndef GET_INFO
#define GET_INFO

#include <time.h>

#define EXECUTION   (1)
#define ROUTING	    (2)  

typedef struct {		    
    int queue_type;	    /* Execution or routing */
    int total_jobs;	    /* How many jobs */
    int enabled;	    /* Is the queue enabled or not. Default not */
    int started;	    /* Has it been started */
    char *state_count;	    /* I'm not sure what this is for, but it's here */
} queue_info; 	    

typedef struct{
    short rank;       /* process rank */
    short job_id;     /* cplant job id */
    int   nid;        /* physical node id */
    int   u_stat;     /* process status   */
    int   session_id; /* job's PBS job id */
    int   parent_id;  /* cplant job ID of spawning parent */
    int   job_pid;    /* process' system pid  */

    time_t       elapsed;    /* seconds since fork        */
    int   niceKillCountdown; /* non-zero if job is being killed */

    int         priority;    /* 1 - regular, 2 - scavenger */
} job_info;

#define EXITING "E"
#define HELD "H"
#define QUEUED "Q"
#define RUNNING "R"
#define MOVED "T"
#define WAITING_EXEC "W"

typedef struct {    /* Information from qstat */

int job_id;	    /* PBS job ID */
char *username;
char *queue;
char *jobname;
int sessID;	    /* Session ID */
int q_time;	    /* Time in queue */
int req_nodes;	    /* number of nodes requested */
int req_time;    /* Amount of time requested */
char status;	    /* Status of job, i.e. queued, running etc. */
int elapsed_time;   /* Elapsed time */
/* int core; */	    /* amount of 'core' nodes requested */
} qstat_entry;

typedef struct {    /* Server information from qmgr */

char *default_queue;	/* name of the default queue */
int size_avail;		/* resources_available.size */
int size_max;		/* resources_max.size */
int size_assign;	/* resources_assigned.size */

} server_info;

#endif

