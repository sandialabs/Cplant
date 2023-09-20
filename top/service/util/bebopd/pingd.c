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
** $Id: pingd.c,v 1.77.2.1 2002/03/20 21:46:16 ktpedre Exp $
**
*/
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <ctype.h>
#include <string.h>

#ifdef __linux__
#include <getopt.h>
#endif
#ifdef __osf__
#include <linux-getopt.h>
#endif

#include "sys_limits.h"
#include "bebopd.h"
#include "bebopd_private.h"
#include "srvr_comm.h"
#include "appload_msg.h"
#include "config.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "sys/defines.h"

#ifndef RELIABLE
/* this should be longer than the definition
   used in bebopd_alloc.c since pingd is waiting
   on bebopd, which is waiting on the pct...
*/
#define WaitPCTstatus 20
#endif

#include "cplant_host.h"

extern int Dbglevel;
extern int maxnamelen;

static int ping_request;
static int start_nid;
static int end_nid;
static int start_pbs_nid;
static int end_pbs_nid;
static int verbose;
static int noinquire;
static int nicekill;
static int jobNum;
static int sessionNum;
static int altBpid, altBnid;
static int mineOnly;
static struct passwd *pw;
static int privLevel;
static unsigned int Euid;
static int node_list[MAX_NODES];
static int nnodes;
static int pbs_list[MAX_NODES];
static int pbs_nnodes;
static int allNodes;           /* query all PCTs */
static int displayAllNodes;    /* display all */
static int summary_only;
static int parseable;
static int simple;
static int pbs_simple;
static int adminOp;
static int interactiveNodes;
static char rUser[64];
static uid_t rUid;
static int daemonWaitLimit=30;

static int send_request(int bnid, int bpid, int bptl, int myptl);
static int provide_node_list(int myptl);
static void query_pcts(int myptl);

#ifndef STRIPDOWN
static void print_child_status_string(UINT16 st);
static void print_nice_kill_status(int seconds);
static void print_child_exit_code(final_status final);
#endif

static pct_rec pcts[MAX_NODES];

static struct option pingd_options[] =
{
  {"reserve", required_argument,              0, 'a'},
  {"unreserve", no_argument,            0, 'b'},
  {"PBSinteractive", required_argument, 0, 'i'},
  {"PBSlist", required_argument,        0, 'l'},
  {"summary", no_argument,              0, 'm'},
  {"parse", no_argument,                0, 'p'},
  {"PBSsupport", required_argument,     0, 's'},
  {"PBSupdate", required_argument,      0, 'u'},
  {"all", no_argument, 0,          'A'},
  {"Debug", no_argument, 0,        'D'},
  {"fast", no_argument, 0,         'F'},
  {"gone", no_argument, 0,         'G'},
  {"help", no_argument, 0,         'H'},
  {"interrupt", no_argument, 0,    'I'},
  {"job", required_argument, 0,    'J'},
  {"kill", no_argument, 0,         'K'},
  {"list", required_argument, 0,   'L'},
  {"mine", no_argument, 0,         'M'},
  {"nicekill", no_argument, 0,     'k'},
  {"nid", required_argument, 0,    'N'},
  {"pid", required_argument, 0,    'P'},
  {"NoInquire", no_argument, 0,    'Q'},
  {"reset", no_argument, 0,        'R'},
  {"pbsjob", required_argument, 0, 'S'},
  {"verbose", no_argument, 0,      'V'},
  {"xtrahelp", no_argument, 0,     'X'},
  {"Scan", no_argument, 0,         'C'},
  {0,0,0,0}
};
static void
node_reservation_warning()
{
printf("\n*************************WARNING************************************\n");
printf("It can create havoc to reserve nodes when PBS is running.\n");
printf("PBS jobs that are already running on a very full machine may fail if the\n");
printf("nodes can't be used.  Also, unless you reduce the PBS attribute\n");
printf("\"resources_available.size\", PBS thinks it has use of those nodes\n");
printf("and may schedule jobs to run that will try to load applications on them.\n\n");
printf("In addition - be aware that reserving a node does not kill a job running\n");
printf("on it.  Reserving the node means that the node allocator will not\n");
printf("in the future allocate it to anyone except %s.\n\n",rUser);
printf("Note that to reserve a node already reserved for someone else,\n");
printf("you must first unreserve the node.\n");
printf("*************************WARNING************************************\n\n");

}
static void
node_list_usage(void)
{
printf("A node-list is a comma separated list of nodes or node ranges.  A node\n");
printf("range is two node numbers separated by one or more dots.  No white space\n");
printf("in node-list please.  Example: 1,5,7,12..18,100..150,152\n\n");

printf("To make life easier, the \"-l\" preceding the node-list may be omitted.\n\n");

printf("Combinations of node specifications are ANDed; we take the most \n");
printf("restrictive interpretation.\n\n");
}
static void
admin_options_usage(void)
{
printf("\nADMINISTRATOR OPTIONS:\n");
printf("-kill                Kill selected PCTs.\n\n");

printf("-reserve [id]      Reserve nodes for specified user (user name or numeric ID).\n");
printf("-unreserve         Turn reservation off for this set of nodes.\n\n");

printf("-gone                Inform bebopd that PCTs on specified nodes are dead.\n\n");
printf("-pid [n]             Alternative portal PID of bebopd, system testing only.\n"); 
printf("-nid [n]             Alternative node ID for bebopd, system testing only.\n\n"); 
printf("-PBSsupport [on|off] If on, bebopd maintains compute partition node count for PBS.\n");
printf("-PBSupdate [on|off]  If on, bebopd updates PBS with changes in count of active compute nodes.\n");
#ifndef STRIPDOWN
printf("-PBSinteractive [n]  Tells bebopd to reserve [n] nodes for interactive (non-PBS) jobs.\n");
printf("-PBSlist [node-list] Tells bebopd to use the listed nodes for interactive jobs.\n");
#else
printf("-PBSlist [node-list|null] Required arg tells bebopd to use either [list|no nodes] for interactive jobs.\n");
#endif
}

