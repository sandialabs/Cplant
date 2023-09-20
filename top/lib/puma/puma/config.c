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
** $Id: config.c,v 1.7.2.1 2002/09/27 16:01:35 jrstear Exp $
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
 
#include <puma.h>

#include "cplant_host.h"
#include "sys_limits.h"

#define MAX_NAME_LEN 20

static FILE *cmap=NULL;

#define USE_HOSTNAME yesplease

#ifdef USE_HOSTNAME
/* generally this grows as we read in host names */
int maxnamelen=0;
#else
/* this is the canonical name (su-xx n-xx) len */
int maxnamelen=10;
#endif

static char *siteInfo = NULL;
static char *siteInfoLast = NULL;
static char needSiteInfo=1;
static int site_definitions(void);
static int get_node_names(void);

enum{
    bebopdRestartFile,    /* path name for bebopd registry file */
    nodeNamesFile,    /* path name for file listing compute node names */
    pctScratch,       /* path name to compute node RAM disk */
    pctLocalStorage,  /* path name to compute node real disk, if any */
    logFileName,      /* path name to user log file */
    notifyList,       /* list of administrators yod sends mail to on error */
    vmNameFile,       /* path name to file listing VM name */
    cplantHostFile,   /* path name to file listing bebopd node & fyod nodes */
    solicitPctsTmout, /* timeout to use in solicit_pct_updates() in bebopd */
    parallelFileSys,  /* path name to a global parallel file system */
    pctHealthCheck,   /* parameters which PCT should monitor at runtime */
#ifdef TWO_STAGE_COPY
    vmGlobalStorage,  /* see yod_site.c for an explanation of these */
    vmGlobalStorageFromSrvNd,
    SUglobalStorage,
    localExecPath,
    singleLevelGlobalStorage,
    singleLevelLocalExecPath,
    suNameFormat,
    vmGlobalMachine,
#endif
    pbsPrefix,        /* PBS executables are found in pbsPrefix/bin, etc. */
    clientTimeout,     /* seconds to wait for yod or pingd to respond to bebopd */
    daemonTimeout,     /* seconds to wait for pct or bebopd to respond to yod or pingd */
    niceKillInterval,   /* number of seconds between SIGTERM & SIGKILL */
    lastDef
} definitionTypes;

typedef struct {
   const char *type;            /* env. variable and site file descriptor */
   const char *dflt;            /* default value if no site file definition */
   const char *siteFileValue;   /* definition from site file                */
} _siteDefs;

static _siteDefs siteDefs[lastDef] =
{
{"BEBOPD_RESTART_FILE", "/tmp/saved_pct_list"  , NULL},
{"NODE_NAMES_FILE",    CPLANT_MAP           , NULL},
{"PCT_SCRATCH",     "/tmp/pct-scratch"              , NULL},
{"PCT_OVERFLOW",    "none"                          , NULL},
{"LOGFILENAME",     "/tmp/userlog"            , NULL},
{"NOTIFYLIST",      "none"                            , NULL},
{"VM_NAME_FILE",    CPLANT_PATH"/etc/vmname"   , NULL},
{"CPLANT_HOST_FILE",    CPLANT_HOST            , NULL},
{"SOLICIT_PCTS_TMOUT", "10", NULL},
{"PARALLEL_FILE_SYSTEM",    "/enfs/tmp"        , NULL},
{"PCT_HEALTH_CHECK",    "zombies:memory:scratch"  , NULL},

#ifdef TWO_STAGE_COPY
/*
** A "%s" in these pathnames will be replaced with the virtual
** machine name.
*/
{"VM_GLOBAL_STORAGE", "/usr/local/%s"               , NULL},
{"VM_GLOBAL_STORAGE_FROM_SRV_NODE", "/etc/local"    , NULL},
{"SU_GLOBAL_STORAGE", "/cplant/nfs-cplant"          , NULL},
{"LOCAL_EXEC_PATH",   "/cplant"                     , NULL},
{"SINGLE_LEVEL_GLOBAL_STORAGE", "/etc/local"         , NULL},
{"SINGLE_LEVEL_LOCAL_EXEC_PATH", "/usr/local/%s"    , NULL},
/*
** A "%d" in the scalable unit name format will be replaced
** with the SU number.
*/
{"SU_NAME_FORMAT",     "z-%d"                       , NULL},
{"VM_GLOBAL_MACHINE",  "alaska-sss1-0"              , NULL},
#endif

{"PBS_PREFIX",         "/"                          , NULL},
{"CLIENT_TIMEOUT",         "5"                      , NULL},
{"DAEMON_TIMEOUT",         "30"                     , NULL},
{"NICE_KILL_INTERVAL",    "300"                     , NULL}                    

};

