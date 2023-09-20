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
** $Id: job_query.c,v 1.2 2001/11/17 01:00:07 lafisk Exp $
**
** Access to library info that callers might want to have.
*/
#include <string.h>
#include <stdio.h>
#include "job_private.h"

int
jobInfo_PBSjobID()
{
    _job_in("jobInfo_PBSjobID");

    JOB_RETURN_VAL(_myEnv.session_id);
}

int
jobInfo_PBSnumNodes()
{
    _job_in("jobInfo_PBSnumNodes");

    JOB_RETURN_VAL(_myEnv.session_nodes);
}
int
jobInfo_retryLoad(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_retryLoad");

    yjob = _getJob(handle);

    if (yjob){

        JOB_RETURN_VAL(yjob->retryLoad);
    }
    else{

        JOB_RETURN_VAL(0);
    }
}
int
jobInfo_numNodes(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_jobID");

    yjob = _getJob(handle);

    if (yjob){

        JOB_RETURN_VAL(yjob->nnodes);
    }
    else{

        JOB_RETURN_VAL(0);
    }
}

int
jobInfo_jobState(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_jobID");

    yjob = _getJob(handle);

    if (yjob){

        JOB_RETURN_VAL(yjob->jobStatus);
    }
    else{
        /*
        ** successful return is a bit map, 
        ** unsuccessful return has no bits set
        */
        JOB_RETURN_VAL(0);
    }
}

int
jobInfo_jobID(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_jobID");

    yjob = _getJob(handle);

    if (yjob){

        JOB_RETURN_VAL(yjob->job_id);
    }
    else{

        JOB_RETURN_VAL(INVAL);
    }
}
int *
jobInfo_appPpidList(int handle)
{
yod_job *yjob;
int *list;
int len, i;

    _job_in("jobInfo_appPpidList");

    yjob = _getJob(handle);

    if (yjob){

        if ((yjob->nnodes == 0) || (yjob->ppidmap == NULL)){
	    _job_error_info("no node list available");
	    JOB_RETURN_VAL(NULL);
	}

	len = yjob->nnodes * sizeof(int);

        list = (int *)malloc(len);

	if (!list){
	    _job_error_info("out of memory");
	    JOB_RETURN_VAL(NULL);
	}

        if (sizeof(ppid_type) == sizeof(int)){
	    memcpy((void *)list, (void *)(yjob->ppidmap), len);
        }
        else{
            for (i=0; i<yjob->nnodes; i++){
                list[i] = yjob->ppidmap[i];
            }
        }
        JOB_RETURN_VAL(list);
    }
    else{
        JOB_RETURN_VAL(NULL);
    }
}
int *
jobInfo_nodeList(int handle)
{
yod_job *yjob;
int *list;
int len;

    _job_in("jobInfo_nodeList");

    yjob = _getJob(handle);

    if (yjob){

        if ((yjob->nnodes == 0) || (yjob->pctNidMap == NULL)){
	    _job_error_info("no node list available");
	    JOB_RETURN_VAL(NULL);
	}

	len = yjob->nnodes * sizeof(int);

        list = (int *)malloc(len);

	if (!list){
	    _job_error_info("out of memory");
	    JOB_RETURN_VAL(NULL);
	}
	memcpy((void *)list, (void *)(yjob->pctNidMap), len);

        JOB_RETURN_VAL(list);
    }
    else{
        JOB_RETURN_VAL(NULL);
    }
}
char *
jobInfo_fileName(int handle)
{
char *fname;
yod_job *yjob;

    _job_in("jobInfo_fileName");

    fname = NULL;

    yjob = _getJob(handle);

    if (yjob && (yjob->nMembers > 0) && yjob->Members){

        fname = yjob->Members[0].pname;
    }

    JOB_RETURN_VAL(fname);
}
int
jobInfo_parentID(int handle)
{
int job_id;
yod_job *yjob;

    _job_in("jobInfo_parentID");

    job_id = INVAL; 

    yjob = _getJob(handle);

    if (yjob && yjob->parent){

        job_id = yjob->parent->job_id;
    }
    JOB_RETURN_VAL(job_id);
}