static void
options_usage(void)
{
printf("DEFAULT:\n");
printf("pingd with no options will query the compute nodes and display status.\n\n");
printf("OPTIONS:\n");
printf("-fast       Report last update from compute nodes without querying them.\n");
printf("-Scan       Query all nodes, including stale and unavailable nodes.\n");
printf("-verbose    Display status information about my running jobs.\n\n");
printf("-reset      Instead of displaying status, reset selected compute nodes.\n");
printf("            Application will be killed and node reset to FREE status.\n");
printf("            You must own the job on the node.\n\n");
printf("-interrupt  Instead of displaying status, interrupt (with SIGTERM) the\n");
printf("            user process on the selected compute nodes.\n");
printf("            You must own the job on the node to interrupt it.\n\n");
printf("-nicekill   Instead of displaying status, interrupt (with SIGTERM) the\n");
printf("            the selected job.  If the job has not terminated in 5 minutes,\n");
printf("            kill it with SIGKILL.  You must own the job to \"nicekill\" it.\n");
printf("            The granularity of \"nicekill\" is a job.  You can not \"nicekill\"\n");
printf("            just some of the members of the parallel application.\n\n");
printf("-summary    Don't display a line per node, just display totals.\n");
printf("-help       Display this help message.\n");
printf("-xtrahelp   Display an even more verbose help message.\n");
printf("-parse      List output in an easily parseable format.\n");
printf("-NoInquire  Tells pingd to do the requested operation without an\n");
printf("             \"are you really sure you want to do this\" type of query.\n\n");


printf("To specify the nodes displayed, reset or interrupted:\n\n");
printf("   -all              all nodes (DEFAULT)\n");
printf("   -mine             only nodes running my jobs\n");
printf("   -job [n]          only nodes running Cplant job ID #n\n");
printf("   -pbsjob [n]       only nodes running PBS job ID #n\n");
printf("   -list [node-list] listed nodes only\n\n");
}

static void
examples(void)
{
printf("EXAMPLES:\n");
printf("   pingd -f\n");
printf("          (display the last update received from compute partition)\n");
printf("   pingd -m -v\n");
printf("          (a verbose display of the status of my compute nodes)\n");
printf("   pingd -reset -j 25\n");
printf("          (reset all nodes running job ID #25, assuming I own it)\n");
printf("   pingd -l 22..100 -m\n");
printf("          (query 22 through 100, display status of those running my jobs)\n");
printf("   pingd 102\n");
printf("          (what's up on node 102 ?)\n");
}
static void
verbose_usage(void)
{
    options_usage();
    admin_options_usage();
    node_list_usage();
    examples();
}
static void
settings(void)
{
    if (ping_request == PCT_DIE_REQUEST) 
            printf("***KILL ");
    else if (ping_request == PCT_RESET_REQUEST) 
            printf("***reset to FREE ");
    else if (ping_request == PCT_INTERRUPT_REQUEST) 
            printf("***interrupt (with a SIGTERM) user processes started by ");
    else if (ping_request == PCT_NICE_KILL_JOB_REQUEST) 
            printf("***nicely kill (with a SIGTERM/delay/SIGKILL) user processes started by ");
    else if (ping_request == PCT_GONE_UPDATE) 
            printf("***inform bebopd of death of ");
    else if (ping_request == PCT_RESERVE_REQUEST) 
            printf("***Reserve for user %s (%d) ",rUser,rUid);
    else 
            return;

    if ((start_nid == 0) && (end_nid == MAX_NODES-1))
        printf("all pcts\n");
    else if (simple)
        printf("pcts on nodes %d through %d\n",start_nid,end_nid);
    else{
        printf("%d pcts on nodes:\n",nnodes);
        print_node_list(stdout, node_list, nnodes, 40, 3);
    }

    if ((ping_request == PCT_DIE_REQUEST) || 
        (ping_request == PCT_RESET_REQUEST) ||
        (ping_request == PCT_RESERVE_REQUEST) ||
        (ping_request == PCT_NICE_KILL_JOB_REQUEST) ||
        (ping_request == PCT_INTERRUPT_REQUEST)   ){
        if (jobNum != INVAL){
            printf("***only if they are running job number %d\n",jobNum);
            if (mineOnly || (privLevel > 1)){
                printf("***AND I own it\n");
            }
        }
        else if (sessionNum != INVAL){
            printf("***only if they are running PBS job number %d\n",sessionNum);
            if (mineOnly || (privLevel > 1)){
                printf("***AND I own it\n");
            }
        }
        else{
            if (mineOnly || (privLevel > 1)){
                printf("***only if they are running applications owned by me\n");
            }
        }
    }

    printf("***\n");
}
/*
** pingd can only handle one ping_request.  Subsequent requests on 
** the command line are ignored.  
*/
static void
pingd_request_error(int opttype)
{
int i;
const char *opstring;

   opstring = NULL;

   for (i=0; pingd_options[i].name; i++){
      if (pingd_options[i].val == opttype){
          opstring = pingd_options[i].name;
	  break;
      }
   }
   if (opstring == NULL){
       printf("ping_request_error: invalid argument\n");
       exit(0);
   }
   printf("\tSORRY: pingd can only send one request to the bebopd at a time.\n");
   printf("\tWe'll ignore the (%s) request.\n\n", opstring);
}

#define MULTIPLE_REQUEST_CHECK                  \
      if (ping_request != INVALID_PCT_REQUEST){ \
          pingd_request_error(opttype);         \
          break;                                \
      }


static int
findUid(char *arg, char *nam, uid_t *id)
{
struct passwd *pwent;

    if (isdigit(arg[0])){

        *id = atoi(arg);

	pwent = getpwuid(*id);

	if (pwent){
	    strncpy(nam, pwent->pw_name, 63);
	    return 0;
	}
    }
    else{

	strncpy(nam, arg, 63);

	pwent = getpwnam(arg);

	if (pwent){

	    *id = pwent->pw_uid;
	    return 0;
	}
    }

    printf("Can't find user %s in the password file\n",arg);

    return -1;
}
 