#define SITE_DEFINITION(which) \
{ const char *ret;                                         \
    if ( (ret = getenv( siteDefs[which].type)) == NULL){   \
        if (needSiteInfo){                                 \
  	    site_definitions();                            \
        }                                                  \
	if ( (ret = siteDefs[which].siteFileValue) == NULL ){  \
	    ret = siteDefs[which].dflt;                    \
	}                                                  \
    }                                                      \
    if (ret &&                                             \
        (!strcmp(ret, "none") || !strcmp(ret, "NONE")))    \
	     ret = NULL;                                   \
    return ret;                                            \
}

const char *node_names_file_name() SITE_DEFINITION(nodeNamesFile);
const char *bebopd_restart_file()  SITE_DEFINITION(bebopdRestartFile);
const char *ram_disk()             SITE_DEFINITION(pctScratch);
const char *disk_disk()            SITE_DEFINITION(pctLocalStorage);
const char *log_file_name()        SITE_DEFINITION(logFileName); 
const char *notify_list()          SITE_DEFINITION(notifyList);
const char *vm_name_file()         SITE_DEFINITION(vmNameFile);
const char *cplant_host_file()     SITE_DEFINITION(cplantHostFile);
const char *solicit_pcts_tmout()   SITE_DEFINITION(solicitPctsTmout);
const char *pfs()                  SITE_DEFINITION(parallelFileSys);
const char *pct_health_check()     SITE_DEFINITION(pctHealthCheck);

#ifdef TWO_STAGE_COPY
const char *vm_global_storage()    SITE_DEFINITION(vmGlobalStorage);
const char *vm_global_from_srv_node() SITE_DEFINITION(vmGlobalStorageFromSrvNd);
const char *su_global_storage()    SITE_DEFINITION(SUglobalStorage);
const char *local_exec_path()      SITE_DEFINITION(localExecPath);
const char *single_level_global_storage() SITE_DEFINITION(singleLevelGlobalStorage);
const char *single_level_exec_path() SITE_DEFINITION(singleLevelLocalExecPath);
const char *su_name_format()       SITE_DEFINITION(suNameFormat);
const char *vm_global_machine()    SITE_DEFINITION(vmGlobalMachine);
#endif

const char *pbs_prefix()           SITE_DEFINITION(pbsPrefix);
const char *daemon_timeout()       SITE_DEFINITION(daemonTimeout);
const char *client_timeout()       SITE_DEFINITION(clientTimeout);
const char *nice_kill_interval()   SITE_DEFINITION(niceKillInterval);

#ifdef CONFIGTEST
main()
{
int ii, i;

    for (ii=0; ii<2; ii++){
	printf("%s: %s\n",siteDefs[nodeNamesFile].type, 
                          node_names_file_name());
	printf("%s: %s\n",siteDefs[bebopdRestartFile].type, 
                          bebopd_restart_file());
	printf("%s: %s\n",siteDefs[pctScratch].type, 
                          ram_disk());
	printf("%s: %s\n",siteDefs[pctLocalStorage].type, 
                          disk_disk());
	printf("%s: %s\n",siteDefs[logFileName].type, 
                          log_file_name());
	printf("%s: %s\n",siteDefs[vmNameFile].type, 
                           vm_name_file());
	printf("%s: %s\n",siteDefs[cplantHostFile].type, 
                           cplant_host_file());
        printf("%s: %s\n", siteDefs[solicitPctsTmout].type,
                           solicit_pcts_tmout());
#ifdef TWO_STAGE_COPY
	printf("%s: %s\n",siteDefs[vmGlobalStorage].type, 
                           vm_global_storage());
	printf("%s: %s\n",siteDefs[vmGlobalStorageFromSrvNd].type, 
                           vm_global_from_srv_node());
	printf("%s: %s\n",siteDefs[SUglobalStorage].type, 
                           su_global_storage());
	printf("%s: %s\n",siteDefs[localExecPath].type, 
                           local_exec_path());
	printf("%s: %s\n",siteDefs[singleLevelGlobalStorage].type, 
                           single_level_global_storage());
	printf("%s: %s\n",siteDefs[singleLevelLocalExecPath].type, 
                           single_level_exec_path());
	printf("%s: %s\n",siteDefs[suNameFormat].type, 
                           su_name_format());
	printf("%s: %s\n",siteDefs[vmGlobalMachine].type, 
                           vm_global_machine());
#endif		       
	printf("%s: %s\n",siteDefs[pbsPrefix].type, 
                           pbs_prefix());

        printf("\n\nand again:\n");
    }
}
#endif

