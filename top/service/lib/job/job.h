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
** $Id: job.h,v 1.3 2001/11/17 01:00:07 lafisk Exp $
**
** The public definitions to be included in codes that use libjob.a
*/

#ifndef JOBH
#define JOBH

/*
** debugging info
*/
#include "../../util/yod/include/util.h"
#include "srvr_comm.h"

extern int Dbgflag;

#define job_debug_io()           (Dbgflag |= DBG_IO_1)
#define job_debug_io_detail()    (Dbgflag |= (DBG_IO_1 | DBG_IO_2))
#define job_debug_job_spawn()    (Dbgflag |= (DBG_JOBS))
#define job_debug_load()         (Dbgflag |= DBG_LOAD_1)
#define job_debug_load_detail()  (Dbgflag |= (DBG_LOAD_1 | DBG_LOAD_2))
#define job_debug_app_progress() (Dbgflag |= DBG_PROGRESS)
#define job_debug_app_failure()  (Dbgflag |= DBG_FAILURE)
#define job_debug_app_debug()    (Dbgflag |= DBG_DBG)
#define job_debug_allocation()   (Dbgflag |= DBG_ALLOC)
#define job_debug_heterogeneous_load()  (Dbgflag |= DBG_HETERO)
#define job_debug_bebopd()              (Dbgflag |= DBG_BEBOPD)
#define job_debug_communications()      (Dbgflag |= DBG_COMM)
#define job_debug_pbs()                 (Dbgflag |= DBG_PBS)
#define job_debug_forrtl()              (Dbgflag |= DBG_FORRTL)
#define job_debug_yod_environment()     (Dbgflag |= DBG_ENVIRON)
#define job_debug_heap()                (Dbgflag |= DBG_MEMORY)

char *job_stack_current();
void job_stack_display();
char *job_strerror();
char *displayJobStatus(int status);

#define MAXDEPTH 10 
extern char *depthmarker[MAXDEPTH];


/*
** options
*/
typedef struct _jobOpt{
int quiet;
int show_link_versions;
int altBnid;
int altBpid;
int get_bt;
int log_startup_actions;
int pauseForDebugger;
int display_allocation;
int attach_gdb;
char *done_file;
int interactive;
int timing_data;
int bypass_link_version_check;
int autokill;
int DebugSpecial;
int retryCount;
char *straceDirectory;
char *straceOptions;
char *straceNodeList;
char *myName;
int IdoIO;
}jobOpt;

extern jobOpt jobOptions;

/*
** query functions
*/
int jobInfo_PBSjobID();
int jobInfo_PBSnumNodes();
int jobInfo_jobID(int handle);
int jobInfo_numNodes(int handle);
int jobInfo_jobState(int handle);
int *jobInfo_appPpidList(int handle);
int *jobInfo_nodeList(int handle);
int jobInfo_retryLoad(int handle);
char *jobInfo_fileName(int handle);
int jobInfo_parentID(int handle);


void jobPrintNodeRequest(int handle);
void jobPrintNodesAllocated(int handle);
void jobPrintCompletionReport(int handle, FILE *fp);


/*
** processing user requests
*/
char *job_find_in_cwd(char *fname);
char *job_find_in_path(char *fname);

#include "cplant.h"    /* for job statuses */

int job_request(int size, int nproc, char *list, int nmembers, int *sizes,
	 int *nprocs, char **lists, char **pnames, char ***pargs, int myparent);
int job_allocate(int handle, int priority);
int job_load(int handle, int blocking);
void job_load_cancel(int handle);
void job_free(int jobHandle);

/*
** servicing running jobs
*/
int job_get_pct_message(int jobHandle,
                     int *mtype, int *nid, int *rank,
                     control_msg_handle *mh);

int job_free_pct_message(int jobHandle, control_msg_handle *mh);


int job_get_app_message(int jobHandle,
                     int *mtype, int *nid, int *rank,
                     control_msg_handle *mh);
int job_free_app_message(int jobHandle, control_msg_handle *mh);

int job_send_signal(int handle, int sig);
int job_nodes_reset(int handle); 


#endif