static int
get_options(int argc, char *argv[])
{
int opttype, i, adminOops, rc;
 
    ping_request = INVALID_PCT_REQUEST;
    nnodes       = 0;
    pbs_nnodes    = 0;
    verbose      = 0;
    noinquire    = 0;
    nicekill     = 0;
    mineOnly     = 0;
    simple       = 0;
    pbs_simple   = 0;
    adminOp      = 0;
    adminOops    = 0;
    jobNum       = INVAL;
    sessionNum   = INVAL;
    allNodes         = 0;
    displayAllNodes  = 1;
    summary_only     = 0;
    parseable        = 0;
    interactiveNodes = 0;
    altBpid         = SRVR_INVAL_PID;
    altBnid         = SRVR_INVAL_NID;
    rUser[0]        = 0;
    rUid            = -1;

    while (1){
        opttype = getopt_long_only(argc, argv, "+", pingd_options, 0);
 
        if (opttype == EOF){

            if (argv[optind]){
                if (!isdigit(argv[optind][0]) || (nnodes > 0)) break;

                /*
                ** It's a common error to type in a node number without
                ** the "-l" option specifier.  So we'll recognize this
                ** as a node list and continue parsing the command line.
                */
                optarg = argv[optind];
                optind++;
                opttype = 'L';
            }
            else{
                break;
            }
        }
 
        switch (opttype){

            case 'a':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_RESERVE_REQUEST;

		rc = findUid(optarg, rUser, &rUid);

		if (rc){
		    adminOops = 1;
		}
                break;

            case 'b':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_UNRESERVE_REQUEST;
                break;

            case 'i':
#ifndef STRIPDOWN /* only use -PBSlist now */
	        MULTIPLE_REQUEST_CHECK;

                interactiveNodes = atoi(optarg);

		if ((interactiveNodes < 0) || (interactiveNodes > MAX_NODES)){
		    adminOops = 1;
		}
		else{
		    ping_request = BEBOPD_PBS_INTERACTIVE;
		    adminOp = 1;
		}
#endif
                break;

            case 'l':                     /* -PBSlist */
	        MULTIPLE_REQUEST_CHECK;

#ifdef STRIPDOWN
                /* empty list */ 
                if ( strcmp("null",optarg) == 0 ) { 
                  optarg = NULL;
                }
#endif
                pbs_nnodes = parse_node_list(optarg, pbs_list, MAX_NODES,
                                         0, MAX_NODES-1);

#ifndef STRIPDOWN /* expect a non-void node list */ 
                if (pbs_nnodes <= 0){
                    node_list_usage();
                    examples();
                    return -1;
                }
#else /* null list OK */
                if (pbs_nnodes < 0){
                  printf("use \"-PBSlist null\" or\n");
                  printf("    \"-PBSlist node-list\"\n");
                  node_list_usage();
                  examples();
                  return -1;
                }
#endif


#ifdef STRIPDOWN
                if (pbs_nnodes == 0) {
                  start_pbs_nid = -2;
                  pbs_simple = 1; 
                }
                else {
#endif
                  pbs_simple = simpleNodeRange(pbs_list, pbs_nnodes);
                
		  if (pbs_simple){
		    if (pbs_list[0] <= pbs_list[pbs_nnodes-1]){
			start_pbs_nid = pbs_list[0];
			end_pbs_nid = pbs_list[pbs_nnodes-1];
		    }
		    else{
			end_pbs_nid = pbs_list[0];
			start_pbs_nid = pbs_list[pbs_nnodes-1];
		    }
		  }
#ifdef STRIPDOWN
                }
#endif

		ping_request = BEBOPD_PBS_LIST;
		adminOp = 1;
                
                break;


            case 'm':
                summary_only = 1;
                break;

            case 'p':
                parseable = 1;
                break;

            case 's':
	        MULTIPLE_REQUEST_CHECK;

		if ((optarg[0] == 'o') || (optarg[0] == 'O')){
		    if ((optarg[1] == 'n') || (optarg[1] == 'N')){
			ping_request = BEBOPD_PBS_SUPPORT_ON;
			adminOp = 1;
		    }
		    else if ((optarg[1] == 'f') || (optarg[1] == 'F')){
			ping_request = BEBOPD_PBS_SUPPORT_OFF;
			adminOp = 1;
		    }
		    else{
			adminOops = 1;
		    }
		}
		else{
		    adminOops = 1;
		}
                break;
 
            case 'u':
	        MULTIPLE_REQUEST_CHECK;

		if ((optarg[0] == 'o') || (optarg[0] == 'O')){
		    if ((optarg[1] == 'n') || (optarg[1] == 'N')){
			ping_request = BEBOPD_PBS_UPDATE_ON;
			adminOp = 1;
		    }
		    else if ((optarg[1] == 'f') || (optarg[1] == 'F')){
			ping_request = BEBOPD_PBS_UPDATE_OFF;
			adminOp = 1;
		    }
		    else{
			adminOops = 1;
		    }
		}
		else{
		    adminOops = 1;
		}
                break;

            case 'C':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_SCAN;
                break;
 
            case 'D':
                Dbglevel++;
                break;

            case 'F':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_FAST_STATUS;
                break;
 
            case 'G':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_GONE_UPDATE;
                break;

            case 'I':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_INTERRUPT_REQUEST;
                break;

            case 'k':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_NICE_KILL_JOB_REQUEST;
                break;

            case 'J':
                jobNum = atoi(optarg);
		displayAllNodes = 0;
                break;

            case 'K':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_DIE_REQUEST;
                break;

            case 'L':

                if (nnodes > 0){
                    printf("One node specification only please\n");
                    return -1;
                }
                nnodes = parse_node_list(optarg, node_list, MAX_NODES,
                                         0, MAX_NODES-1);
                if (nnodes <= 0){
                    node_list_usage();
                    examples();
                    return -1;
                }

                simple = simpleNodeRange(node_list, nnodes);
                
		if (simple){
		    if (node_list[0] <= node_list[nnodes-1]){
			start_nid = node_list[0];
			end_nid = node_list[nnodes-1];
		    }
		    else{
			end_nid = node_list[0];
			start_nid = node_list[nnodes-1];
		    }
		}

		displayAllNodes = 0;
                
                break;

            case 'A':
                allNodes = 1;
                break;

            case 'H':
                options_usage();
                exit(0);

            case 'M':
                mineOnly = 1;
		displayAllNodes = 0;
                break;

            case 'N':
                altBnid = atoi(optarg);
                break;

            case 'P':
                altBpid = atoi(optarg);
                break;

            case 'Q':
                noinquire = 1;
                break;

            case 'R':
	        MULTIPLE_REQUEST_CHECK;

                ping_request = PCT_RESET_REQUEST;
                break;

            case 'S':
                sessionNum = atoi(optarg);
		displayAllNodes = 0;
                break;
 
            case 'V':
                verbose = 1;
                break;
 
            case 'X':
                verbose_usage();
                exit(0);
 
            default:
		options_usage();
		admin_options_usage();
                return -1;
                break;
        }
    }

    if (ping_request == INVALID_PCT_REQUEST){
        ping_request = PCT_STATUS_REQUEST;
    }

    if (adminOops){
	admin_options_usage();
	return -1;
    }

    if (nnodes == 0){
        allNodes = 1;      /* default */
    }
    if (allNodes && (nnodes > 0)){
        printf("Ignoring -a (all nodes), since a node list was provided.\n");
        allNodes = 0;
    }
    if (allNodes){
        start_nid = 0;
        end_nid = MAX_NODES-1;
        nnodes = MAX_NODES;

        for (i=0; i<nnodes; i++){
            node_list[i] = i;
        }
        simple = 1;
    }
    if ((ping_request == PCT_GONE_UPDATE) && 
	((jobNum != INVAL) || mineOnly || (sessionNum != INVAL))){
        printf("Warning -job, -mine, and -pbsjob are ignored with -gone request\n");
    }
    if (summary_only){
	verbose = 0;
    }

    if (ping_request == PCT_NICE_KILL_JOB_REQUEST){

        if ((jobNum == INVAL) && !mineOnly && (sessionNum == INVAL) && !allNodes){

	     printf("The \"nicekill\" command may not be issued to only part of a Cplant job.\n");
	     printf("You must kill the whole job or none of it.\n\n");

	     if (nnodes > 0){
	         printf("Although it's possible the nodes on your node list specified an\n");
	         printf("entire Cplant job, we have no way of knowing that.\n\n");   
	     }
	     printf(
	     "Please specify \"-job {job-id}\" or \"-pbsjob {pbs-id}\" or \"-all\" or \"-mine\".\n");
	    return -1;
	}
    }

    return 0;
}