static char *
findDefValue(const char *def)
{
char *c;
char *nm, *val, *giveup;

    if ((siteInfo == NULL) || (def == NULL)) return NULL;

    c = siteInfo;  
    while (!(*c)) c++;

    giveup = siteInfoLast - strlen(def) + 1;

    if (giveup <= c) return NULL;

    nm = NULL;
    val = NULL;

    /*
    ** The site file contains the definition names followed
    ** by the definition values, with only whitespace separating
    ** them.
    **
    ** All whitespace has been converted to NULL, so we can
    ** find strings.
    */
    while (1){

        if (!strcmp(def, c)){
	    nm = c;
	    break;
	}
	c += strlen(c);

	while (!(*c) && (c < giveup)) c++;

        if (c >= giveup) break;
    }

    if (nm){
	 
	   for ((val = nm + strlen(nm)); 
	        !(*val) && (val < siteInfoLast); 
		val++);

    }
    return val;

}
static int
site_definitions()
{
const char *sitefile;
char *c;
struct stat sbuf;
int i, fd, rc;

    if (siteInfo){
        free(siteInfo);
    }
    needSiteInfo = 0;

#ifdef CONFIGTEST
    printf("Scanning site definitions\n");
#endif

    if ( (sitefile = getenv( "SITE_FILE" ) ) == NULL) {
        sitefile = SITE_FILE ;
    }

    for (i=0; i<lastDef; i++){
        siteDefs[i].siteFileValue = NULL;
    }

    rc = stat(sitefile, &sbuf);

    if (rc < 0){
        return 0;
    }

    fd = open(sitefile, O_RDONLY);

    if (fd < 0){
        return 0;
    } 

    siteInfo = (char *)malloc(sbuf.st_size + 1);

    if (siteInfo == NULL){
	close(fd);
        return -1;
    }
    siteInfoLast = siteInfo + sbuf.st_size;

    rc = read(fd, siteInfo, sbuf.st_size);

    if (rc < sbuf.st_size){
        free(siteInfo);
	siteInfo = NULL;
        close(fd);
	return -1;
    }
    *siteInfoLast = 0;
   
    close(fd);

    for (c=siteInfo; c<siteInfoLast; c++){
         if (isspace(*c)) *c = 0;
    }

    for (i=0; i<lastDef; i++){

        c = findDefValue(siteDefs[i].type);

	if (c){
	 
           siteDefs[i].siteFileValue = c;
	           
	}
    }

    return 0;
}
void
refresh_config()
{
    needSiteInfo = 1;

    site_definitions();
}

/*
*******************************************************************************
**  List of node names 
*******************************************************************************
*/
static int numNames=0;

#ifdef USE_HOSTNAME
char nodeStrings[MAX_NODES][MAX_NAME_LEN];

#else
static int nodeNum[MAX_NODES];
static int suNum[MAX_NODES];
static int rackType[MAX_NODES];

static const char *nnames[65]={
"n-00", "n-01", "n-02", "n-03", "n-04", "n-05", "n-06", "n-07",
"n-08", "n-09", "n-10", "n-11", "n-12", "n-13", "n-14", "n-15",
"n-16", "n-17", "n-18", "n-19", "n-20", "n-21", "n-22", "n-23",
"n-24", "n-25", "n-26", "n-27", "n-28", "n-29", "n-30", "n-31",
"n-32", "n-33", "n-34", "n-35", "n-36", "n-37", "n-38", "n-39",
"n-40", "n-41", "n-42", "n-43", "n-44", "n-45", "n-46", "n-47",
"n-48", "n-49", "n-50", "n-51", "n-52", "n-53", "n-54", "n-55",
"n-56", "n-57", "n-58", "n-59", "n-60", "n-61", "n-62", "n-63",
"n-64"
};

static const char *rack[6]={ "SU-", "r-", "t-", "b-", "y-", "g-" };

enum{ RACK_SU, RACK_R, RACK_T, RACK_B, RACK_Y, RACK_G };

static const char *sunames[100]={
"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", 
"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", 
"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", 
"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", 
"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", 
"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", 
"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", 
"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", 
"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", 
"90", "91", "92", "93", "94", "95", "96", "97", "98", "99"
};
#endif

