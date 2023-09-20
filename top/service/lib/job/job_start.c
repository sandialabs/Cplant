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
** $Id: job_start.c,v 1.2 2001/11/17 01:00:07 lafisk Exp $
**

job_request()  - parse load request, create structures required for
                  bebopd node request and structures required to
                  load the job onto the compute nodes.
                  Return handle for job or error.

job_allocate() - argument is handle, send request to bebopd for
                   nodes, return Cplant job ID, or error

job_load() - argument is handle, also whether call is blocking 
              or non-blocking.  For blocking call,
              return when apps have gone to main(), or return an
              error.  For non-blocking call, return if PCT replies
              with a TRY_AGAIN message.  Subsequent job_load() goes
              right to the REQUEST_TO_LOAD step.
**
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "job_private.h"
#include "config.h"
#include "srvr_comm.h"
#include "fileHandle.h"
#include "yod_tv.h"
#include "yod_site.h"
#include "pct_ports.h"

static int build_bebopd_requests(yod_job *yjob);
static void set_bebopd_error(int errorcode);
static void node_spec_type(char *nl, int size, int *type, nodeSpec *spec);
static int verify_list_subset(int *l1, int len1, int *l2, int len2);
static int _job_load_part2(yod_job *yjob, int blocking);
static loadMbrs *find_member(yod_job *yjob, int rank);
static void copy_warning(loadMbrs *mbr, int nd);
static void request_progress_msg(int trial, int *mtypes, int *mdata, int nnodes);

/********************************************************************************/
/********************************************************************************/
/*   Process request for nodes and create a bebopd node request from it         */
/********************************************************************************/
/********************************************************************************/

/**************************************************************************
** Process a load request and return a handle required by subsequent
** calls. 
**
**   Routine is flexible - for example if you don't want to provide a
**   list for each member, you can set "lists" to NULL, or have "lists"
**   point to an array of NULL pointers, or you can have "lists"
**   point to an array of pointers to NULL strings.  "sizes" can be
**   NULL, or an array of zeros.  We'll figure it out.
**
** 99.999% of the time there is "size" request, single program name with
** arguments and that's it.  But if you want to get fancy we're ready.
**
***************************************************************************
*/

int
job_request(int size,                 /* number of nodes for the whole job */
            int nProcsNode,           /* procs per node - not implemented */
             char *list,              /* node list for the whole job       */
             int nmembers,     /* number of members - like load file lines */
             int *sizes,            /* size request for each member */
             int *nprocs, /* procs per node request for each member */
             char **lists,     /* node list request for each member */
             char **pnames,         /* program name for each member */
             char ***pargs,    /* program arguments for each member */
             int myparent)            /* handle, or -1 if no parent */
{
int rc, handle, nargs, i, ii;
yod_job *yjob;
loadMbrs *mbr, *sameAs;
char *fullname;
char **argarray;
struct stat statbuf;

    _job_in("job_request");

    rc = _makeEnv();

    if (rc){
        JOB_RETURN_ERROR;
    }

    rc = _initComm();

    if (rc){
        JOB_RETURN_ERROR;
    }

    handle = _newJob();

    if (handle < 0){
        JOB_RETURN_ERROR;
    }

    yjob = _getJob(handle);

    /*
    ** Build node request for bebopd
    */

    yjob->globalSize          = 0;
    yjob->globalNprocs        = 0;
    yjob->globalListSize      = 0;
    yjob->globalList          = NULL;
    yjob->globalListStr       = NULL;

    if (myparent >= 0){
        yjob->parent = _getJob(myparent);

        if (! yjob->parent){
            _job_error_info("invalid parent handle\n");
            _removeJob(handle);
            JOB_RETURN_ERROR;
        }
        yjob->parent->nkids++;
    }
    else{
        yjob->parent = NULL;
    }

    /*
    ** global size argument - could be individual size arguments as well
    **                         in heterogeneous load file
    */
    if (size > 0){
        yjob->globalSize = size;

        if ((size < 0) || (size > MAX_PROC_PER_GROUP)){
            _job_error_info("invalid size: try  1 through %d nodes\n",
                   MAX_PROC_PER_GROUP);
            _removeJob(handle);
            JOB_RETURN_ERROR;
        }
        if ((_myEnv.pbs_job != NO_PBS_JOB) && (size > _myEnv.session_nodes)){
            _job_error_info(
            "Compute nodes requested (%d) exceed PBS allocation (%d).\n",
             size, _myEnv.session_nodes);
            _removeJob(handle);
            JOB_RETURN_ERROR;
       }
   }
   if (nprocs > 0){
      /*
      ** processes per node - not yet implemented
      */
      yjob->globalNprocs = nProcsNode;
   }

    /*
    ** global node list - could be individual lists as well
    **                         in heterogeneous load file
    */
    if (list && list[0]){
        if (_myEnv.pbs_job != NO_PBS_JOB){
            _job_error_info( "Node list requests can not be honored for PBS jobs.\n");
            _removeJob(handle);
            JOB_RETURN_ERROR;
        }
        else{

            rc = _process_list(list, &(yjob->globalListSize), &(yjob->globalList));

            if (rc){
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }

            yjob->globalListStr = strdup(list);

            if (!yjob->globalListStr){
                _job_error_info( "malloc failure (%s)\n",strerror(errno));
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }
        }
   }
   else{
       list = NULL;
   }

   if ( (size > 0) && (list) && (size > yjob->globalListSize)){
        _job_error_info(
           "Requested partition size of %d will not fit in list (%s) of %d nodes\n",
            size, list, yjob->globalListSize);
        _removeJob(handle);
        JOB_RETURN_ERROR;
   }
   /*
   ** A job can be simple or heterogeneous.  A simple job has a size and/or a node
   ** list, and one executable name and set of program arguments.  A heterogeneous
   ** load may have several size/list/executable/args command line specifications.
   ** The heterogeneous load may also have a global size and/or list, referring
   ** to the job as a whole.
   **
   **  size - This is the global size argument for the load.  It's the total number
   **           of nodes required by the job.  Set to 0 if all the size arguments
   **           are given in the "sizes" argument.
   **  list - Global node list, for the entire job.
   **  nmembers - number of distinct command lines in the job.  (This is 1 for a
   **                simple load, more for heterogeneous load.)
   **  sizes - The node size request for each command line.  If the second size
   **          is -1, the first size holds for all command lines.
   **  nprocs - The processors per node request for each command line.  If the
   **          second procs value is -1, the first holds for all command lines.
   **  lists - The node list request for each command line.
   **  pnames - The executable name for each command line.  If the first name
   **           is NULL, run the parent job executable.  If the first name is
   **           defined and the second name is NULL, use the named executable
   **           for each command line.
   **  pargs - The program argument string for each command line.
   */

   yjob->nMembers = nmembers;

   yjob->Members = (loadMbrs *)malloc(nmembers * sizeof(loadMbrs));

   yjob->requests = (yod_request *)malloc((nmembers+1) * sizeof(yod_request));

   yjob->ndListStr = (char **)calloc((nmembers+1) , sizeof(char *));

   yjob->ndList    = (int **)calloc((nmembers+1) , sizeof(int *));

   if (!yjob->Members || !yjob->requests || !yjob->ndListStr || !yjob->ndList){

        _job_error_info( "malloc error in job_request (%s)\n",strerror(errno));
        _removeJob(handle);
        JOB_RETURN_ERROR;
   }

   for (i=0; i<nmembers; i++){

       mbr = yjob->Members + i;

       _initMember(mbr);

       /*
       ** check job size and node list, if any
       */

       if (sizes){
           if (i && (sizes[1] > 0)){
               mbr->localSize = sizes[i];
           }
           else{
               mbr->localSize = sizes[0];
           }
       }
       else{
           mbr->localSize = 0;
       }

       if (nprocs){
           if (i && (nprocs[1] > 0)){
               mbr->localNprocs = nprocs[i];
           }
           else{
               mbr->localNprocs = nprocs[0];
           }
       }
       else{
           mbr->localNprocs = 0;
       }

       if ( lists && lists[i] && lists[i][0] &&

            (!list ||                /* if node list matches global list, ignore it */

            strcmp(list, lists[i]))) {

            if (_myEnv.pbs_job != NO_PBS_JOB){
                _job_error_info( "Node list requests can not be honored for PBS jobs.\n");
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }

            mbr->localListStr = strdup(lists[i]);

            if (!mbr->localListStr){
                _job_error_info( "malloc error in job_request (%s)\n",strerror(errno));
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }

            rc = _process_list(lists[i], &(mbr->localListSize), &(mbr->localList));

            if (rc){
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }

	    if (yjob->globalListSize){
                rc = verify_list_subset( yjob->globalList, yjob->globalListSize,
				mbr->localList, mbr->localListSize);

                if (rc){
                    _removeJob(handle);
                    JOB_RETURN_ERROR;
	        }
	    }
        }
        else{
            mbr->localListSize = 0;
        }

        /*
        ** program name 
        */
        fullname = NULL;
        sameAs = NULL;

        if (!pnames){
            if (!yjob->parent){
                _job_error_info( "Program name is missing.\n");
                _removeJob(handle);
                JOB_RETURN_ERROR;
            }
            else{
                /*
                ** load same executable as parent
                */
                if (i == 0){
                    fullname = yjob->parent->Members[0].pname;
                }
                else{
                    sameAs = yjob->Members;
                }
            }
        }
        else if ((i == 0) || (pnames[1] != NULL)){

            if (pnames[i][0] != '/'){

                if ((fullname = job_find_in_cwd(pnames[i])) == NULL){

                    fullname = job_find_in_path(pnames[i]);
                }
            }
            else{
                fullname = _real_path_name(pnames[i]);
            }
        }
        else{
	    sameAs = yjob->Members;
        }

        if (!fullname && !sameAs){
            _job_error_info( "Can't locate executable %s\n",pnames[i]);
            _removeJob(handle);
            JOB_RETURN_ERROR;
        }
        /*
        ** Some lines in the load file may be referring to the same executable,
        **  but with different arguments.  We only want to test and read in the
        **  executable once.
        */

        if (!sameAs && (i > 0)){
            sameAs = _sameExecutable(yjob, fullname, i);
        }

        if (sameAs){
            mbr->pname       = sameAs->pname;
            mbr->pnameCount  = 0;
            mbr->pnameSameAs = sameAs;

            mbr->data.execlen = sameAs->data.execlen;

            sameAs->pnameCount++;
        }
        else{

            mbr->pnameCount  = 1;
            mbr->pnameSameAs = NULL;

            mbr->pname = strdup(fullname);

            if (!mbr->pname){
                _job_error_info( "strdup error (%s)\n",strerror(errno));
                _removeJob(handle);
                 JOB_RETURN_ERROR;
            }

            rc = stat(mbr->pname, &statbuf);

            if (rc == ENOENT){
                _job_error_info("Can't stat %s\n",mbr->pname);
                _removeJob(handle);
                 JOB_RETURN_ERROR;
            }

            mbr->data.execlen = statbuf.st_size;
       }

       /*
       ** arguments
       */

       nargs = 1;
       ii=0;

       if (pargs && pargs[i]){
           while (pargs[i][ii]){

               nargs++;
               ii++;
           }
       }

       argarray = (char **)malloc((nargs+1) * sizeof(char *));

       argarray[0] = mbr->pname;

       for (ii=1; ii<nargs; ii++){

           argarray[ii] = pargs[i][ii-1];

       }
       argarray[nargs] = NULL;

       mbr->data.argbuflen = pack_string(argarray, nargs, _tmpBuf, TMPLEN);

       if (mbr->data.argbuflen < 0){
           JOB_RETURN_ERROR;
       }

       mbr->argstr = (char *)malloc(mbr->data.argbuflen);

       if (!mbr->argstr){
            _job_error_info("malloc error (%s)\n",strerror(errno));
            _removeJob(handle);
            JOB_RETURN_ERROR;
       }

       memcpy(mbr->argstr, _tmpBuf, mbr->data.argbuflen);

       free(argarray);
    }

    rc = build_bebopd_requests(yjob);   

    if (rc == -1){
	_removeJob(handle);
        JOB_RETURN_ERROR;
    }

    yjob->jobStatus |= JOB_NODE_REQUEST_BUILT;

    JOB_RETURN_VAL(handle);
}