int 
main(int argc, char *argv[])
{
int rc, i;
int bnid, bpid, b1, b2, b3;
int bptl,myptl;
char *nm;
const char *c;
struct group *gr;
int list;
int count;

    Dbglevel     = 0;

/* Register my PPID */
    _my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_PINGD, "pingd");
    if ( _my_ppid == 0 )
    {
        fprintf(stderr, "Can not register myself as PPID=%d\n", PPID_AUTO);
        exit(-1);
    }
    rc = server_library_init();

    rc = get_options(argc, argv);

    if (rc) {
        exit(0);
    }
    if ((c = daemon_timeout())) daemonWaitLimit = atoi(c);

    pw = getpwuid(Euid = geteuid());

    if (!pw){
        printf("Can't determine access to functions (pw)\n");
        exit(0);
    }

    privLevel = 2;   /* ordinary user */

    if (strcmp(pw->pw_name, "root")){

        gr = getgrnam("bebop");
        if (gr != NULL){
	    for (i=0; ; i++){
		nm = gr->gr_mem[i];
		if (!nm) break;
		if (!strcmp(nm, pw->pw_name)){
		    privLevel = 1;            /* user's in group bebop */
		    break;
		}
	    }
	}
    }
    else {
        privLevel = 0;                    /* superuser */
    }

    if ((ping_request == PCT_RESET_REQUEST)     ||
        (ping_request == PCT_INTERRUPT_REQUEST) ||
        (ping_request == PCT_NICE_KILL_JOB_REQUEST)  ) { 

        if (privLevel > 1){
            printf("Warning, you will only be able to %s compute nodes\n",
             ((ping_request == PCT_RESET_REQUEST) ? "reset" : "interrupt jobs on"));
            printf("that are running your own jobs.\n");
        }
    }

    if ((ping_request == PCT_DIE_REQUEST) || (ping_request == PCT_GONE_UPDATE) ||
        (ping_request == PCT_RESERVE_REQUEST)  || 
        (ping_request == PCT_UNRESERVE_REQUEST)  || 
        (ping_request == BEBOPD_PBS_SUPPORT_ON)  || 
        (ping_request == BEBOPD_PBS_SUPPORT_OFF) || 
	(ping_request == BEBOPD_PBS_UPDATE_ON)   ||
	(ping_request == BEBOPD_PBS_UPDATE_OFF)  ||
	(ping_request == BEBOPD_PBS_LIST)  ||
	(ping_request == BEBOPD_PBS_INTERACTIVE) ){

        if (privLevel > 1){
            printf("Access denied to this function.\n");
            exit(0);
        }
    }
    if ((ping_request == PCT_DIE_REQUEST) || 
        (ping_request == PCT_RESET_REQUEST) ||
        (ping_request == PCT_INTERRUPT_REQUEST) ||
        (ping_request == PCT_NICE_KILL_JOB_REQUEST) ||
        (ping_request == PCT_RESERVE_REQUEST) ||
        (ping_request == PCT_GONE_UPDATE)){

        settings();

        if (!noinquire){

            if (ping_request == PCT_RESERVE_REQUEST) node_reservation_warning();

            printf("Are you sure? (^C to interrupt)\n");
            getchar();
        }
    }

/* Get pnid of bebopd */

    if (altBnid == SRVR_INVAL_NID){

        if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK )
        {
            fprintf(stderr, "Can not get nid from cplant-host file\n");
            exit(-1);
        }

        if ( count != 1 )
            fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = ((altBnid == SRVR_INVAL_NID) ? list : altBnid);
    bpid = ((altBpid == SRVR_INVAL_PID) ? PPID_BEBOPD : altBpid);

    b1 = REQUEST_PTL;
    b2 = UPDATE_PTL;
    b3 = PING_PTL;

    bptl = (int)b3;

    /*
    ** Requests which require data from the bebopd need a control portal
    ** in which to receive the PUT request from the bebopd.  Requests
    ** requiring bebopd to get a node list from us need a control portal
    ** in which to receive the node list GET request.
    */
    if ((ping_request == PCT_STATUS_REQUEST) ||
        (ping_request == PCT_FAST_STATUS)    ||
        (ping_request == PCT_SCAN)    ||
	(pbs_nnodes && !pbs_simple)   ||
        (!simple)    ){

        myptl = srvr_init_control_ptl(2);

        if (myptl == SRVR_INVAL_PTL){
            printf("Can't open portal to receive message from bebopd\n");
            exit(0);
        }
    }        
    else{
        myptl = SRVR_INVAL_PTL;
    }

    /*
    ** Send request to bebopd.  
    */
    rc = send_request(bnid, bpid, bptl, myptl);

    if (rc < 0){
        exit(0);
    }

    /*
    ** Send list of nodes to bebopd.
    */
    if (!simple || (pbs_nnodes && !pbs_simple)){
        rc = provide_node_list(myptl);

        if (rc < 0){
            exit(0);
        }
    }

    /*
    ** Requests that require a bebopd response.
    */
    if ((ping_request == PCT_STATUS_REQUEST)||
        (ping_request == PCT_FAST_STATUS)   ||
        (ping_request == PCT_SCAN))            {

        query_pcts(myptl);
    }
    return 0;
}
static int
send_request(int bnid, int bpid, int bptl, int myptl)
{
int rc;
ping_req req;

    memset((char *)&req, 0, sizeof(ping_req));

    if (adminOp){   
	if (ping_request == BEBOPD_PBS_INTERACTIVE){
	    req.args.nid1 = interactiveNodes;
	}
	else if (ping_request == BEBOPD_PBS_LIST){

	    if (pbs_simple){
	        req.args.nid1 = start_pbs_nid;
	        req.args.nid2 = end_pbs_nid;
	    }
	    else{
	        req.args.nid1 = -1;
		req.args.nid2 = pbs_nnodes;
	    }
	}
    }
    else if (simple){
        req.args.nid1 = start_nid;  /* restrict op to nodes in this range */
        req.args.nid2 = end_nid;
    }
    else{
        req.args.nid1 = -1;        /* bebopd will pull node list */
        req.args.nid2 = nnodes; 
    }

    req.args.jobID = jobNum; /* restrict op to nodes running job ID # jobNum */
                             /* if jobNum==INVAL, it is ignored         */

    req.args.sessionID = sessionNum; /* restrict op to nodes this PBS job      */
                                     /* sessionNum==INVAL ->it is ignored */

    req.args.euid  = 
    ((privLevel > 1) ? 
        Euid :    /* ordinary user also restricted to nodes running his jobs */
   (mineOnly ? 
        Euid :  /* special user wishes to restrict to nodes running his jobs */
          -1)); /* special user wishes no such restriction */

    req.pingPtl = myptl;

    if (ping_request == PCT_RESERVE_REQUEST) req.args.reserve_uid = rUid;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, ping_request,
                                (char *)&req, sizeof(ping_req));

    if (rc < 0){
        printf("(%s) sending request to bebopd\n",CPstrerror(CPerrno));
        return -1;
    }

    return 0;
}
static int
provide_node_list(int myptl)
{
int rc;
control_msg_handle mhandle;
time_t t1;

    t1 = time(NULL);

    while (1){

        SRVR_CLEAR_HANDLE(mhandle); 

        rc = srvr_get_next_control_msg(myptl, &mhandle, NULL, NULL, NULL);

        if (rc == 1) break;

        if (rc < 0){
            printf("(%s) - getting bebopd request for node list\n",
                         CPstrerror(CPerrno));
            return -1;
        } 

        if ((time(NULL) - t1) > daemonWaitLimit){
            printf("No reponse from bebopd.\n");
            return -1;
        }
    }

    if (pbs_nnodes && !pbs_simple){
	rc = srvr_comm_get_reply(&mhandle, (void *)pbs_list,
				     pbs_nnodes * sizeof(int));
    }
    else {
	rc = srvr_comm_get_reply(&mhandle, (void *)node_list,
				     nnodes * sizeof(int));
    }

    if (rc < 0){
        printf("(%s) - error sending node list to bebopd\n",
                         CPstrerror(CPerrno));
        return -1;
    }

    srvr_free_control_msg(myptl, &mhandle);

    return 0;
}