void
jobPrintNodeRequest(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_nodeList");

    yjob = _getJob(handle);

    if (yjob){

        _displayNodeAllocationRequest(yjob);
    }
}
void
jobPrintNodesAllocated(int handle)
{
yod_job *yjob;

    _job_in("jobInfo_nodeList");

    yjob = _getJob(handle);

    if (yjob){
        _displayNodeList(yjob);
    }
}
void
jobPrintCompletionReport(int handle, FILE *fp)
{
int i, ii, iii, nmembers;
app_proc_done *dmsg;
int hours, minutes, seconds;
loadMbrs *mbr, *prev;
int len;
yod_job *yjob;

    _job_in("jobPrintCompletionReport");

    yjob = _getJob(handle);

    if (!yjob) JOB_RETURN;

    if (!yjob->done_status) JOB_RETURN;

    fprintf(fp,"\n");
    if (yjob->proc_done_count < yjob->nnodes){
        fprintf(fp, 
        "   ********* Responding compute nodes **********\n");
    }
    fprintf(fp, " Name");
    for (i=0; i<maxnamelen+1; i++) {
      fprintf(fp, " ");
    }
    fprintf(fp, "Rank  Node   SPID   Elapsed  Exit  Signal\n");
    for (i=0; i<maxnamelen+5; i++) {
      fprintf(fp, "-");
    }
    fprintf(fp, " ----  ----  -----  --------  ----  ------\n");

    nmembers = yjob->nMembers;
    prev = NULL;
    mbr = yjob->Members;

    for (i=0; i < nmembers; i++, mbr++){

        if (nmembers > 1){
           if ((i == 0) || (mbr->pname != prev->pname)){
               fprintf(fp,"(%s)\n", mbr->pname);
           }
           prev = mbr;           
        }

        for (ii=mbr->data.fromRank; ii<=mbr->data.toRank; ii++){

            dmsg = yjob->done_status + ii;

            if (dmsg->pid){
                hours = dmsg->elapsed / 3600;
                minutes = (dmsg->elapsed % 3600) / 60;
                seconds = dmsg->elapsed % 60;

                fprintf(fp, " ");
                len = print_node_name(dmsg->nid, fp);

                for (iii=0; iii<(maxnamelen-len+4); iii++) {
                  fprintf(fp, " ");
                }

                fprintf(fp," %4d  %4d  %5d  %02d:%02d:%02d",
                         ii, dmsg->nid, yjob->pidmap[ii],
                         hours, minutes, seconds);

                if (! (dmsg->status & STARTED)){
                    fprintf(fp,"  code never started");
                }
                else if (!(dmsg->status & SENT_TO_MAIN)){
                    fprintf(fp,"  faulted in startup, before user code");
                }
                else if (dmsg->status & CHILD_NACK){
                    fprintf(fp,"  exited in startup, before user code");
                }
                else if (dmsg->status & SENT_KILL_2){
                    fprintf(fp,"  killed by SIGKILL request");
                }
                else if (dmsg->status & SENT_KILL_1){
                    fprintf(fp,"  killed by SIGTERM request");
                }
                else{
                    if ( dmsg->final.exit_code ) 

                    fprintf(fp,"   %3d",dmsg->final.exit_code);

                    if (dmsg->final.term_sig){
                        fprintf(fp,"   %d, %s",
                           dmsg->final.term_sig,
                           select_signal_name(dmsg->final.term_sig));
                    }
                    else{
                        fprintf(fp,"   none");
                    }
                }
                fprintf(fp,"\n");

                if (yjob->backTraces && yjob->backTraces[ii]){
                     fprintf(fp," /\n");
                  fprintf(fp,"%s",yjob->backTraces[ii]);
                  fprintf(fp,
  "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
                }
            }
        }
    }
    JOB_RETURN;
}
