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
** $Id: yod_data.c,v 1.23 2001/11/17 01:19:19 lafisk Exp $
**
** The yod command line contains a file name.  It is either the name
** of the executable to be run on the compute partition, or the name
** of a text load file listing the names of executables.
**
**   yod {yod-args} executable-name {executable-args}
** 
**                 OR
**
**   yod {yod-args} load-file-name
**
** Important note:
**   Yod distinguishes an "executable-name" from a "load-file-name" based 
**   on the file permission bits.
**       An executable-file must have execute permission.
**       A load-file MAY NOT have execute permission!
**
** The load file has the following format:
**
**   # comments like this are skipped
**   #
**   {-sz n1} {-l node_list1} executable-name-1 {arg-list-1}
**   {-sz n2} {-l node_list2} executable-name-2 {arg-list-2}
**   #
**
** If yod was invoked from a PBS session, any "-l" argument in the load file
** will be ignored.  You don't get to choose the exact nodes you run on.
**
** In general, if a node_list is not given:
**    If the "-sz" argument is omitted, the number of nodes is taken to be "1". 
**    Otherwise, yod attempts to load the executable on the specified number
**    of nodes.
**
** If a node_list is given:
**    If a "-sz n" argument is given, yod attempts to allocate the first
**    n free nodes from the list and will fail if it cannot.
**
**    If no "-sz" argument is given, yod will attempt to allocate all nodes
**    in the node list to the executable, and will fail if it cannot.
**      
*/
#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/param.h>
#include <errno.h>
#include "srvr_err.h"
#include "appload_msg.h"
#include "config.h"
#include "yod_data.h"
#include "util.h"

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

#define IS_EXECUTABLE   1
#define IS_REGULAR      2
#define IS_NOGOOD       3

extern void load_file_lines(void);

static int check_permissions(char *fname, uid_t euid, uid_t egid);
static int pack_up_args(int which);
#if 0
static void display_chars(unsigned char *c, int len, int width);
#endif
static int read_load_file(char *fname, int pbs_job, uid_t euid, uid_t egid,char *cwd);
static int determine_partitioning(int pbs_job, int global_size, 
	      char *global_node_list, int global_listsize);

static int nMembers=0;      /* number of load file entries */
static int totalnodes=0;    /* total compute nodes required */
static int tnodesany=0;     /* total nodes with no local node list specified */

static loadMembers *Members;    /* array of load file entries */

char fileName[MAXPATHLEN]="unknown";

gid_t groupList[NGROUPS_MAX];
int   ngroups = -1;

#define ISBLANK(c) (!isgraph(c))
int
total_compute_nodes()
{
    return totalnodes;
}
int
total_unspecified_nodes()
{
    return tnodesany;
}
int
total_members()
{
    return nMembers;
}
loadMembers *
member_data(int which)
{
struct _loadMembers *mbr;

    if ((which < 0) || (which >= nMembers)){
	mbr = NULL;
    }
    else{
        mbr = Members + which;
    }
    return mbr;
}
loadMembers *
which_member(int rank)
{
struct _loadMembers *mbr;
int i;

    mbr = NULL;

    for (i=0; i<nMembers; i++){
	if ( (rank >= Members[i].data.fromRank) &&
	     (rank <= Members[i].data.toRank)       ){

             mbr = Members + i;
	     break;
	}
    }
    return mbr;
}
int
sameExecutable(int member, char *pname)
{
int i;

    if (!pname) return -1;

    for (i=0; i<member; i++){

        if (Members[i].pname &&
	    (strcmp(Members[i].pname, pname) == 0)){

	    return i;

	}
    }
    return -1;
}
void
initMember(loadMembers *mbr)
{
    mbr->data.fromRank  = mbr->data.toRank = -1;
    mbr->data.argbuflen = mbr->data.execlen = 0;

    mbr->pname       = NULL;
    mbr->pnameCount  = 0;
    mbr->pnameSameAs = -1;
    mbr->exec        = NULL;
    mbr->exec_check  = 0;
    mbr->execPath    = NULL;

    mbr->nargs        = 0;
    mbr->args         = (char **)NULL;
    mbr->argstr       = NULL;
    mbr->local_ndlist = NULL;
    mbr->listsize     = 0;
    mbr->local_size   = 0;
    mbr->send_or_copy = INVALID_MSG;
    mbr->loadFile     = NULL;
}
void
display_members()
{
int i,ii;

    for (i=0; i<nMembers; i++){
	if (Members[i].pname){
	    yodmsg("%s\n",Members[i].pname);
        }
        if (Members[i].pnameSameAs > -1){
	    yodmsg("Program name same as member # %d\n",Members[i].pnameSameAs);
	}

	yodmsg("size %d, ranks %d - %d, arg length %d exec length %d\n",
	   Members[i].local_size,
	   Members[i].data.fromRank, Members[i].data.toRank,
	   Members[i].data.argbuflen, Members[i].data.execlen);

        if (Members[i].local_ndlist)
	    yodmsg("node list %s\n", Members[i].local_ndlist);

        if (Members[i].nargs > 1){
	    yodmsg("%d args: ",Members[i].nargs);
	    for (ii=0; ii<Members[i].nargs; ii++)
		yodmsg(" %s ",Members[i].args[ii]);
	    yodmsg("\n");
        }
	if (Members[i].loadFile) printf("From load file: \"%s\"\n\n",Members[i].loadFile);
    }
}