#ifndef STRIPDOWN
static void
query_pcts(int myptl)
{
int rsize, xcount, bcount, dcount, ucount, rc, i, myjob;
int freeCount, pbsRunning, pbsPending, intRunning, intPending;
int dyingScavengers, runningScavengers;
int intLowPriorityAvailable;
int runningJob, pendingJob;
int pcts_recd, avail;
control_msg_handle mhandle;
time_t t1;
pct_ID *pstatus;
pingd_summary *ps;
int reserved_free, reserved_busy, reserved_other;

    printf("Awaiting status from bebopd...\n");

    rsize = sizeof(pct_rec) * nnodes;  /* upper bound on msg from bebopd */

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle); 

        rc = srvr_get_next_control_msg(myptl, &mhandle, NULL, NULL, (char **)&ps);

        if (rc == 1) break;

        if (rc < 0){
            printf("(%s) - getting bebopd notice of data arrival\n",
                         CPstrerror(CPerrno));
            return;
        } 

#ifdef RELIABLE
        if ((time(NULL) - t1) > daemonWaitLimit) {
            printf("No response with data from bebopd in %d seconds.\n", daemonWaitLimit);
#else
        if ((time(NULL) - t1) > WaitPCTstatus) {
            printf("No reponse with data from bebopd.\n");
#endif
            return;
        }
    }

    if (ps->nodes == 0){
	printf("There are no active compute nodes at this time.\n");
        srvr_free_control_msg(myptl, &mhandle);
	return;
    }
    printf("Awaiting pct list from bebopd\n");

    /*
    ** NOTE: bebopd will send some subset of the data we asked for.
    ** It doesn't send data for pcts on nodes it has never received
    ** updates from.
    ** This may mean it sends no pct statuses, or less than nnodes.
    */
    rc = srvr_comm_put_reply(&mhandle, (void *)pcts, rsize);

    if (rc < 0){
      printf("(%s) - receiving data from bebopd\n", CPstrerror(CPerrno));
      srvr_free_control_msg(myptl, &mhandle);
      return;
    }
    pcts_recd = (SRVR_HANDLE_TRANSFER_LEN(mhandle)) / sizeof(pct_rec);

    xcount=bcount=dcount=ucount=0;
    reserved_free=reserved_other=reserved_busy=0;
    freeCount = pbsRunning = pbsPending = intRunning = intPending = 0;
    dyingScavengers = runningScavengers = 0;
    intLowPriorityAvailable = 0;

    if (!summary_only){
      if ( parseable ) {
	display_pct_ID_title_parse();
      }
      else{
	display_pct_ID_title(ps->reserved);
      }
    }

    for (i=0; i< pcts_recd; i++){

        pstatus = &(pcts[i].status);

	runningJob = (pstatus->job_id != INVAL)      ? pstatus->job_id      : 0;
	pendingJob = (pstatus->pending_jid != INVAL) ? pstatus->pending_jid : 0;

        myjob = (runningJob && (pstatus->euid == Euid));

        if ((!mineOnly || myjob) &&
            ((jobNum==INVAL) || (pstatus->job_id == jobNum)) &&
            ((sessionNum==INVAL) || 
            (pstatus->session_id == sessionNum))){

            dcount++;
            if (pstatus->status == STATUS_UNREPORTED) ucount++;
            if (pstatus->status == STATUS_DOWN) xcount++;
            if (pstatus->status == STATUS_TROUBLE) xcount++;

            if (pstatus->status == STATUS_BUSY){

	        bcount++;

		if (pstatus->session_id == INVAL){

		    intRunning++;

		    if ((pstatus->user_status & NICE_KILL_JOB_STARTED) &&
		        !pendingJob){

			/* Killing current job and not yet allocated 
                        ** to a new job.
			*/
			intLowPriorityAvailable++;
                    }
		    else if ( !(pstatus->user_status & NICE_KILL_JOB_STARTED) &&
		               (pstatus->priority == SCAVENGER)              ){

			/*
			** Running a low priority "yod -nice" job
			*/
                        intLowPriorityAvailable++;
                    }

		}
		else{
		    pbsRunning++;

		    if (pstatus->priority == SCAVENGER){
		        if (pstatus->user_status & NICE_KILL_JOB_STARTED){
			    dyingScavengers++;
			}
			else{
			    runningScavengers++;
			}
		    }
		}
	    }
	    else if ((pstatus->status == STATUS_FREE) && pendingJob){

		if (pstatus->pending_sid != INVAL){
		    pbsPending++;
		}
                else{
		    intPending++;
		}
	    }

	    if (pstatus->reserved != INVAL){

	        if (pstatus->status == STATUS_FREE) reserved_free++;
	        else if (pstatus->status == STATUS_BUSY) reserved_busy++;
		else reserved_other++;
	    }

            if (!summary_only) {
              if ( parseable ) {
                display_pct_ID_parse(&pcts[i]);
              }
              else {
                display_pct_ID(&pcts[i]);
              }
            }


            if (verbose && (myjob || (privLevel < 2)) && runningJob){
                print_child_status_string(pstatus->user_status);

		if (pstatus->user_status & NICE_KILL_JOB_STARTED){
		    print_nice_kill_status(pstatus->niceKillCountdown);
		}

                print_child_exit_code(pstatus->final);
                printf("\n");
            }
        }
    }
    printf("\nTotal: %d\n",dcount);

    printf("Total busy: %d\n",bcount);

    if (intPending || pbsPending){
        printf("Total pending allocation: %d (check again soon)\n",
	        intPending + pbsPending);
    }

    freeCount = dcount-bcount-ucount-xcount-intPending-pbsPending;
    
    printf("Total free: %d\n",freeCount);
    
    if (ucount){
	printf("Total not responding to ping (try again): %d\n",ucount);
    }
    if (xcount){
	printf("Total nodes unavailable: %d\n",xcount);
    }


    /*
    ** This information only makes sense if user ran pingd for all
    ** nodes out there, not a subset.
    */
    if (displayAllNodes && (ps->pbsSupportLevel > 0)){

	printf("\n");
	if (ps->pbsInteractive >0){
	    printf("Compute nodes are being scheduled by PBS, but %d\n",
		      ps->pbsInteractive);
	    printf( "nodes are currently reserved for non-PBS interactive use.\n");
            printf("Interactive nodes: %s\n", ps->ilist);
        }
	else{
	    printf("All compute nodes are being scheduled by PBS.\n");
	}

	printf("\n");

	if (pbsPending){
	    printf("Free nodes pending allocation to PBS jobs: %d\n", 
		    pbsPending);
	}

	printf("Nodes currently hosting PBS jobs:         %d\n", 
		pbsRunning);

	if (runningScavengers || dyingScavengers){
	    printf("   regular jobs:       %4d\n",
		pbsRunning-runningScavengers-dyingScavengers);
	    printf("   running scavengers: %4d\n",runningScavengers);
	    printf("   dying scavengers:   %4d\n",dyingScavengers);
        }

	if (runningScavengers || dyingScavengers){
	   printf(
	   "\n   (Nodes allocated to a regular priority PBS job but not being used by it may be\n"
	   "   allocated to a low priority \"scavenger\" PBS job.  These low priority jobs are\n"
	   "   killed when a regular PBS job requires the use of it's nodes.)\n");
	}

	avail = ps->pbsInteractive -                     /* nodes set aside for interactive use */
	   (intRunning + intPending - ps->intOutliers) - /* less those running interactive jobs */
	   ps->pbsOutliers;                                      /* less those running PBS jobs */

	if (intRunning || intPending || ps->pbsInteractive){

	    printf("\n");

            if (intPending){
		printf("Free nodes pending allocation to interactive jobs: %d\n", 
			intPending);
	    }
	    printf("Nodes currently hosting interactive jobs: %d\n", 
		       intRunning);

	    printf("Free nodes remaining for interactive jobs: %d\n", avail);

	    /*
	    ** An administrator may start up PBS when there are interactive
	    ** jobs on the machine.  We let them finish, and then PBS
	    ** takes the nodes.  Or an administrator may reduce the nodes
	    ** allocated to interactive use while interactive jobs are running.
	    */
	    
	    if (ps->pbsOutliers){
	        printf("(%d interactive nodes are being used by PBS jobs.  These nodes will\n",
		 ps->pbsOutliers);
	        printf("be reclaimed for interactive use when the PBS jobs complete.)\n");
	    }
	    if (ps->intOutliers){
	        printf("(%d PBS nodes are being used by interactive jobs.  These nodes will\n",
		   ps->intOutliers);
	        printf("be reclaimed for PBS use when the interactive jobs complete.)\n");
	    }
	}
	printf("\n");
    }

    if (intLowPriorityAvailable){
        printf("Interactive node report:\n");
	printf(
"    %d of the nodes that are busy running interactive jobs are actually\n"
"    available for allocation to regular priority interactive jobs.  These\n" 
"    nodes are running jobs that are in the process of being killed, or they\n"
"    are running low priority jobs that will be killed if a regular priority\n"
"    interactive job requests the nodes.\n\n"
"    A job is a regular priority job if it is NOT started with the \"-nice\"\n"
"    option to yod.\n", intLowPriorityAvailable);
    }

    if (reserved_free || reserved_busy || reserved_other){

        printf("\n");
	if (reserved_free){
	    printf(
	    "Total free nodes reserved by system administrators: %d\n",reserved_free);
	}
	if (reserved_busy){
	    printf(
	    "Total busy nodes reserved by system administrators: %d\n",reserved_busy);
	}
	if (reserved_other){
	    printf(
	    "Total unavailable nodes reserved by system administrators: %d\n",
		     reserved_other);
	}
        printf(
	"(Nodes may be reserved by administrators when they don't wish jobs to be\n"
	"loaded onto them due to some problem they may have observed and wish to\n"
	"investigate.  The reservation will be released as soon as the problem is\n"
	"resolved.  A job running at the time the node is reserved may be allowed\n"
	"to continue running while the investigation procedes, but no other jobs\n"
	"will be loaded except those owned by the holder of the reservation.)\n");
    }

    srvr_free_control_msg(myptl, &mhandle);
}
#endif /* !STRIPDOWN */

