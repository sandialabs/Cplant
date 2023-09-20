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
** $Id: yod_data.h,v 1.3 2001/11/02 17:25:30 lafisk Exp $
*/
#ifndef YOD_DATAH
#define YOD_DATAH

#include "appload_msg.h"

#define NO_PBS_JOB  0
#define PBS_BATCH   1
#define PBS_INTERACTIVE 2

#define NRETRIES   3

/*
** What is a "load member":  users can run yod with the name of an executable,
** or with a load file.  Each line of the load file is a "load member".  It
** specifies an executable name and program arguments (and may specify a "-sz" 
** or "-list" argument).  Some of the executables may be the same, but with 
** different command line arguments.  
*/
typedef struct _loadMembers {
    loadMemberData data;         /* see appload_msg.h */

    /*
    ** executable path and info - some lines in load file may refer to the
    ** same executable.  We only want to read it in once.
    */
    char *pname;      /* pathname of executable */
    int pnameCount;   /* first entry counts number of members requiring this executable */
    int pnameSameAs;  /* earlier member with same executable as me, or -1 */
    char *exec;                /* executable file in memory */
    unsigned char exec_check;  /* check sum on executable   */
    char *execPath;    /* executable copied to a location where PCT can read it in */

    int nargs;
    char **args;
    char *argstr;
    char *local_ndlist;
    int listsize;
    int local_size;
    int send_or_copy;

    char *loadFile;
}loadMembers;

typedef struct _launchErrors{
    int errType;
    int failedNid;
    int failedPtl;
    char failedOp;
    char reportingStatus;
} launchErrors;

int total_compute_nodes(void);
int total_unspecified_nodes(void);
int total_members(void);
loadMembers *member_data(int which);
loadMembers *which_member(int rank);


void display_members(void);

int determine_members(int argc, char **argv, int pbs_job,
		   int global_size, char *global_node_list, int global_listsize,
		      uid_t euid, uid_t egid, char *cwd);
int pack_up_env(char **epack, char *envp[], char *cwd);
char *find_in_cwd(char *fname,char *cwd);
char *find_in_path(char *fname);
char *real_path_name(char *fname);


/*
** All messages from yod to stderr/stdout should use these macros.  
** This is because "yod -quiet" requests that yod display nothing but
** output from the parallel application.
*/
extern int quiet;
extern int daemonWaitLimit;

extern int pbs_job;
extern int priority;
extern int retryLoad;

extern int physnid2rank[];
extern int rank2physnid[];
extern int NumProcs;
extern char msgbuf[];

#if 0
#define yodmsg(format, args...) \
    if (!quiet) printf(format, ## args)
#define yoderrmsg(format, args...) \
    if (!quiet) fprintf(stderr, format, ## args)
#else
void yodmsg(char* format, ...); 
void yoderrmsg(char* format, ...); 
#endif

extern gid_t groupList[];
extern int   ngroups;
extern  char fileName[];

#endif