/*
** parse the end of the yod command line, including executable name or
** load file name, and parse load file if given.
**
** Return number of "members", that is number of distinct command lines for the load.
*/
int
determine_members(int argc, char **argv, int pbs_job, 
		   int global_size, char *global_node_list, int global_listsize,
		   uid_t euid, uid_t egid, char *cwd)
{
int load_file, rc, i;
struct stat statbuf;
char *c;

    nMembers = 0;

    if (argv[0] == NULL){
        yoderrmsg(" No load file name or program name\n");
	return nMembers;
    }

    if (argv[0][0] != '/'){             

        if ((c = find_in_cwd(argv[0], cwd)) == NULL){

	    c = find_in_path(argv[0]);
	}
    }
    else{
        c = real_path_name(argv[0]);
    }

    if (c == NULL){
         yoderrmsg("Unable to find %s in current working directory or in your PATH\n",
	                   argv[0]);
	 return -1;
    }

    strcpy(fileName, c);

    load_file = check_permissions(fileName, euid, egid);

    if (load_file == IS_EXECUTABLE){

	nMembers = 1;

        Members = (loadMembers *)malloc(sizeof (loadMembers));

        if (!Members){
            CPerrno = ENOMEM;
            return -1;
        }
	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: %p (%u) for load information\n",
		   Members, sizeof(loadMembers));
	}

	initMember(Members);

	Members[0].nargs = argc;

	if (Members[0].nargs){

	    Members[0].args = 
	        (char **)malloc(Members[0].nargs * sizeof(char *));

	    if (!Members[0].args){
		CPerrno = ENOMEM;
		return -1;
	    }
            if (DBG_FLAGS(DBG_MEMORY)){
		yodmsg("memory: %p (%u) for argument data\n",
		       Members[0].args,
		       (unsigned int)(Members[0].nargs * sizeof(char *)));
	    }
	    for (i=0; i<argc; i++){
                Members[0].args[i] = argv[i];
	    }
	}
	Members[0].pname = fileName;

	if (Members[0].pname == NULL){
	    return -1;
	}
	rc = stat(Members[0].pname, &statbuf);

	if (rc == ENOENT){
	    yoderrmsg("Can't stat %s\n",Members[0].pname);
	    return 0;
	}

	Members[0].data.execlen = statbuf.st_size;

	Members[0].pnameCount  = 1;
	Members[0].pnameSameAs = -1;

    }
    else if (load_file == IS_REGULAR){

        nMembers = read_load_file(fileName, pbs_job, euid, egid, cwd);
	if (nMembers < 1){
	    load_file_lines();
	}

    }
    else {
        yoderrmsg("File access denied\n");
    }

    /*
    ** For each member, compute number of nodes on which to run,
    ** and from_rank and to_rank.
    */

    if (nMembers == 1){                        /* easy case */
	
	if (Members[0].local_size == 0){
	    if (Members[0].listsize > 0){
		Members[0].local_size = Members[0].listsize;
	    }
	    else if (global_size > 0){
		Members[0].local_size = global_size;
	    }
	    else if (global_listsize > 0){
		Members[0].local_size = global_listsize;
	    }
	    else{
		Members[0].local_size = 1;
	    }
	}
	if ((Members[0].listsize > 0) &&
	    (Members[0].local_size > Members[0].listsize)){

	    yoderrmsg("Request of %d nodes exceeds the %d nodes in %s\n",
		Members[0].local_size,
		Members[0].listsize,
		Members[0].local_ndlist);
            return 0;
	}
        
	Members[0].data.fromRank = 0;
	Members[0].data.toRank   = Members[0].local_size - 1;

	totalnodes = Members[0].local_size;

	if (Members[0].local_ndlist == NULL){
	    tnodesany = Members[0].local_size;
	}
    }
    else if (nMembers > 1) {                  /* heterogeneous load */

	rc = determine_partitioning(pbs_job, 
		      global_size, global_node_list, global_listsize);

	if (rc < 0){
            yoderrmsg( "Heterogenous load Failed \n");
	    nMembers = 0;
	}

	if ((pbs_job != NO_PBS_JOB) && (totalnodes != tnodesany)){
            yoderrmsg( "Your PBS job appears to have specified a node list.\n");
            yoderrmsg( "PBS jobs may not request specific nodes.\n");
            yoderrmsg( "Re-run your job using \"-sz\" to request nodes.\n");
	    nMembers = 0;
	}
    }

    for (i=0; i<nMembers; i++){
        rc = pack_up_args(i);

	if (rc < 0){
            yoderrmsg( " PACK_UP_ARGS failed\n");
	    nMembers = 0;
	}
    }

    return nMembers;
}
/*
** This is the complicated part.
*/
static int
determine_partitioning(int pbs_job ATTR_UNUSED, int global_size,
                       char *global_node_list, int global_listsize)
{
int i, rank;

    if (nMembers <= 1){
	return -1;
    }
    if (global_size > 0){
	yodmsg( "WARNING:%c\n", BELL);
	yodmsg(
	"The size argument of %d on the yod command line will be ignored.  The\n",
	global_size);
	yodmsg(
	"arguments in the load file will be used.  (If a line in the load file\n");
	yodmsg(
	"does not specify a size, then the size of the node list specified in\n");
	yodmsg(
	"load file will be used, or, if no node list is specified, then the\n");
	yodmsg("executable will be run on 1 node.)\n");
    }
    /*
    ** Each line in the load file can specify a node list.  
    ** There also can be a global node list specified as
    ** a yod argument.  Use this global list if there is no list
    ** specified in the load file.
    */
    for (i=0, totalnodes=0, tnodesany=0; i<nMembers; i++){
	if (Members[i].local_size == 0){
	    if (Members[i].listsize > 0)
		Members[i].local_size = Members[i].listsize;
            else
		Members[i].local_size = 1;
	}

	totalnodes += Members[i].local_size;

	if (Members[i].local_ndlist == NULL){
	    tnodesany += Members[i].local_size;
	}
    }

    if (global_node_list && (global_listsize < tnodesany)){
	yoderrmsg("The node list %s, which lists %d nodes,\n",
	       global_node_list, global_listsize);
        yoderrmsg("is too small to contain the %d nodes required by your\n",
	      tnodesany);
        yoderrmsg("load file entries.\n");
        return -1;
    }
    for (i=0, rank=0; i<nMembers; i++){
	Members[i].data.fromRank = rank;
	rank += Members[i].local_size;
	Members[i].data.toRank = rank-1;
    }

    return 0;
}
/*
** Read in load file, write loadMember structure and return number
** of executables.  Returns 0 if can't read or parse load file. 
** Returns -1 on malloc or other resource error.
*/
static char *load_file_text;