#ifdef STRIPDOWN
static void
query_pcts(int myptl)
{
int rsize, xcount, bcount, dcount, ucount, rc, i, myjob;
int freeCount, pbsRunning, pbsPending, intRunning, intPending;
int intFree, pbsFree, intNodes;
int dyingScavengers, runningScavengers;
int intLowPriorityAvailable;
int runningJob;
int pcts_recd;
control_msg_handle mhandle;
time_t t1;
pct_ID *pstatus;
pingd_summary *ps;
int reserved_free, reserved_busy, reserved_other;

    printf("Awaiting status from bebopd...\n");

    rsize = sizeof(pct_rec) * nnodes;  /* upper bound on msg from bebopd */

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle); 

        rc = srvr_get_next_control_msg(myptl, &mhandle, NULL, NULL, (char **)&ps);

        if (rc == 1) break;

        if (rc < 0){
            printf("(%s) - getting bebopd notice of data arrival\n",
                         CPstrerror(CPerrno));
            return;
        } 

#ifdef RELIABLE
        if ((time(NULL) - t1) > daemonWaitLimit){
            printf("No response with data from bebopd in %d seconds.\n", daemonWaitLimit);
#else
        if ((time(NULL) - t1) > WaitPCTstatus){
            printf("No response with data from bebopd.\n");
#endif
            return;
        }
    }

    if (ps->nodes == 0){
	printf("There are no active compute nodes at this time.\n");
        srvr_free_control_msg(myptl, &mhandle);
	return;
    }
    printf("Awaiting pct list from bebopd\n");

    /*
    ** NOTE: bebopd will send some subset of the data we asked for.
    ** It doesn't send data for pcts on nodes it has never received
    ** updates from.
    ** This may mean it sends no pct statuses, or less than nnodes.
    */
    rc = srvr_comm_put_reply(&mhandle, (void *)pcts, rsize);

    if (rc < 0){
      printf("(%s) - receiving data from bebopd\n", CPstrerror(CPerrno));
      srvr_free_control_msg(myptl, &mhandle);
      return;
    }
    pcts_recd = (SRVR_HANDLE_TRANSFER_LEN(mhandle)) / sizeof(pct_rec);

    xcount=bcount=dcount=ucount=0;
    reserved_free=reserved_other=reserved_busy=0;
    freeCount = pbsRunning = pbsPending = intRunning = intPending = 0;
    pbsFree = intFree = intNodes = 0;
    
    dyingScavengers = runningScavengers = 0;
    intLowPriorityAvailable = 0;

    if (!summary_only){
      if ( parseable ) {
	display_pct_ID_title_parse();
      }
      else{
	display_pct_ID_title(ps->reserved);
      }
    }