/********************************************************************************/
/********************************************************************************/
/*   Send request to bebopd and receive list of allocated compute nodes.        */
/********************************************************************************/
/********************************************************************************/
int
job_allocate(int handle, int priority)
{
yod_job *yjob;
int listsize, rc, i;
yod_request *req;

    _job_in("job_allocate");

    yjob = _getJob(handle);

    if (yjob == NULL){
        _job_error_info("invalid job handle");
        JOB_RETURN_ERROR;
    }

    if ( ! (yjob->jobStatus & JOB_NODE_REQUEST_BUILT)){
        _job_error_info("node request not created");
        JOB_RETURN_ERROR;
    }

    if ( yjob->jobStatus & JOB_PCT_LIST_ALLOCATED){
        _job_error_info("nodes already allocated");
        JOB_RETURN_ERROR;
    }

    if (priority == SCAVENGER){
        /*
	** priority for session was established in _makeEnv, but
	** a REGULAR priority session can start SCAVENGER jobs
	*/
        for (i=0; i <yjob->nrequests; i++){
	     yjob->requests[i].priority = SCAVENGER;
	}
    }

    /*
    ** OK - send request to bebopd, await PCT list
    */

    req = yjob->requests;

    if (DBG_FLAGS(DBG_BEBOPD)){
        printf("Send node request to bebopd\n");
    }

    rc = srvr_send_to_control_ptl(_myEnv.bnid, _myEnv.bpid, _myEnv.bptl,
                        req->specType, (char *)req, sizeof(yod_request));

    if (rc < 0){
        _job_error_info("(%s) - sending request to bebopd\n",
                 CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    if (yjob->nrequests > 1){
        /*
        ** bebopd will request the list of node allocation requests
        */
	if (DBG_FLAGS(DBG_BEBOPD)){
	    printf("Await bebopd request for list of node specifications\n");
	}

        rc = _await_bebopd_get((char *)(req + 1),
            (yjob->nrequests - 1) * sizeof(yod_request), BEBOPD_GET_COMPOUND_REQ);

        if (rc < 0){                    /* internal error */
	    JOB_RETURN_ERROR;
        }
        else if (rc > 0){
            set_bebopd_error(rc);
	    JOB_RETURN_ERROR;
        }
    }

    for (i=0, req=yjob->requests; i< yjob->nrequests; i++, req++){
        /*
        ** Some requests require that bebopd pull a node list from us
        */
        if (req->specType != YOD_NODE_REQ_LIST) continue;

	listsize = req->spec.list.listsize;

	/*
	** bebopd will request the node list
	*/
	if (DBG_FLAGS(DBG_BEBOPD)){
	    printf("Await bebopd request for node list %s\n",yjob->ndListStr[i]);
	}

	rc = _await_bebopd_get((char *)(yjob->ndList[i]),
		listsize * sizeof(int), BEBOPD_GET_NODE_LIST);

	if (rc < 0){
	    JOB_RETURN_ERROR;
	}
	else if (rc > 0){
	    set_bebopd_error(rc);
	    JOB_RETURN_ERROR;
	}
    }
    if (DBG_FLAGS(DBG_BEBOPD)){
	printf("Await PCT list from bebopd.\n");
    }

    rc = _await_bebopd_reply(yjob);

    if (rc < 0){     /* internal error */
	JOB_RETURN_ERROR;
    }
    else if (rc > 0){   /* bebopd error */

        set_bebopd_error(rc);

	JOB_RETURN_ERROR;
    }
    if (DBG_FLAGS(DBG_BEBOPD)){
	printf("PCT list received\n");
    }

    if (DBG_FLAGS(DBG_ALLOC)){
        printf("Allocated pcts:\n");
        _displayNodeList(yjob);
    }

    /*
    ** yod/pct communication:  The pct sends user process
    ** pids and also sends failure/completion
    ** messages so we need a control portal to check for these.
    */
    yjob->pctPtl = srvr_init_control_ptl(yjob->nnodes);

    if (yjob->pctPtl == SRVR_INVAL_PTL){
        _job_error_info( "Can't create portal for incoming PCT messages (%s)\n",
                              CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    /*
    ** yod/app communication: We receive IO commands from the
    ** application.  We need a control portal for these.
    */
    yjob->appPtl = srvr_init_control_ptl(yjob->nnodes);

    if (yjob->appPtl == SRVR_INVAL_PTL){
        _job_error_info( "Can't create portal for incoming application messages (%s)\n",
                             CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    yjob->jobStatus |= JOB_PCT_LIST_ALLOCATED;

    JOB_RETURN_VAL(yjob->job_id);
}
static void
set_bebopd_error(int errorcode)
{

    _job_in("set_bebopd_error");

    switch (errorcode){
	case BEBOPD_ERR_INVALID:
	    _job_error_info(
	    "yod sent an invalid request to the node allocator.  This\n"
	    "should never happen.  Please notify system administration.\n");
	    break;

	case BEBOPD_ERR_INTERNAL:
	    _job_error_info(
	    "bebopd internal error, This should never happen.\n"
	    "Please notify system administration.\n");
	    break;

	/*
	** bebopd running with PBS support
	*/
	case BEBOPD_ERR_SESSION_LIMIT:

	    _job_error_info(
               "Allocating these nodes to you would give you more "
               "nodes than you were allocated by PBS.\n");
	    break;

	case BEBOPD_ERR_NO_INT_SUPPORT:

	    _job_error_info(
	      "We are dedicated completely to PBS jobs at this point in time.\n");
	    break;

	case BEBOPD_ERR_INSUFF_INT_NODES:

	    _job_error_info(
	      "We are running PBS (scheduled) jobs and have insufficient nodes\n"
	      "reserved for interactive (non-PBS) jobs at this time.\n"
	      "Run \"pingd\" to learn about nodes reserved for interactive use.\n");
	    break;

	/*
	** bebopd is NOT running in PBS support mode
	*/

	case BEBOPD_ERR_FREE_NODES:

	    _job_error_info("Insufficient number of free compute nodes.\n");
	    break;

	case BEBOPD_ERR_NO_PBS_SUPPORT:

	    _job_error_info(
   "The node allocator is not supporting PBS scheduling at this time.\n");
	    break;

	default:
	    _job_error_info("Unrecognized node allocator return code.\n"
	    "This should never happen.  Please notify system administration.\n");
	    break;
    }
    JOB_RETURN;
}

static int
build_bebopd_requests(yod_job *yjob)
{
loadMbrs *Members, *mbr, *mbr2;
int rank, tnodesany, member, next;
nodeSpec global_spec;
int global_spec_type, compoundReq;
int nnodes, i;
yod_request *reqList;
char *otherNdList;
char **ndListStr;
int **ndList;

    _job_in("build_bebopd_requests");

    Members = yjob->Members;
    reqList = yjob->requests;
    ndListStr  = yjob->ndListStr;
    ndList     = yjob->ndList;

    /*
    ** First, for each member, compute number of nodes on which to run,
    ** and from_rank and to_rank.
    */

    if (yjob->nMembers == 1){                        

        if (Members[0].localSize == 0){
            if (Members[0].localListSize > 0){
                Members[0].localSize = Members[0].localListSize;
            }
            else if (yjob->globalSize > 0){
                Members[0].localSize = yjob->globalSize;
            }
            else if (yjob->globalListSize > 0){
                Members[0].localSize = yjob->globalListSize;
            }
            else{
                Members[0].localSize = 1;
            }
        }
        if ((Members[0].localListSize > 0) &&
            (Members[0].localSize > Members[0].localListSize)){

            _job_error_info("Request of %d nodes exceeds the %d nodes in %s\n",
	      Members[0].localSize, Members[0].localListSize, 
	      Members[0].localListStr);

            JOB_RETURN_ERROR;
        }
        else if ((yjob->globalListSize > 0) &&
                 (Members[0].localSize > yjob->globalListSize)){

            _job_error_info("Request of %d nodes exceeds the %d nodes in %s\n",
	      Members[0].localSize, yjob->globalListSize, yjob->globalListStr);

            JOB_RETURN_ERROR;
        }

        Members[0].data.fromRank = 0;
        Members[0].data.toRank   = Members[0].localSize - 1;

    }
    else{                          /* heterogeneous load */
        tnodesany=0;

	for (i=0; i< yjob->nMembers; i++){

	    if (Members[i].localSize == 0){
		if (Members[i].localListSize > 0)
		    Members[i].localSize = Members[i].localListSize;
		else
		    Members[i].localSize = 1;
	    }

            if (Members[i].localListSize == 0){
                tnodesany += Members[i].localSize;
            }

	}

	if (yjob->globalListSize && (yjob->globalListSize < tnodesany)){
	    _job_error_info("The node list %s is too small to contain the %d nodes you need\n",
		   yjob->globalListStr, tnodesany);
	    JOB_RETURN_ERROR;
	}
	for (i=0, rank=0; i<yjob->nMembers; i++){
	    Members[i].data.fromRank = rank;
	    rank += Members[i].localSize;
	    Members[i].data.toRank = rank-1;
	}
    }
    /*
    ** Now build the node requests for bebopd.  If several lines in a
    ** row have the same type of request, we combine them.
    */

    yjob->nrequests = 0;

    node_spec_type(yjob->globalListStr, yjob->globalListSize,
              &global_spec_type, &global_spec);

    /*
    ** We send one of these requests for nodes:
    **
    **   YOD_NODE_REQ_ANY - We request allocation of any collection
    **                      of [n] compute nodes.
    **
    **   YOD_NODE_REQ_RANGE - We request allocation of any collection
    **        of [n] compute nodes from the physical node number range
    **        beginning with [from_node] and ending with [to_node].
    **
    **   YOD_NODE_REQ_LIST - We request allocation of any collection
    **        of [n] compute nodes from the [node_list] provided.
    **
    **   YOD_NODE_REQ_COMPOUND - We request simultaneous allocation of
    **        of [n1, n2, .., nk] compute nodes from a set of k
    **        specifications of type ANY, RANGE or LIST.  This is for
    **        the case of heterogeneous load, where the different
    **        executables may have different node type requirements.
    **
    **   The bebopd returns a list of PCTs that are reserved for us.
    **   The list retains the order of the specification passed in.
    **   (So if a node range of 100..98 was passed in, the pct list
    **   returned will list 100, 99, 98 in that order.)  Better
    **   hurry up because the PCTs won't wait forever.  They transition
    **   back to FREE if they don't hear from yod soon.
    */

    /*
    ** Do we require a compound request?
    **
    ** If we have more than one member (executable), and at least one
    ** has a node list and it does not agree with the node list of every
    ** other member, then we need to send bebopd a compound request.
    */
    compoundReq = 0;

    if (yjob->nMembers > 1){

        for (i=0; i<yjob->nMembers; i++){
    
            if (Members[i].localListStr){
                if (i > 0){
                    compoundReq = 1;
                }
                else{
                    otherNdList = Members[0].localListStr;

                    for (i=1; i< yjob->nMembers; i++){

                        if (Members[i].localListStr == NULL){
                            compoundReq = 1;
                        }
                        else if (strcmp(Members[i].localListStr, otherNdList)){
                            compoundReq = 1;
                        }
                        if (compoundReq) break;
                    }
                }
                break;
            }
        }
    }

    if (!compoundReq){ /* either there is no load file, or the load file */
                       /* has only one line, or all load file lines use  */
                       /* the same node specification                    */

        if (Members[0].localListStr){

            node_spec_type(Members[0].localListStr, Members[0].localListSize,
                      &(reqList[0].specType), &(reqList[0].spec));

            if (reqList[0].specType == YOD_NODE_REQ_LIST){

	        ndListStr[0] = Members[0].localListStr;
	        ndList[0]    = Members[0].localList;
	    }

        }
        else {
            reqList[0].specType = global_spec_type;

            if (reqList[0].specType != YOD_NODE_REQ_ANY){

                memcpy(&(reqList[0].spec), &(global_spec), sizeof(nodeSpec));

		if (reqList[0].specType == YOD_NODE_REQ_LIST){
		    ndListStr[0] = yjob->globalListStr;
		    ndList[0]    = yjob->globalList;
		}
            }
        }

        reqList[0].nnodes = 0;

        for (i=0; i< yjob->nMembers; i++){
            reqList[0].nnodes += Members[i].localSize;
        }

    }

    if (compoundReq){

        reqList[0].specType = YOD_NODE_REQ_COMPOUND;

        reqList[0].nnodes = 0;

        compoundReq = 1;

        for (member = 0; member < yjob->nMembers; member++){

            mbr = Members + member;

            nnodes = mbr->localSize;

            if (mbr->localListStr == NULL){

                /*
                ** Combine adjacent requests if they don't specify
                ** a node list.
                */
                for (next = member+1; next < yjob->nMembers; next++){

                    mbr = Members + next;

                    if (mbr->localListStr == NULL){
                        nnodes += mbr->localSize;
                        member = next;
                    }
                    else{
                        break;
                    }
                }

                reqList[0].nnodes += nnodes;

                reqList[compoundReq].nnodes = nnodes;

                reqList[compoundReq].specType = global_spec_type;

                if (global_spec_type != YOD_NODE_REQ_ANY){

                    memcpy(&(reqList[compoundReq].spec), &global_spec,
                                 sizeof(nodeSpec));

                    if (global_spec_type == YOD_NODE_REQ_LIST)
                        ndListStr[compoundReq] = yjob->globalListStr;
                        ndList[compoundReq]    = yjob->globalList;
                }

            }
            else{
                /*
                ** Combine adjacent requests if their node list
                ** is the same
                */
                for (next = member+1; next < yjob->nMembers; next++){

                    mbr2 = Members + next;

                    if (mbr2->localListStr &&
                        !strcmp(mbr->localListStr, mbr2->localListStr)) {

                        nnodes += mbr2->localSize;
                        member = next;
                    }
                    else{
                        break;
                    }
                }
                node_spec_type(mbr->localListStr, mbr->localListSize,
                   &(reqList[compoundReq].specType), &(reqList[compoundReq].spec));

                reqList[0].nnodes += nnodes;

                reqList[compoundReq].nnodes = nnodes;

		if (reqList[compoundReq].specType == YOD_NODE_REQ_LIST)
		    ndListStr[compoundReq] = mbr->localListStr;
		    ndList[compoundReq]    = mbr->localList;

            }
            compoundReq++;
        }

        reqList[0].spec.req.numRequests = compoundReq-1;
    }

    yjob->nrequests = (compoundReq == 0) ? 1 : compoundReq;

    if (yjob->nrequests < yjob->nMembers + 1){
        /*
        ** can free up some memory
        */
        yjob->requests = realloc(yjob->requests, 
	                     yjob->nrequests * sizeof(yod_request));

        if (!yjob->requests){
            _job_error_info("Memory allocation in realloc\n");
            JOB_RETURN_ERROR;
        }

        yjob->ndListStr = realloc(yjob->ndListStr, 
	                      yjob->nrequests * sizeof(char *));

        if (!yjob->ndListStr){
            _job_error_info("Memory allocation in realloc\n");
            JOB_RETURN_ERROR;
        }

        yjob->ndList= realloc(yjob->ndList, 
	                      yjob->nrequests * sizeof(int *));

        if (!yjob->ndList){
            _job_error_info("Memory allocation in realloc\n");
            JOB_RETURN_ERROR;
        }
    }

    for (i=0; i < yjob->nrequests ; i++){
	reqList[i].session_id   = _myEnv.session_id;
	reqList[i].nnodes_limit = _myEnv.session_nodes;
	reqList[i].priority     = _myEnv.session_priority;
	reqList[i].myptl        = _myEnv.bebopdPtl;
	reqList[i].euid         = _myEnv.euid;
    }

    if (DBG_FLAGS(DBG_ALLOC)){
        _displayNodeAllocationRequest(yjob);
    }

    JOB_RETURN_OK;
}

#define COMMA   44
#define DOT     46

static void
node_spec_type(char *nodelist, int listsize, int *spec_type, nodeSpec *spec)
{
char *c1, *c2;
char nodenum[10];

    _job_in("node_spec_type");

    *spec_type = YOD_NODE_REQ_ANY;

    if (listsize){

        if (strchr(nodelist, COMMA)){
            *spec_type = YOD_NODE_REQ_LIST;
            spec->list.listsize = listsize;
        }
        else{
            *spec_type = YOD_NODE_REQ_RANGE;
            if (strchr(nodelist, DOT)){

                c1 = nodelist; c2 = nodenum;
                while (isdigit(*c1)) *c2++ = *c1++;
                *c2 = 0;

                spec->range.from_node = atoi(nodenum);
                while (!isdigit(*c1)) c1++;
                spec->range.to_node   = atoi(c1);
            }
            else{
                spec->range.from_node = spec->range.to_node = atoi(nodelist);
            }
        }
    }
    JOB_RETURN;
}

static int
lessThan(const void *n1, const void *n2)
{
int a,b;

    a = *(int *)n1;
    b = *(int *)n2;

    if (a < b) return -1;
    else if (a == b) return 0;
    else return 1;
}

static int l1len=0;
static int l2len=0;
static int *l1=NULL;
static int *l2=NULL;

static int
verify_list_subset(int *biglist, int biglen, int *littlelist, int littlelen)
{
int i, j, ii, found;

    _job_in("verify_list_subset");

    if (littlelen > biglen){
	_job_error_info(
	"Node list for job member is not subset of node list for whole job.\n");
	JOB_RETURN_ERROR;
    }

    if (l1len < biglen){
	l1 = realloc(l1, biglen * sizeof(int));
    }
    if (l2len < littlelen){
	l2 = realloc(l2, littlelen * sizeof(int));
    }

    if (!l1 || !l2){
	 _job_error_info("malloc problem in verify_list_subset (%s)\n",strerror(errno));
	 JOB_RETURN_ERROR;
    }

    memcpy(l1, biglist, sizeof(int) * biglen);
    memcpy(l2, littlelist, sizeof(int) * littlelen);

    qsort(l1, biglen, sizeof(int), lessThan);
    qsort(l2, littlelen, sizeof(int), lessThan);

    for (i=0, ii=0, found=0; i<littlelen; i++){

	for (j=ii; j<biglen; j++){

	    if (l1[j] == l2[i]){
		found++;
		break;
	    }
	}

	ii = j+1;
	if (ii >= biglen) break;
    }

    if (found < littlelen){
	_job_error_info(
	"Node list for job member is not subset of node list for whole job.\n");
	JOB_RETURN_ERROR;
    }
    else{
	JOB_RETURN_OK;
    }
}

/********************************************************************************/
/********************************************************************************/
/*   LOAD cplant application onto compute nodes                                 */
/********************************************************************************/
/********************************************************************************/

/*
**  0 - OK, load successfully started (we sent a request to the PCTs to load
**          the app), and load may even be completed.
**
** -1 - failure
**
** If called in non-blocking mode, check job_load_done() to see if load
** completed.  If not, call job_load() again.
** Load could take up to 5 minutes to complete if some PCTs are killing
** off scavenger jobs.
*/


int
job_load(int handle, int blocking) 
{
yod_job *yjob;
int i, rc, new_umask;
fileHandle_t *fh;

    _job_in("job_load");

    yjob = _getJob(handle);

    if (yjob == NULL){
        _job_error_info("invalid job handle");
        JOB_RETURN_ERROR;
    }
    if ( yjob->jobStatus & JOB_APP_STARTED){
        _job_error_info("job has already been loaded onto compute nodes");
        JOB_RETURN_ERROR;
    }
    if ( !(yjob->jobStatus & JOB_PCT_LIST_ALLOCATED)){
        _job_error_info("must obtain compute nodes from bebopd before loading");
        JOB_RETURN_ERROR;
    }

    if (yjob->jobStatus & JOB_REQUESTED_TO_LOAD){

        /*
        ** we have already set up data structures and we're
        ** waiting for the PCTs to say OK to the load.
        */
        rc = _job_load_part2(yjob, blocking);

	JOB_RETURN_VAL(rc);
    }

    /*
    ** Part One of load - set up the data structures 
    */

    if (yjob->parent){
	yjob->msg1.parent_handle   = _getJobHandle(yjob->parent);
	yjob->msg1.parent_job_id   = yjob->parent->job_id;
    }
    else{
        yjob->msg1.parent_handle   = INVAL;
        yjob->msg1.parent_job_id   = INVAL;
    }

    yjob->msg1.my_handle = handle;

    yjob->msg1.job_id    = yjob->job_id;
    yjob->msg1.session_id      = _myEnv.session_id;
    yjob->msg1.nprocs          = yjob->nnodes;
    yjob->msg1.n_members       = yjob->nMembers;
    yjob->msg1.yod_id.nid      = _my_pnid;
    yjob->msg1.yod_id.pid      = _my_ppid; 
    yjob->msg1.yod_id.ptl      = yjob->pctPtl;

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("Initial request to load to be sent to PCTs\n");
        _displayInitialMsg(&(yjob->msg1));
    }

    yjob->msg2.priority        = yjob->requests[0].priority;
    yjob->msg2.app_serv_ptl    = yjob->appPtl;
    yjob->msg2.envbuflen       = _myEnv.envlen;

    initStdioFileHandles();

    fh = checkFileHandleList( STDIN_FILE_NAME );

    yjob->msg2.fstdio[0].retVal = (long)fh;
    yjob->msg2.fstdio[0].info.openAck.curPos         = fh->curPos;
    yjob->msg2.fstdio[0].info.openAck.isattyFlag     = isatty(fh->fd);
    yjob->msg2.fstdio[0].info.openAck.srvrNid        = _my_pnid;
    yjob->msg2.fstdio[0].info.openAck.srvrPid        = _my_ppid;
    yjob->msg2.fstdio[0].info.openAck.srvrPtl        = yjob->msg2.app_serv_ptl;

    fh = checkFileHandleList( STDOUT_FILE_NAME );

    yjob->msg2.fstdio[1].retVal = (long)fh;
    yjob->msg2.fstdio[1].info.openAck.curPos         = fh->curPos;
    yjob->msg2.fstdio[1].info.openAck.isattyFlag     = isatty(fh->fd);
    yjob->msg2.fstdio[1].info.openAck.srvrNid        = _my_pnid;
    yjob->msg2.fstdio[1].info.openAck.srvrPid        = _my_ppid;
    yjob->msg2.fstdio[1].info.openAck.srvrPtl        = yjob->msg2.app_serv_ptl;

    fh = checkFileHandleList( STDERR_FILE_NAME );

    yjob->msg2.fstdio[2].retVal = (long)fh;
    yjob->msg2.fstdio[2].info.openAck.curPos         = fh->curPos;
    yjob->msg2.fstdio[2].info.openAck.isattyFlag     = isatty(fh->fd);
    yjob->msg2.fstdio[2].info.openAck.srvrNid        = _my_pnid;
    yjob->msg2.fstdio[2].info.openAck.srvrPid        = _my_ppid;
    yjob->msg2.fstdio[2].info.openAck.srvrPtl        = yjob->msg2.app_serv_ptl;

    new_umask = 0x0000;
    yjob->msg2.u_mask = umask(new_umask);
    umask(yjob->msg2.u_mask);

    _updateUids();

    yjob->msg2.uid  = _myEnv.uid;
    yjob->msg2.gid  = _myEnv.gid;
    yjob->msg2.euid = _myEnv.euid;
    yjob->msg2.egid = _myEnv.egid;

    yjob->msg2.fyod_nid = 0;  /* PCTs get fyod nids now */
    yjob->msg2.ngroups = _myEnv.ngroups;

    if (_myEnv.ngroups <= FEW_GROUPS){
        for (i=0; i<_myEnv.ngroups; i++){
            yjob->msg2.groups[i] = _myEnv.groups[i];
        }
    }
    if (jobOptions.straceDirectory){
        char *c;
        int straceDirLen, straceOptLen, straceListLen;

        straceDirLen  = strlen(jobOptions.straceDirectory);
        straceOptLen  = 
          (jobOptions.straceOptions  ? strlen(jobOptions.straceOptions)  : 0);
        straceListLen = 
          (jobOptions.straceNodeList ? strlen(jobOptions.straceNodeList) : 0);

        yjob->msg2.straceMsgLen =
                  sizeof(straceInfo) +
                  straceDirLen + straceOptLen + straceListLen +
                  3;                           /* null bytes */

        yjob->straceMsg = (straceInfo *)malloc(yjob->msg2.straceMsgLen);

        if (!yjob->straceMsg){
            _job_error_info("allocating memory for strace information");
            JOB_RETURN_ERROR;
        }

        yjob->straceMsg->job_ID = yjob->job_id;
        yjob->straceMsg->dirlen = straceDirLen;
        yjob->straceMsg->optlen = straceOptLen;
        yjob->straceMsg->listlen = straceListLen;

        c = (char *)yjob->straceMsg + sizeof(straceInfo);

        strcpy(c, jobOptions.straceDirectory);
        c += yjob->straceMsg->dirlen;
        c++;

        if (yjob->straceMsg->optlen){

            strcpy(c, jobOptions.straceOptions);
            c += yjob->straceMsg->optlen;
            c++;
        }
        else{
            *c++ = 0;
        }

        if (yjob->straceMsg->listlen){
            strcpy(c, jobOptions.straceNodeList);
        }
        else{
            *c++ = 0;
        }
    }
    else{
        yjob->straceMsg = NULL;
        yjob->msg2.straceMsgLen = 0;
    }

    if (jobOptions.get_bt){
       yjob->msg2.option_bits |= OPT_BT;
    }

    if (jobOptions.attach_gdb){
       yjob->msg2.option_bits |= OPT_ATTACH;
    }

    if (jobOptions.log_startup_actions){
       yjob->msg2.option_bits |= OPT_LOG;
    }

    if (jobOptions.DebugSpecial){
       yjob->msg2.option_bits |= OPT_SPECIAL;
    }

    if (jobOptions.pauseForDebugger == 1){
       yjob->msg2.option_bits |= OPT_SLEEP_1;
    }
    else if (jobOptions.pauseForDebugger == 2){
       yjob->msg2.option_bits |= OPT_SLEEP_2;
    }
    else if (jobOptions.pauseForDebugger == 3){
       yjob->msg2.option_bits |= OPT_SLEEP_3;
    }
    else if (jobOptions.pauseForDebugger == 4){
       yjob->msg2.option_bits |= OPT_SLEEP_4;
    }


    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("Message to be fanned out to PCTs\n");
        _displayLoadMsg(&(yjob->msg2));
    }

    rc = _job_load_part2(yjob, blocking);

    JOB_RETURN_VAL(rc);
}
static int
_job_load_part2(yod_job *yjob, int blocking)
{
int *replies;
char *mdata, *put_data_buf, *c;
int mtype, pctRank, timeout;
int i, rc, trial, ntrials, copies;
int okToLoad, put_data_len, loadStarted;
double td2;
loadMbrs *mbr;
char *udata, ch[80];

    _job_in("_job_load_part2");

    (void *)replies = (void *)mdata = NULL;

    loadStarted = 0;

    /*************************************************************
    ** Notify all pcts that we are ready to load.  Don't procede
    ** until all reply OK_TO_LOAD.  In a system supporting
    ** cycle-stealing jobs, the PCTs allocated to my job may be
    ** in the process of killing off cycle-stealers.  This can
    ** take a few minutes since we're nice and give them time
    ** to clean up.  PCTs will reply OK when they're ready.  If
    ** we don't support cycle-stealing jobs, the PCTs are always
    ** ready when they are handed to us by the bebopd.
    **
    ** If you don't support cycle-stealing jobs, you can comment
    ** out this step in the load.
    **
    ** (Cycle stealing jobs come from a low priority PBS queue.
    ** They are jobs that are killable and that run on the nodes
    ** that regular PBS jobs have been allocated
    ** but are not currently using.  When the regular jobs want
    ** their nodes back, bebopd tells the PCTs to kill them,
    ** and the regular job can load an application as soon as
    ** the cycle stealer exits.)
    **
    ** (Interactive low priority jobs {"yod -nice"} are jobs that
    ** can be killed when a regular priority interactive job needs
    ** the nodes.)
    *************************************************************/

    replies = (int *)malloc(yjob->nnodes * sizeof(int));
    mdata = (char *)malloc(yjob->nnodes * sizeof(int));

    if (!replies || !mdata){
        _job_error_info("can't allocate memory");
        if (replies) free(replies);
        if (mdata) free(mdata);
        JOB_RETURN_ERROR;
    }

    ntrials = (blocking ? 14 : 1);

    yjob->retryLoad = 0;

    for (trial=0; trial<ntrials; trial++){

        if (DBG_FLAGS(DBG_LOAD_2)){
            _jobMsg("Send REQUEST_TO_LOAD message to pcts\n");
        }

        rc =  _job_send_all_pcts_control_message(yjob, MSG_REQUEST_TO_LOAD,
                         (char *)&(yjob->msg1), sizeof(load_msg1));

        if (rc){
            _job_error_info("error sending REQUEST_TO_LOAD to the pcts\n");
            goto loadFailureRetry;
        }

        if (DBG_FLAGS(DBG_LOAD_2)){
            _jobMsg("AWAIT OK message from pcts\n");
        }

        rc = _job_all_get_pct_control_message(yjob, replies, 
	                             mdata, sizeof(int), _myEnv.daemonWaitLimit);

        if (rc){
            _job_error_info("Problem getting OK TO LOAD messages from the PCTs\n");
            goto loadFailureRetry;
        }

        okToLoad = 1;

        for (i=0; i<yjob->nnodes; i++){

            if (replies[i] == TRY_AGAIN_MSG){

                 request_progress_msg(trial, replies, (int *)mdata, yjob->nnodes);

                 okToLoad = 0;
                 break;
            }
            else if (replies[i] == REJECT_LOAD_MSG){

                 _job_error_info("A node (physical %d) refuses to load the job.\n",
                                    yjob->pctNidMap[i]);

                goto loadFailureRetry;
            }
        }
        if (okToLoad || !blocking) break;

        sleep(30);

    }
    yjob->jobStatus |= JOB_REQUESTED_TO_LOAD;

    free(replies); free(mdata);
    (void *)replies = (void *)mdata = NULL;

    if (!okToLoad){

        if (!blocking){
	    JOB_RETURN_OK;
        }
        else{
           _job_error_info("Could not get OK to load from all PCTs\n");
           goto loadFailureRetry;
        }
    }

    yjob->jobStatus |= JOB_GOT_OK_TO_LOAD;

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("All PCTs reported OK TO LOAD.\n");
    }

    /*************************************************************
    ** PCTs will pull the load data and pct map from us.
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("Send INIT_LOAD message to pcts\n");
    }

    if (jobOptions.timing_data){
        td2 = dclock();
    }

    put_data_len = sizeof(load_msg2) + (sizeof(int) * yjob->nnodes);

    put_data_buf = (char *)malloc(put_data_len);

    if (!put_data_buf){
        _job_error_info("error allocating %d bytes\n",put_data_len);
         goto loadFailure;
    }

    memcpy(put_data_buf + sizeof(load_msg2), (char *)(yjob->pctNidMap), 
	  sizeof(int) * yjob->nnodes);

    loadStarted = 1;

    for (i=0; i < yjob->nMembers ; i++){

        mbr = yjob->Members + i;

        memcpy((char *)&(yjob->msg2.data), (char *)&(mbr->data),
                      sizeof(loadMemberData));

        memcpy(put_data_buf, (char *)&(yjob->msg2), sizeof(load_msg2));

        rc = _job_send_pcts_put_message(yjob, MSG_INIT_LOAD,
                          (char *)&(yjob->msg1), sizeof(load_msg1),
                           put_data_buf, put_data_len,
                           _myEnv.daemonWaitLimit, PUT_ALL_PCTS, i);

        if (rc){
            _job_error_info("error sending initial data and pct map to pcts\n");
            free(put_data_buf);
            goto loadFailureRetry;
        }
    }
    free(put_data_buf);

    loadStarted = 2;

    if (jobOptions.timing_data){
        _jobMsg("YOD TIMING: send initial msg to all pcts %f\n",dclock()-td2);
    }

    /*************************************************************
    ** Pcts are initializing their membership and their collective
    ** communication structures.  Then they acknowledge and let yod
    ** know if executable fits in RAM disk (SEND_EXEC_MSG) or does
    ** not (COPY_EXEC_MSG).
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("Await OK message from root pct\n");
    }

    if (jobOptions.timing_data){
        td2 = dclock();
    }

    for (i=0; i< yjob->nMembers; i++){

        mbr = yjob->Members + i;
        mbr->send_or_copy = INVALID_MSG;
    }

    for (i=0; i< yjob->nMembers; i++){

        rc = _job_await_pct_msg(yjob, &mtype, &udata, &pctRank, 
                                _myEnv.daemonWaitLimit);

        if (mtype == LAUNCH_FAILURE_MSG){
            _job_launch_failed(yjob, (launch_failure *)udata);
            _job_error_info("load failed, gathering post mortem\n");
	     goto loadFailureRetry;
        }
        if (rc < 0){
             _job_error_info( "Waiting for send/copy message\n");
             goto loadFailureRetry;
        }
        if ( ( (mtype!=SEND_EXEC_MSG) && (mtype!=COPY_EXEC_MSG)) ||
             (pctRank < 0) || (pctRank >= yjob->nnodes)  ) {

             _job_error_info( "invalid data in send/copy message from pcts\n");
             goto loadFailure;
         }

         mbr = find_member(yjob, pctRank);

         if (!mbr){
             _job_error_info("error with internal data structures (a.k.a a bug)\n");
             goto loadFailure;
         }

         if (mbr->send_or_copy != INVALID_MSG){
             _job_error_info( "duplicate messages received from PCTs\n");
	     goto loadFailure;
         }

         if (mtype == SEND_EXEC_MSG){
             mbr->send_or_copy = SEND_EXEC_MSG;
         }
         else if (mtype == COPY_EXEC_MSG){
             mbr->send_or_copy = COPY_EXEC_MSG;
             copy_warning(mbr, (int)(*(int *)udata));

             rc = _job_move_executable(yjob, mbr);

             if (rc){
                 _job_error_info("abandon load - can't copy executable\n");
                 goto loadFailure;
             }

             copies = 1;
         }
    }

    if (jobOptions.timing_data){
        _jobMsg("YOD TIMING: pcts form group %f\n",dclock()-td2);
    }

    /*************************************************************
    ** Send load data to the pcts.  The program arguments and
    ** executable must be sent to the root pct of each member.
    ** The environment is the same for all members so is sent
    ** only to the root pct of the entire application.
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg("Send program arguments to root PCT\n");
    }
    if (jobOptions.timing_data){
        td2 = dclock();
    }
    for (i=0; i < yjob->nMembers; i++){

        mbr = yjob->Members + i;

        rc = _job_send_pcts_put_message(yjob, MSG_PUT_ARGS, 
		  (char *)&(yjob->job_id), sizeof(int),
		  mbr->argstr, mbr->data.argbuflen, _myEnv.daemonWaitLimit,
		  PUT_ROOT_PCT_ONLY, i);
        if (rc){
            _job_error_info("can't send user command line args to pcts\n");
            goto loadFailureRetry;
        }
    }

    if (jobOptions.timing_data){
        _jobMsg("YOD TIMING: pcts pull arg data %f\n",dclock() - td2);
    }

    if (_myEnv.envlen > 0){
        if (DBG_FLAGS(DBG_LOAD_2)){
            _jobMsg("Send user environment to root PCT\n");
        }
        if (jobOptions.timing_data){
            td2 = dclock();
        }
        rc = _job_send_pcts_put_message(yjob, MSG_PUT_ENV, 
                  (char *)&(yjob->job_id), sizeof(int),
                  _myEnv.env, _myEnv.envlen, _myEnv.daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
            _job_error_info("can't send user environment to pcts\n");
            goto loadFailureRetry;
        }
        if (jobOptions.timing_data){
            _jobMsg("YOD TIMING: pcts pull env data %f\n",dclock() - td2);
        }

    }

    if (_myEnv.ngroups > FEW_GROUPS){
        if (DBG_FLAGS(DBG_LOAD_2)){
            _jobMsg("Send group list to root PCT\n");
        }
        if (jobOptions.timing_data){
            td2 = dclock();
        }
        rc = _job_send_pcts_put_message(yjob, MSG_PUT_GROUPS, 
                  (char *)&(yjob->job_id), sizeof(int),
                  (char *)_myEnv.groups, _myEnv.ngroups*sizeof(gid_t),
                  _myEnv.daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
            _job_error_info("can't send groups to pcts\n");
            goto loadFailureRetry;
        }
        if (jobOptions.timing_data){
            _jobMsg("YOD TIMING: pcts pull group data %f\n",dclock() - td2);
        }

    }

    if (yjob->straceMsg){
        if (DBG_FLAGS(DBG_LOAD_2)){
            _jobMsg("Send strace info to root PCT\n");
        }
        if (jobOptions.timing_data){
            td2 = dclock();
        }
        rc = _job_send_pcts_put_message(yjob, MSG_PUT_STRACE, 
                  (char *)&(yjob->job_id), sizeof(int),
                  (char *)(yjob->straceMsg), yjob->msg2.straceMsgLen,
                  _myEnv.daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
            _job_error_info("can't send strace request to pcts\n");
            goto loadFailureRetry;
        }
        if (jobOptions.timing_data){
            _jobMsg("YOD TIMING: pcts pull strace request info %f\n",dclock() - td2);
        }
    }

    if (jobOptions.timing_data){
        _jobMsg("YOD TIMING: Tell pcts to come get executable\n");
        td2 = dclock();
    }
    for (i=0; i < yjob->nMembers; i++){

        mbr = yjob->Members + i;

        if (mbr->send_or_copy == SEND_EXEC_MSG){

            if (DBG_FLAGS(DBG_LOAD_2)){
                _jobMsg("Send program executable file to PCT %d\n",
                                 mbr->data.fromRank);
            }

            if (mbr->exec == NULL){

                rc = _job_read_executable(yjob, i);

                if (rc){
                    _job_error_info("Can't read executable into memory\n");
                    goto loadFailure;
                }

                if (!jobOptions.bypass_link_version_check){

                    rc = _job_check_link_version(yjob, i);

                    if (rc){
                        _job_error_info("error checking link version\n");
                        goto loadFailure;
                    }
                }
            }

            rc = _job_send_executable(yjob, i);
            if (rc){
                _job_error_info("can't send executable to pcts\n");
                goto loadFailureRetry;
            }
        }
        else{

            if (DBG_FLAGS(DBG_LOAD_2)){
                _jobMsg("Send program path name to PCT %d\n",
                                 mbr->data.fromRank);
            }

            rc = _job_send_pcts_put_message(yjob, MSG_PUT_EXEC_PATH, 
                  (char *)&(yjob->job_id), sizeof(int),
                  mbr->execPath, strlen(mbr->execPath)+1, _myEnv.daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, i);

            if (rc){
                _job_error_info("Error sending name %s to pcts\n",mbr->execPath);
                goto loadFailureRetry;
            }
        }
    }
    if (jobOptions.timing_data){
        _jobMsg("       executable(s) sent to compute partition: %f\n",
                       dclock()-td2);
    }

    /*************************************************************
    ** PCTs fork the application process. Each app process sends a
    ** message to its PCT with its portal ID. Each PCT sends the
    ** portal ID and application process ID (if different) to yod.
    ** yod builds a portal ID map and sends it up to the root PCT
    ** for broadcast.
    **
    ** It is essential that no one send portals messages to the
    ** PCT during the fork call, as these messages may be lost.
    ** To ensure this, each PCT sends the READY token to yod and
    ** sits down for awhile.  After receiving the message to come
    ** get the global portal ID map, the PCT knows all PCTs are
    ** out of fork. It is now safe for the PCTs to resume talking
    ** to each other.
    **
    ** Of course if the user interrupts yod at this point, yod
    ** will send an ABORT message to the PCTs which may get lost
    ** if it arrives during the fork call.  Our user will hopefully
    ** notice nothing happened and interrupt yod again, this
    ** time successfully.
    *************************************************************/

    i        = 0;

    timeout = _myEnv.daemonWaitLimit;

    if (jobOptions.pauseForDebugger){
        timeout += 60;  /* allow more time since app process will sleep(60) */
    }

    td2      = dclock();

    _jobMsg(
    "Awaiting synchronization of compute nodes before beginning user code.\n");

    replies = (int *)malloc(yjob->nnodes * sizeof(int));
    mdata   = (char *)malloc(yjob->nnodes * SRVR_USR_DATA_LEN);

    if (!replies || !mdata){
       _job_error_info("Out of memory\n");
       goto loadFailure;
    }

    yjob->pidmap = (spid_type *)malloc(yjob->nnodes * sizeof(spid_type));
    yjob->ppidmap = (ppid_type *)malloc(yjob->nnodes * sizeof(ppid_type));

    if (!yjob->pidmap || !yjob->ppidmap){
       _job_error_info("Out of memory (2)\n");
       goto loadFailure;
    }

    rc = _job_all_get_pct_control_message(yjob, replies, mdata, 
                                        SRVR_USR_DATA_LEN, timeout);

    if (rc){
        _job_error_info("Error awaiting synchronization messages from PCTs\n");
        goto loadFailureRetry;
    }

    for (pctRank=0, c=mdata; pctRank<yjob->nnodes; 
         pctRank++, c+=SRVR_USR_DATA_LEN) {

        if (replies[pctRank] == APP_PORTAL_ID){

            if (DBG_FLAGS(DBG_PROGRESS)){
                _jobMsg("Received portal ID message from node %d\n", 
                         yjob->pctNidMap[pctRank]);
            }

            yjob->pidmap[pctRank]  = ((appID *)c)->pid;
            yjob->ppidmap[pctRank] = ((appID *)c)->ppid;

        }
        else if (replies[pctRank] == LAUNCH_FAILURE_MSG){
             _job_launch_failed(yjob, (launch_failure *)(c));
             goto loadFailure;
        }
        else{
             _job_error_info(
               "want APP_PORTAL_ID, mtype = %d (%s)\n",replies[pctRank],
                       select_pct_to_yod_string(replies[pctRank]));
             goto loadFailure;
        }
    }

    free(mdata); free(replies);
    (void *)mdata = (void *)replies = NULL;

    if (jobOptions.timing_data){
        _jobMsg("       compute partition synchronized: %f\n",
                       dclock()-td2);
    }
    /* Send map of portal IDs to root PCT for broadcast
    */
    if (DBG_FLAGS(DBG_LOAD_2)) {
      _jobMsg( "Send portal ID map to root PCT\n");
    }
    if (jobOptions.timing_data) {
      td2 = dclock();
    }

    rc = _job_send_pcts_put_message(yjob, MSG_PUT_PORTAL_IDS, 
         (char*)&(yjob->job_id), sizeof(int),
         (char*)(yjob->ppidmap), (sizeof(ppid_type) * yjob->nnodes),
         _myEnv.daemonWaitLimit, PUT_ROOT_PCT_ONLY, 0);

    if (rc){
        _job_error_info("can't send portal ID map to pcts\n");
        goto loadFailureRetry;
    }

    if (jobOptions.timing_data){
      _jobMsg("YOD TIMING: root pct pulls portal ID map %f\n",dclock()-td2);
    }

    /* PCTs supply the portal ID map to the application processes, the
       app processes complete pre-main initialization, and send a READY
       message to PCT. The PCTs synchronize, and root PCT sends a
       READY message to yod.
    */

    if (DBG_FLAGS(DBG_LOAD_2)){
        _jobMsg( "Await READY message from root PCT\n");
    }

    rc = _job_await_pct_msg(yjob, &mtype, &udata , NULL, timeout);

    if ( (rc < 0) || (mtype != APP_READY_MSG)){
         if (mtype == LAUNCH_FAILURE_MSG){ 
             _job_launch_failed(yjob, (launch_failure *)udata);
             _job_error_info("application failed before user code\n");
             goto loadFailure;
         }
         else{
             _job_error_info( "Apparent problem with compute nodes.\n");
             goto loadFailureRetry;
         }
    }
    if (DBG_FLAGS(DBG_PROGRESS)){
        _jobMsg("Received READY message from node %d\n", yjob->pctNidMap[0]);
    }
    if (jobOptions.timing_data){
        _jobMsg(
        "YOD TIMING: %f elapsed from start of executable fanout to apps ready\n",
                    dclock()-td2);
    }

    if (yjob->parent == NULL){
        /*
        ** We support Totalview debugging on the original
        ** application only, not on it's spawned children.
        */
        init_TotalView_symbols( yjob->nnodes, yjob->pctNidMap, yjob->pidmap,
                                yjob->Members, yjob->nMembers);
    }

    /* "yod -a" displays PCTs allocated and prompts to send application
       along to user code
    */

    if (jobOptions.display_allocation){

        printf("Allocated nodes in process rank order:\n");
        print_node_list(stdout, yjob->pctNidMap, yjob->nnodes,
                           60, 10);

        printf( "Proceed to application main? (y/n) \n");
        scanf("%s", ch);

        if ( (ch[0] != 'y') && (ch[0] != 'Y')){
            printf("Job cancelled\n");
            goto loadFailure;
        }
    }

    if ( MPIR_being_debugged ) {
        MPIR_debug_state = MPIR_DEBUG_SPAWNED;
        MPIR_Breakpoint();
    }

    if ( jobOptions.attach_gdb ) {
        printf("Physical nodes in process rank order:\n");
        print_node_list(stdout, yjob->pctNidMap, yjob->nnodes,
                           10, 60);

        _jobMsg( "Press <ENTER> to proceed to main:\n");
        getchar();
    }

    rc = srvr_send_to_control_ptl(yjob->pctNidMap[0], PPID_PCT,
           PCT_LOAD_PORTAL, MSG_GO_MAIN,
           (char *)&(yjob->job_id), sizeof(int));

    if (rc < 0){
        _job_error_info("error sending \"go main\" message to application");
        goto loadFailure;
    }
    _jobMsg("Application processes begin user code.\n");

    yjob->jobStatus |= JOB_APP_STARTED;

    _job_mkdate(time(NULL), &(yjob->startTime));

    _job_log_start(yjob);

    JOB_RETURN_OK;

loadFailureRetry:

    yjob->retryLoad = 1;

loadFailure:

    if (replies) free(replies);
    if (mdata) free(mdata);

    if (loadStarted < 2){

        /*
        ** PCTs are pending for job until they get MSG_INIT_LOAD or
        ** MSG_CANCEL_REQUEST_TO_LOAD.
        */
        _job_send_all_pcts_control_message(yjob, MSG_CANCEL_REQUEST_TO_LOAD,
		 (char *)&(yjob->job_id), sizeof(int));

    }
    if (loadStarted > 1){

        /*
        ** PCTs are loading job once they have received MSG_INIT_LOAD.
        ** MSG_ABORT_RESET cancels the load.
        */
        _job_send_all_pcts_control_message(yjob, MSG_ABORT_RESET,
		 (char *)&(yjob->job_id), sizeof(int));
    }

    _rollBackToUnallocated(yjob);

    JOB_RETURN_ERROR;
}
void
job_load_cancel(int handle)
{
yod_job *yjob;

    _job_in("job_load_cancel");

    yjob = _getJob(handle);

    if (yjob == NULL){
        JOB_RETURN;
    }
    _job_send_all_pcts_control_message(yjob, MSG_CANCEL_REQUEST_TO_LOAD,
	 (char *)&(yjob->job_id), sizeof(int));

    _job_send_all_pcts_control_message(yjob, MSG_ABORT_RESET,
  	 (char *)&(yjob->job_id), sizeof(int));

    if ((yjob->jobStatus & JOB_GOT_OK_TO_LOAD) &&
        ~(yjob->jobStatus & JOB_APP_FINISHED)        ){

        _job_remove_executables(yjob);
    }

    _rollBackToUnallocated(yjob);

    JOB_RETURN;
}
/*
** Free up the library resources associated with a job.  Do this
** if the job is abandoned, for instance if the load fails.  For
** jobs that load and run, you probably want to keep the job
** structure for reporting, querying later on.
*/
void
job_free(int jobHandle)
{
yod_job *yjob;
int status;

    _job_in("job_free");

    yjob = _getJob(jobHandle);

    if (!yjob) JOB_RETURN;

    status = yjob->jobStatus;

    if ((status & JOB_GOT_OK_TO_LOAD) &&
        ~(status & JOB_APP_FINISHED)        ){

        _job_remove_executables(yjob);
    }

    _removeJob(jobHandle);

    JOB_RETURN;
}

static void
copy_warning(loadMbrs *mbr, int nd)
{
    _job_in("copy_warning");

    if (!mbr) JOB_RETURN;

    _jobMsg("Warning, your executable %s\n", mbr->pname);

    _jobMsg("may take longer than usual to load because it's size (%d)\n",
                    mbr->data.execlen);
    _jobMsg("exceeds the memory available on node %d to store it.\n", nd);
    _jobMsg("We need to copy it somewhere.  Please be patient.\n\n");

    JOB_RETURN;
}

static loadMbrs *
find_member(yod_job *yjob, int rank) 
{
loadMbrs *mbr;
int i;

    _job_in("find_member");

    mbr = NULL;

    for (i=0; i< yjob->nMembers; i++){

        if ( (rank >= yjob->Members[i].data.fromRank) &&
             (rank <= yjob->Members[i].data.toRank)       ){

             mbr = yjob->Members + i;
             break;
        }
    }
    JOB_RETURN_VAL(mbr);
}

static void
request_progress_msg(int trial, int *mtypes, int *mdata, int nnodes)
{
int waittime, i;

    _job_in("request_progress_msg");

    waittime = 0;

    for (i=0; i<nnodes; i++){
        if ((mtypes[i] == TRY_AGAIN_MSG) && (mdata[i] > waittime))
             waittime = mdata[i];
    }

    if (trial > 0){
        _jobMsg(
        ">>>Still waiting for currently running job(s) to exit. (%d seconds)\n", waittime);
    }
    else{
        if (_myEnv.session_id != INVAL){
            _jobMsg(
            "   A cycle stealing job is being killed to make way for your job.  Please wait.\n");
        }
        else{
            _jobMsg(
            "   Some of the nodes you were allocated are running a low-priority job.\n");
            _jobMsg(
            "   The job is being interrupted and killed.  This may take a few minutes.\n");
            _jobMsg(
            "   If you don't wish to wait, interrupt yod with a control-C.\n");
        }
        _jobMsg(
        "   At most %d seconds remain until the nodes will be available.\n",waittime);
    }

    JOB_RETURN;
}

/*
** -1 - error
**  0 - job not loaded yet
**  1 - job has loaded
*/
int
job_load_done(int handle)
{
yod_job *yjob;

    _job_in("job_load_done");

    yjob = _getJob(handle);

    if (yjob == NULL){
        _job_error_info("invalid job handle");
        JOB_RETURN_ERROR;
    }
    if ( yjob->jobStatus & JOB_APP_STARTED){
        JOB_RETURN_VAL(1);
    }
    JOB_RETURN_VAL(0);
}