/*
** Please list all valid yod arguments that can appear before the
** executable name in the load file.  We'll accept these or prefixes
** of these.
*/
#define ARG_STR_COUNT 3
#define REQUIRES_VAL  1   /* this yod argument requires a value */
#define NO_ARG        2   /* this yod argument does not         */

static struct{
   const char *arg;
   int val;
}yodArgs[ARG_STR_COUNT] = 
    {
    {"size", REQUIRES_VAL},
    {"sz", REQUIRES_VAL},
    {"list", REQUIRES_VAL}
    };

static int
get_yod_arg(char *c)
{
int i, num = -1;
const char *c2;

    for (i=0; i<ARG_STR_COUNT; i++){
	c2 = yodArgs[i].arg;
	while (*c && *c2 && !ISBLANK((int)*c) && (*c++ == *c2++));
	if ( (*c == 0) || ISBLANK(*c)){
	    num = i;
	    break;
	}
    }
    return num;
}

#define GET_INT_VAL(ptr, val, line) \
    ptr += 2;                              \
    while (isalpha((int)*ptr)) ptr++;      \
    while (ISBLANK((int)*ptr)) ptr++;      \
    if ((*ptr == 0) || !(isdigit((int)*ptr))){                   \
	yoderrmsg( "can't parse this line in load file\n"); \
	yoderrmsg( "%s\n",line);                            \
	return 0;                          \
    }                                      \
    val = atoi(ptr);