#if 1
    for (i=0; i< pcts_recd; i++){
        pstatus = &(pcts[i].status);
        myjob = (runningJob && (pstatus->euid == Euid));
        if ((!mineOnly || myjob) &&
            ((jobNum==INVAL) || (pstatus->job_id == jobNum)) &&
            ((sessionNum==INVAL) || 
            (pstatus->session_id == sessionNum))){

            if (pstatus->status == STATUS_FREE) {
              if (pstatus->nodeType == IACTIVE_NODE) {
                intFree++;
              }
              else {
                pbsFree++; 
              }
            }

            if (pstatus->status == STATUS_BUSY) {
              if (pstatus->nodeType == IACTIVE_NODE) {
                intRunning++;
              }
              else {
                pbsRunning++; 
              }
            }

            if (pstatus->status == STATUS_UNREPORTED) ucount++;
            if (pstatus->status == STATUS_DOWN) xcount++;

            if (!summary_only) {
              if ( parseable ) {
                display_pct_ID_parse(&pcts[i]);
              }
              else {
                display_pct_ID(&pcts[i]);
              }
            }
        }
    }
#endif

    freeCount = intFree + pbsFree;
    bcount = intRunning + pbsRunning;

    dcount = freeCount + bcount;

    intNodes = intFree + intRunning;

    printf("\nTotal: %d\n",dcount);

    printf("Total busy: %d\n",bcount);

    if (intPending || pbsPending){
        printf("Total pending allocation: %d (check again soon)\n",
	        intPending + pbsPending);
    }

    printf("Total free: %d\n",freeCount);
    
    if (ucount){
	printf("Total not responding to ping (try again): %d\n",ucount);
    }
    if (xcount){
	printf("Total nodes unavailable: %d\n",xcount);
    }

    /*
    ** This information only makes sense if user ran pingd for all
    ** nodes out there, not a subset.
    */
    if (displayAllNodes && (ps->pbsSupportLevel > 0)){

	printf("\n");
	if (intNodes >0){
	    printf("Compute nodes are being scheduled by PBS, but %d\n",
		      intNodes);
	    printf( "nodes are currently reserved for non-PBS interactive use.\n");
            printf("Interactive list: %s\n", ps->ilist);
        }
	else{
	    printf("All compute nodes are being scheduled by PBS.\n");
	}

	printf("\n");

	printf("Nodes currently hosting PBS jobs:         %d\n", 
		pbsRunning);
	printf("Free nodes remaining for PBS jobs:         %d\n", 
		pbsFree);

	if (intNodes) {

	    printf("\n");

	    printf("Nodes currently hosting interactive jobs: %d\n", 
		       intRunning);

	    printf("Free nodes remaining for interactive jobs: %d\n", 
                       intFree);

	    /*
	    ** An administrator may start up PBS when there are interactive
	    ** jobs on the machine.  We let them finish, and then PBS
	    ** takes the nodes.  Or an administrator may reduce the nodes
	    ** allocated to interactive use while interactive jobs are running.
	    */
	    
	    if (ps->pbsOutliers){
	        printf("(%d interactive nodes are being used by PBS jobs.  These nodes will\n",
		 ps->pbsOutliers);
	        printf("be reclaimed for interactive use when the PBS jobs complete.)\n");
	    }
	    if (ps->intOutliers){
	        printf("(%d PBS nodes are being used by interactive jobs.  These nodes will\n",
		   ps->intOutliers);
	        printf("be reclaimed for PBS use when the interactive jobs complete.)\n");
	    }
	}
	printf("\n");
    }

    if (reserved_free || reserved_busy || reserved_other){

        printf("\n");
	if (reserved_free){
	    printf(
	    "Total free nodes reserved by system administrators: %d\n",reserved_free);
	}
	if (reserved_busy){
	    printf(
	    "Total busy nodes reserved by system administrators: %d\n",reserved_busy);
	}
	if (reserved_other){
	    printf(
	    "Total unavailable nodes reserved by system administrators: %d\n",
		     reserved_other);
	}
        printf(
	"(Nodes may be reserved by administrators when they don't wish jobs to be\n"
	"loaded onto them due to some problem they may have observed and wish to\n"
	"investigate.  The reservation will be released as soon as the problem is\n"
	"resolved.  A job running at the time the node is reserved may be allowed\n"
	"to continue running while the investigation procedes, but no other jobs\n"
	"will be loaded except those owned by the holder of the reservation.)\n");
    }

    srvr_free_control_msg(myptl, &mhandle);
}
#endif /* STRIPDOWN */