static int
get_node_names(void)
{
  static int first_time= 1;
  int ii, len;
  char line[100], *start, *end; 
#ifndef USE_HOSTNAME
  char *cptr;
#endif
  const char *mapFile;

  mapFile = node_names_file_name();

  if (!mapFile){
      fprintf(stderr,"Can't locate node names file\n"); 
      return -1;
  }

    if (!first_time)   {
      return 0;
    }
    first_time= 0;

    /* use cplant map file */
    if ( (cmap = fopen( mapFile, "r" )) == NULL ) {
      fprintf(stderr, "get_node_names: cant open cplant map file %s\n", mapFile);
      return -1;
    }

    ii = 0;
    while ( fgets(line, sizeof(line), cmap) != NULL && ii < MAX_NODES ) {
      start = line;
      if (line[0] == '\"') {
        start++;
      }
      end = start;
      while ( *end != '\n' && *end != '\"' ) {
        end++;
      } 
      *end = 0;
      len = (int)(end-start);

#ifdef USE_HOSTNAME
      if (len > maxnamelen) {
        maxnamelen = len;
      }
      strcpy(nodeStrings[ii], start);
#else
      nodeNum[ii] = 0;
      suNum[ii] = 99;
      rackType[ii] = RACK_SU;

      cptr = strstr(line, "c-");
      if (cptr) {
        sscanf(cptr+2, "%d", &nodeNum[ii]);
      }
      cptr = strstr(line, "n-");
      if (cptr) {
        sscanf(cptr+2, "%d", &nodeNum[ii]);
      }

      cptr = strstr(line, "SU-");
      if (cptr) {
        sscanf(cptr+3, "%d", &suNum[ii]);
        rackType[ii] = RACK_SU;
        goto ENDRACK;
      }
      cptr = strstr(line, "r-");
      if (cptr) {
        sscanf(cptr+2, "%d", &suNum[ii]);
        rackType[ii] = RACK_R;
        goto ENDRACK;
      }
      cptr = strstr(line, "t-");
      if (cptr) {
        sscanf(cptr+2, "%d", &suNum[ii]);
        rackType[ii] = RACK_T;
        goto ENDRACK;
      }
      cptr = strstr(line, "b-");
      if (cptr) {
        sscanf(cptr+2, "%d", &suNum[ii]);
        rackType[ii] = RACK_B;
        goto ENDRACK;
      }
      cptr = strstr(line, "y-");
      if (cptr) {
        sscanf(cptr+2, "%d", &suNum[ii]);
        rackType[ii] = RACK_Y;
        goto ENDRACK;
      }
      cptr = strstr(line, "g-");
      if (cptr) {
        sscanf(cptr+2, "%d", &suNum[ii]);
        rackType[ii] = RACK_G;
      }
ENDRACK:
#endif
      ii++;
    }
        
    if (ii == MAX_NODES ) {
      fprintf(stderr, "get_node_names: no. of nodes from cplant map file %s\n", mapFile);
      fprintf(stderr, "get_node_names: exceeds MAX_NODES\n");
      return -1;
    }
    numNames = ii;
    return 0;
}

#define VALIDATE_PHYSNID(physnid, err) \
    if (numNames < 1){              \
	if (get_node_names()<0){  \
	    return err;              \
	}                           \
    }                               \
    if ((physnid < 0) || (physnid >= numNames)){  \
	return err;                  \
    }

int
phys_to_SU_number(int physnid)
{
    VALIDATE_PHYSNID(physnid, -1);
#ifndef USE_HOSTNAME
    return suNum[physnid];
#else
    return 0;
#endif
}

/* prints node name */
int
print_node_name(int phys_nid, FILE *fp)
{
#ifdef USE_HOSTNAME
  fprintf(fp, "%s", nodeStrings[phys_nid]);
  return strlen(nodeStrings[phys_nid]);
#else
  int n, s, r;

  VALIDATE_PHYSNID(phys_nid, -1);

  n = nodeNum[phys_nid];
  s = suNum[phys_nid];
  r = rackType[phys_nid];
  fprintf(fp, "%s%s %s", rack[r], sunames[s], nnames[n]);
  return strlen(rack[r]) + strlen(sunames[s]) + 1 + strlen(nnames[n]);
#endif 
}
	
int
phys2name(int physnid)
{
    VALIDATE_PHYSNID(physnid, -1);
    return 0;
}