#define GET_STRING_VAL(ptr, val, line) \
    ptr += 2;                          \
    while (isalpha((int)*ptr)) ptr++;  \
    while (ISBLANK((int)*ptr)) ptr++;  \
    if (*ptr == 0){                    \
	yoderrmsg( "can't parse this line in load file\n"); \
	yoderrmsg( "%s\n",line);                            \
	return 0;                      \
    }                                  \
    val = ptr;                         \
    while (!ISBLANK((int)*ptr)) ptr++; \
    *ptr = 0;

static int
read_load_file(char *fname, int pbs_job, uid_t euid, uid_t egid, char *cwd)
{
struct stat statbuf;
int i, ii, fsize, rc, fd, sz, argnum, nlines, sameAs;
char *local_ndlist;
char *c, *endbuf, *yodargs, *pname, *pargs;

    /*
    ** open load file and read into buffer
    */
    rc = stat(fname, &statbuf);

    if (rc == ENOENT){
	yoderrmsg("Can't stat %s\n",fname);
	return 0;
    }

    fsize = statbuf.st_size;

    load_file_text = (char *)malloc(fsize+1);

    if (!load_file_text){
	CPerrno = ENOMEM;
	return -1;
    }

    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) for load file\n",load_file_text,fsize+1);
    }
    fd = open(fname, O_RDONLY);

    if (fd < 0){
	yoderrmsg("Can't open %s\n",fname);
	return -1;
    }
    rc = read(fd, load_file_text, fsize);

    if (rc != fsize){
	yoderrmsg("Can't read %d bytes of %s, rc %d\n",fsize, fname, rc);
	return -1;
    }
    close(fd);

    load_file_text[fsize] = 0;

    endbuf = load_file_text + fsize;

    /*
    ** Null terminate each line in the load file
    */
    c = load_file_text;
    nlines = 0;

    while (c < endbuf){

	if (*c == '\n'){

	    *c = 0;
	    nlines++;     /* upper bound on number of load file entries */
	}
	c++;
    }
    nlines++;    /* maybe last entry wasn't followed by a new line */

    c = load_file_text;
    nMembers = 0; 

    Members = (loadMembers *)malloc(nlines * sizeof (loadMembers));

    if (!Members){
        CPerrno = ENOMEM;
        return -1;
    }
    if (DBG_FLAGS(DBG_MEMORY)){
        yodmsg("memory: 0x%p (%u) for load file member records\n",Members,
	            nlines * sizeof (loadMembers));
    }
    
    for (i=0; i<nlines; i++){
	initMember(Members + i);
    }

    /*
    ** Identify the lines in the load file that list executables
    */
    while (1){

        while ( ((*c == 0) || ISBLANK((int)*c)) && (c < endbuf)) c++;

	if (c == endbuf) break;

	if (*c != '#'){
	    Members[nMembers++].loadFile = c;
	}
	while (*c) c++;
    }
    if (nMembers == 0){
	free(load_file_text);
	free(Members);
	yoderrmsg("No entries detected in load file\n");
	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: %p load file text FREED\n",load_file_text);
	    yodmsg("memory: %p member records FREED\n",Members);
	}
	return 0;
    }
    /*
    ** Format of each line is:
    **
    **{optional yod arguments} executable-name {optional program arguments}
    */
     
    for (i=0; i<nMembers; i++){

	sz = 0;
	local_ndlist = NULL;

	yodargs = NULL;   /* will point to first yod arg, or NULL */
        pname = NULL;     /* will point to first char of executable name */
        pargs = NULL;     /* will point to program's first arg, or NULL */

	Members[i].argstr = NULL;
	Members[i].data.argbuflen = 0;
	Members[i].data.fromRank= 0;
	Members[i].data.toRank= 0;

	/*
	** Locate start of yod argments, executable name, and start of
	** executable arguments.  Verify yod arguments are valid.
	**
	** Null terminate the executable name.
	*/
	c = Members[i].loadFile;     /* first non blank in the line */

	if (*c == '-'){
	    yodargs = c;    /* check these are valid yod arguments */
	    while (*c == '-'){
		argnum = get_yod_arg(c+1);
		if (argnum < 0){
		    yoderrmsg("bad argument in load file\n");
		    yoderrmsg("%s\n",Members[i].loadFile);
		    return 0;
		}
               
		while (*c && !ISBLANK((int)*c)) c++;  /* pass argument name */
		if (yodArgs[argnum].val == REQUIRES_VAL){
		    while (*c && ISBLANK((int)*c)) c++;
		    if (*c == 0){
			yoderrmsg("bad argument in load file\n");
			yoderrmsg("%s\n",Members[i].loadFile);
			return 0;
		    }
		    while (*c && !ISBLANK((int)*c)) c++;  /* pass argument value */
		}
		while (*c && ISBLANK((int)*c)) c++;
	    }
	    pname = c;
	    if (*pname == 0){
		yoderrmsg("bad format in load file\n");
		yoderrmsg("%s\n",Members[i].loadFile);
		return 0;
	    }
	}
	else{             /* no yod arguments preceding executable name */
	   pname = c;
	}

        c = pname; 
	while (*c && !ISBLANK((int)*c)) c++;
	if (*c){
	    *c = 0;    /* null terminate executable name */
	    pargs = c+1;
	    while (*pargs && ISBLANK((int)*pargs)) pargs++;
	    if (*pargs == 0) pargs = NULL;
	}
	else{
	    pargs = NULL;
	}


	/*
	** At most two yod arguments can appear before the program
	** name, they are -size and -list.
	**
	** Program name is null terminated so we don't need to worry
	** about picking up program arguments with strstr.
	*/
	
	if ((c = strstr(Members[i].loadFile, "-s"))){
	    GET_INT_VAL(c, sz, Members[i].loadFile);
	}
	if ((c = strstr(Members[i].loadFile, "-l"))){
	    if (pbs_job != NO_PBS_JOB){
		yoderrmsg("WARNING-node list in load file will be ignored\n");
	    }
	    else{
	        GET_STRING_VAL(c, local_ndlist, Members[i].loadFile);
	    }
	}
	Members[i].local_size = sz;
	Members[i].local_ndlist = local_ndlist;

	if (Members[i].local_ndlist){

	    Members[i].listsize= parse_node_list(
					Members[i].local_ndlist,
		                        NULL, 0, 0, MAX_NODES);

	    if (Members[i].local_size > Members[i].listsize){
		yoderrmsg( "ERROR: %s\n",Members[i].loadFile);
                yoderrmsg( "requests %d nodes from list %s of size %d\n",
			Members[i].local_size,
			Members[i].local_ndlist, Members[i].listsize);

                return 0;
	    }
	}

        if (*pname != '/'){             

            if ((c = find_in_cwd(pname, cwd)) == NULL){

	        c = find_in_path(pname);
	    }
        }
        else{
            c = real_path_name(pname);
        }

	if (c == NULL){
	    yoderrmsg("Parsing your load file: Where is executable name %s???\n",pname);
	    return 0;
        }
        
        /*
	** Some lines in the load file may be referring to the same executable,
	**  but with different arguments.  We only want to test and read in the
	**  executable once.
	*/
	sameAs = sameExecutable(i, c);

	if (sameAs > -1){
	    Members[i].pname       = Members[sameAs].pname;
	    Members[i].pnameCount  = 0;
	    Members[i].pnameSameAs = sameAs;

	    Members[i].data.execlen = Members[sameAs].data.execlen;

	    Members[sameAs].pnameCount++;
	}
	else{

	    Members[i].pnameCount  = 1;
	    Members[i].pnameSameAs = -1;
        
	    Members[i].pname = (char *)malloc(strlen(c) + 1);
	    if (!Members[i].pname){
		 CPerrno = ENOMEM;
		 return -1;
	    }
	    if (DBG_FLAGS(DBG_MEMORY)){
		yodmsg("memory: %p (%u) for load member executable name %d\n",
			  Members[i].pname ,
			  (unsigned int)(strlen(c) + 1),i);
	    }

	    strcpy(Members[i].pname, c);

	    rc = check_permissions(Members[i].pname, euid, egid);

	    if (rc != IS_EXECUTABLE){
		yoderrmsg("no permission to execute %s\n",Members[i].pname);
		return 0;
	    }
	    rc = stat(Members[i].pname, &statbuf);

	    if (rc == ENOENT){
		yoderrmsg("Can't stat %s\n",Members[i].pname);
		return 0;
	    }

	    Members[i].data.execlen = statbuf.st_size;
	}

	/*
	** Anything following the executable name is assumed to be
	** program arguments.
	*/
        Members[i].nargs = 1; /* prog name as typed in load file is 1st arg */

	if (pargs){
            c = pargs;
	    while (1){
		Members[i].nargs++;
		while (*c && !ISBLANK((int)*c)) c++;  /* skip over arg */
		if (*c == 0) break;
		while (ISBLANK((int)*c)) c++;   /* get to start of arg */
		if (*c == 0) break;
	    }
	}
	Members[i].args = (char **)malloc(Members[i].nargs * sizeof(char *));

	if ( !Members[i].args){
	    CPerrno = ENOMEM;
	    return -1;
	}
	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: %p (%u) for member args %d\n",
		  Members[i].args,(unsigned int)(Members[i].nargs * sizeof(char *)),
		  i);
	}
	Members[i].args[0] = pname;

	if (Members[i].nargs > 1){
	    c = pargs;

	    for (ii=1; ii<Members[i].nargs; ii++){	
		Members[i].args[ii] = c;
		while (*c && !ISBLANK((int)*c)) c++;   
                if (*c == 0) break;

		*c++ = 0;                    /* null terminate the arg */

		while (*c && ISBLANK((int)*c)) c++; /* get to start of arg */
	    }
	}
    }

    return nMembers;
}