#ifndef STRIPDOWN
static void
print_child_exit_code(final_status final)
{
    if (final.term_sig > 0){
        printf("\t\tterminating signal: %d, %s\n",
                 final.term_sig, select_signal_name(final.term_sig));
    }
    if (final.exit_code > 0){
        printf("\t\tapplication exit code %d\n",final.exit_code);
    }
    if (final.terminator != PCT_NO_TERMINATOR){
       printf("\t\tTerminated by %s.\n",
       (final.terminator == PCT_JOB_OWNER) ?
	  "job owner" : "cplant administrator or PBS daemon");
    }
}
static void
print_nice_kill_status(int seconds)
{
    if (seconds > 0){
        printf("\t\t(process will be killed in approximately %d seconds)\n",seconds);
    }
}
static void
print_child_status_string(UINT16 st)
{
INT32 i; 
unsigned int mask;

    if (st == 0){
        printf("\tno status set for app process\n");
    }
    else{
        for (i=1, mask = 1; i <= 16; i++, mask <<= 1){

            if (st & mask){
                printf("\t\t%s\n",child_status_strings[i]);
            }
        }
    }
}

#endif /* STRIPDOWN */

void display_pct_ID_title_parse(void) {
  printf("\n");
  printf("nid:name:status:reserved:session:job-id:spid:rank:owner:hours:minutes:seconds\n");
  printf("status=free|busy|stale|down|pending|trouble\n");
  printf("reserved=true|false\n");
  printf("-----------------------------------------------------------------------------\n");
}

static char *res0 = "reserve";
static char *res1 = "  for  ";
static char *res2 = "-------";

void display_pct_ID_title(int reservations) {
  int i, offset;

  phys2name(0);
  /* stuff to the right of the name can scooch over if the
     name grows... */ 
  offset = 8+maxnamelen+4;

  printf("\n");

  for (i=0; i<offset; i++) {
    printf(" ");
  }
  printf(
  "                                             ------scavengers--------\n");
  for (i=0; i<offset; i++) {
    printf(" ");
  }
  printf(
  "                                       PBS   |low time   upcoming   |%s\n",
           reservations ? res0 : "");

  printf("      node");
  for (i=0; i<offset-10; i++) {
    printf(" ");
  }

  printf(
  " JobID   SPID/rank  owner   elapsed   session|pri left JobId session|%s\n",
           reservations ? res1 : "");

  for (i=0; i<offset; i++) {
    printf("-");
  }
  printf(
  " ----- ----------- ------- ---------- ------- --- ---- ----- ------- %s\n",
           reservations ? res2 : "");
  
}

void display_pct_ID_parse(pct_rec *pct_stat) {
  struct passwd *pwent;                       
  int hours, minutes, seconds;               
  pct_ID *p;                                
  char *yea = "true";
  char *nea = "false";
  char *reserved = nea;


  p = &(pct_stat->status);                  

  if (p->reserved != -1 ) {
    reserved = yea;
  }
  pwent = getpwuid(p->euid);              
  if (p->elapsed){                       
    hours = p->elapsed / 3600;          
    minutes = ( (p->elapsed % 3600) / 60);
    seconds = p->elapsed % 60;           
  }                         
  else{                    
    hours=minutes=seconds = 0;  
  }                          

  phys2name(0);

  printf("%d:", pct_stat->nid);   
  print_node_name(pct_stat->nid, stdout);
  printf(":");

  if (p->status == STATUS_UNREPORTED) {      
    printf("stale:%s:%d:_:_:_:_:_:_:_\n", reserved, p->session_id);
  } else if (p->status == STATUS_FREE && (p->pending_jid != INVAL)) {
    printf("pending:%s:%d:_:_:_:_:_:_:_\n", reserved, p->session_id);
  } else if (p->status == STATUS_BUSY){                    
    printf("busy:%s:%d:%d:%d:%d:%s:%d:%d:%d\n", reserved,
                                           p->session_id,
                           p->job_id, p->job_pid, p->rank,
      ((pwent && pwent->pw_name) ? pwent->pw_name : "none"),
                                   hours, minutes, seconds);
  } else if (p->status == STATUS_DOWN){      
    printf("down:%s:%d:_:_:_:_:_:_:_\n", reserved, p->session_id);
  } else if (p->status == STATUS_TROUBLE){      
    printf("trouble:%s:%d:_:_:_:_:_:_:_\n", reserved, p->session_id);
  } else if (p->status == STATUS_FREE){      
    printf("free:%s:%d:_:_:_:_:_:_:_\n", reserved, p->session_id);
  }
}

void display_pct_ID(pct_rec *pct_stat) {
  struct passwd *pwent;                       
  int hours, minutes, seconds;               
  int i, len;
  pct_ID *p;                                
  char name7[8];

  p = &(pct_stat->status);                  
  pwent = getpwuid(p->euid);              
  if (p->elapsed){                       
    hours = p->elapsed / 3600;          
    minutes = ( (p->elapsed % 3600) / 60);
    seconds = p->elapsed % 60;           
  }                         
  else{                    
    hours=minutes=seconds = 0;  
  }                          
  printf("%5d ( ", pct_stat->nid);   
  len = print_node_name(pct_stat->nid, stdout);
  printf(" )");
  for (i=0; i< (maxnamelen-len+3); i++) {
    printf(" ");
  }

  if (p->status == STATUS_UNREPORTED) {      
    printf("node data is stale\n");      
    return;
  } else if (p->status == STATUS_DOWN){      
    printf("node is unavailable\n");   
    return;
  } else if (p->status == STATUS_TROUBLE){      
    printf("node reports problems found\n");   
    return;
  }

  if (p->status == STATUS_BUSY){

    if (pwent){
        strncpy(name7, pwent->pw_name, 7);
	name7[7]=0;
    }
    else{
        strcpy(name7,"unknown");
    }

    printf("%5d %6d/%4d %7s %4d:%02d:%02d",
		 p->job_id, p->job_pid, p->rank, name7,
			       hours, minutes, seconds); 
  
    if (p->session_id != INVAL){
        printf(" %7d",p->session_id);
    }
    else{
        printf("        ");
    }

    if (p->priority == SCAVENGER){
        printf("  * ");
    }
    else{
        printf("    ");
    }

    if (p->niceKillCountdown){
        printf(" %4d",p->niceKillCountdown);
    }
    else{
        printf("     ");
    }
  }
  else if (p->status == STATUS_FREE || p->status == STATUS_ALLOCATED) {
      for (i=0; i<53; i++) {
          printf(" ");
      }
  }

  if (p->pending_jid != INVAL){
      printf(" %5d", p->pending_jid);    
  }
  else{
      printf("      ");
  }
  if (p->pending_sid != INVAL){
      printf(" %7d", p->pending_sid);    
  }
  else{
      printf("        ");
  }

  if (p->reserved != INVAL){
    pwent = getpwuid(p->reserved);

    if (pwent){
        strncpy(name7, pwent->pw_name, 7);
	name7[7]=0;
    }
    else{
        strcpy(name7,"unknown");
    }
    printf(" %s",name7);
  }

  printf("\n");
}