/*
********************************************************************************
  Prepare load data
********************************************************************************
*/

#define WORKBSIZE 5000
static const char *cwd_env_str="PWD=";
static const char *old_cwd_env_str="__CWD=";
static char workb[WORKBSIZE];
 
int
pack_up_env(char **epack, char *envp[], char *cwd)
{
int ne, rc, len, total;
 
    len = strlen(cwd) + strlen(cwd_env_str) + 1;

    sprintf(workb, "%s%s",cwd_env_str,cwd);

    /*
    ** for compatibility with old startup.o, which thinks the cwd
    ** is in environment variable __CWD, we put this in the env too.
    */
 
    sprintf(workb+len, "%s%s",old_cwd_env_str,cwd);
 
    len += (strlen(cwd) + strlen(old_cwd_env_str) + 1);

    if (len > WORKBSIZE){
        yoderrmsg( "pack_up_env: error\n");
        return -1;
    }
 
    for (ne=0; envp[ne]; ne++);
 
    rc = pack_string(envp, ne, workb+len, WORKBSIZE-len);
 
    if (rc < 0){
        yoderrmsg("pack_up_env: error\n");
        return -1;
    }
 
    total = rc + len;

    *epack = (char *)malloc(total);

    if (!(*epack)){
        CPerrno = ENOMEM;
        return -1;
    }
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) for environment\n",*epack, total);
    }

    memcpy(*epack, workb, total);
 
    return total;
}
static int
pack_up_args(int which)
{
loadMembers *mbr;

    if ((which < 0) || (which >= nMembers)){
	yoderrmsg("Invalid argument %d to pack_up_args\n",which);
	return -1;
    }
    mbr = Members + which;

    if (mbr->nargs == 0){
	mbr->data.argbuflen = 0;
	return 0;
    }
    
    mbr->data.argbuflen = 
	pack_string(mbr->args, mbr->nargs, workb, WORKBSIZE);
 
    if (mbr->data.argbuflen < 0){
        yoderrmsg("pack_up_args: error\n");
        return -1;
    }

    mbr->argstr = (char *)malloc(mbr->data.argbuflen);

    if (!(mbr->argstr)){
        CPerrno = ENOMEM;
        return -1;
    } 
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) for args %d\n",mbr->argstr,mbr->data.argbuflen,which);
    }

    memcpy(mbr->argstr, workb, mbr->data.argbuflen);

    return mbr->data.argbuflen;
}
#if 0
static void
display_chars(unsigned char *c, int len, int width)
{
int i;

    yodmsg("\n");
    for (i=0; i<len; i++){
	if (i && (i%width == 0)){
	   yodmsg("\n");
	}
	if ((*c > 31) && (*c < 127)) {
	    yodmsg("%c",*c);
	} else {
	    yodmsg("<%d>",(int)(*c));
	}

        c++;
    }
    yodmsg("\n");
}
#endif

static char full_path_test[MAXPATHLEN];
static char full_path[MAXPATHLEN];

/*
** Fix up fully specified path name
*/
char *
real_path_name(char *fname)
{
char *c;

    if (fname[0] == '/'){ 
        c = realpath(fname, full_path);
 
        if (!c){
            return NULL;
        }
	else{
	   return full_path;
	}
    }
    else{
        yoderrmsg("real_path_name called with relative path name - error\n");
        return NULL;
    }
}
/*
** Check in current working directory for path name.
*/
char *
find_in_cwd(char *fname, char *cwd)
{
char *c;

    if (fname[0] == '/'){ 
        yoderrmsg("find_in_cwd called with relative path name - error\n");
	return NULL;
    }

    sprintf(full_path_test, "%s/%s", cwd, fname);

    c = realpath(full_path_test, full_path);

    if (!c){
	return NULL;
    }
    else{
       return full_path;
    }
}

/*
** Search PATH environment variable for a file name
*/
 
char *
find_in_path(char *fname)
{
char *mypath, *dirloc, *delim, *endchar;
int dlen, fplen;
char *c;
 
    if (fname[0] == '/'){ 
        yoderrmsg("find_in_path called with relative path name - error\n");
	return NULL;
    }

    mypath = getenv("PATH");
 
    if (mypath) {
 
        dirloc = mypath;
 
        endchar = mypath + strlen(mypath);
 
        fplen = strlen(full_path) + 1;
 
        while (dirloc < endchar){  /* search user's path */
 
            delim = strchr(dirloc, ':');
 
            if (delim){
                dlen = (int)(delim - dirloc);
            }
            else{
                dlen = (int)(endchar - dirloc);
            }
 
            if (dlen == 0){
                 dirloc++;
                 continue;
            }
 
            strncpy(full_path_test, dirloc, dlen);
 
            sprintf(full_path_test + dlen, "/%s", fname);

            c = realpath(full_path_test, full_path);
 
            if (c == NULL){
                if (delim){
                    dirloc = delim + 1;
                    continue;
                }
                else{
                    dirloc = endchar;
                    continue;
                }
            }
            break;     /* found it */
        }
 
        if (dirloc >= endchar){
            return NULL;
        }
    }
    return full_path;
}
/*
** Return IS_EXECUTABLE if file is executable and user has permission
** to execute it, return IS_REGULAR if it's a non-executable regular
** file and user has permission to read it, return IS_NOGOOD if neither
** of these statements applies.
*/
static int
check_permissions(char *fname, uid_t euid, uid_t egid)
{
struct stat statbuf;
int rc, Xperm_OK, Rperm_OK, gid_OK, i;
 
    rc = stat(fname, &statbuf);

    if (rc == ENOENT){
	yoderrmsg("Error stat'ing %s\n",fname);
	return IS_NOGOOD;
    }
 
    if (!S_ISREG(statbuf.st_mode)){
 
        yoderrmsg("Sorry, %s is not a regular file\n",
                   fname);
        return IS_NOGOOD;
    }
 
    Xperm_OK = 0;
    Rperm_OK = 0;
    gid_OK  = 0;
 
    if (statbuf.st_gid == egid){
        gid_OK = 1;
    }
    else{
        if (ngroups < 0){
            yoderrmsg("check_permissions: ngroups not set");
            return IS_NOGOOD;
        }
 
        for (i=0; i<ngroups; i++){
            if (statbuf.st_gid == groupList[i]){
                gid_OK = 1;
                break;
            }
        }
    }
 
    if (statbuf.st_uid == euid) {
        if (statbuf.st_mode & S_IXUSR)
            Xperm_OK = 1;
    }
    else if (gid_OK) {
        if (statbuf.st_mode & S_IXGRP)
            Xperm_OK = 1;
    }
    else if (statbuf.st_mode & S_IXOTH){
        Xperm_OK = 1;
    }
 
    if (!Xperm_OK){
	if (statbuf.st_uid == euid) {
	    if (statbuf.st_mode & S_IRUSR)
		Rperm_OK = 1;
	}
	else if (gid_OK) {
	    if (statbuf.st_mode & S_IRGRP)
		Rperm_OK = 1;
	}
	else if (statbuf.st_mode & S_IROTH){
	    Rperm_OK = 1;
	}
    }
 
    if (Xperm_OK){
	return IS_EXECUTABLE;
    }
    else if (Rperm_OK){
	return IS_REGULAR;
    }
 
    return IS_NOGOOD;
}
