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
** $Id: pct_health.c,v 1.4 2001/08/23 15:17:37 rklundt Exp $
**
** Functions to query compute node status.
** 
*/

#include "config.h"
#include "pct_health.h"
 
/*
** Check for undesirable conditions on the host.
** return value - OK if host passes checks.
*/
INT32
check_host_health()
{
const char *checkInfo;

  checkInfo = pct_health_check();

  if (!checkInfo) return OK;

#ifdef CHECK_ZOMBIES
  if (strstr(checkInfo, "zombies") ||
      strstr(checkInfo, "ZOMBIES")    ){

      if ( check_host_zombies() ) return ERROR;
  }
#endif
#ifdef CHECK_MEMORY
  if (strstr(checkInfo, "memory") ||
      strstr(checkInfo, "MEMORY")    ){

      if ( check_host_memory() )  return ERROR;
  }
#endif
#ifdef CHECK_SCRATCH
  if (strstr(checkInfo, "scratch") ||
      strstr(checkInfo, "SCRATCH")    ){

      if ( check_host_scratch() ) return ERROR;
  }
#endif
  if (Dbglevel) { log_msg("Host checks healthy."); }
  return OK;
}

/*
** If the memory available is low, return error.
*/
INT32
check_host_memory()
{
  FILE *fp;
  MEMINFO mi;
  int rc;
  char line[80];
  char qualifier[80];
  long value;
	int value2, used;
  double mem_thresh = MEMORY_THRESHOLD;
  double mem_free = 0.0, mem_total = 0.0, mem_level = 0.0, mem_used = 0.0;

  LOCATION("check_host_health","top");

  /* 
  ** Read /proc/meminfo. If MemFree < MEMORY_THRESHOLD% of MemTotal, 
  ** return error.
  */
  if ( (fp = fopen("/proc/meminfo", "r")) != NULL ) {
	 
	 while ( fgets(line, 80, fp) != NULL ) {
		if ( sscanf(line, "%s %ld %d", qualifier, &value, &value2) ) {
		  if (!strncmp("Mem:", qualifier, sizeof("Mem:"))) {
			 used = value2;
			 break;
		  }
		}
	 }
	 while ( fgets(line, 80, fp) != NULL ) {
		if ( sscanf(line, "%s %ld", qualifier, &value) ) {
		  if (!strncmp("MemTotal:", qualifier, sizeof("MemTotal:")))
			 mi.total = value << 10;
		  else if (!strncmp("MemFree:", qualifier, sizeof("MemFree:")))
			 mi.free = value << 10; 
		}
	 }
	 fclose(fp);
	 
	 /* check for free >= mem_thresh% of total */
	 mem_total = mi.total;
/* 	 mem_free = mi.free; */
	 mem_used = used;
	 mem_free = mem_total - mem_used;
	 mem_level = mem_thresh * mem_total / 100.0;
	 if (Dbglevel) log_msg("MemFree: %ld\nMemTotal: %ld\nMem Used: %f\nTotal - Used: %f\n"
								  , mi.free, mi.total, mem_used, mem_free);
	 if ( mem_free < mem_level ) {
		log_msg("Low free memory detected.\n");
		log_msg("Total - Used: %f\nThreshold: %f\n", mem_free, mem_level);
		return ERROR;
	 }
  } else {
	 if (Dbglevel) log_msg( "Unsuccessful call to meminfo: %d\n", rc );
  }

  return OK;
}

/*
** Wait for zombie children of the PCT
** Report existence of other zombies
*/
INT32
check_host_zombies()
{
  int rc;
  int *status;
  int options = WNOHANG | WUNTRACED;
  pid_t foundpid = 1;
  pid_t pid = 0;

  LOCATION("check_host_zombies", "top");
  
  /*
  ** Look for orphaned children.
  */
  while (foundpid > 0) { 
	 foundpid = waitpid(pid, status, options);
	 if ( foundpid > 0 && Dbglevel ) 
		{ log_msg("Found child, pid: %d", foundpid); }
  }

  /* 
  ** Check for any remaining zombies, they require sysadmin attention
  */
  rc = system("grep \") Z \" /proc/*/stat > /dev/null 2>&1");
  if ( rc == 0 ) {
	 log_msg("Zombie process(es) found on host.");
	 return ERROR;
  }

  return OK;
}

INT32
check_host_scratch()
{
  int rc;
  char du_cmd[256];
  char rm_cmd[256];
  mode_t mod = 0755;

  LOCATION("check_host_scratch", "top");

  /*
  ** This variable is defined at first app load, 
  ** before which there is no need to check it.
  */
  if ( ! scratch_loc ) 
	 return OK;

  /*
  ** Check existence of scratch area, perhaps
  ** it has been inadvertently removed.
  */
  sprintf(du_cmd, "[ -d %s ]", scratch_loc);
  rc = system(du_cmd);
  if ( rc != 0 ) {
	 if (mkdir(scratch_loc, mod) != 0 && 
		  errno != EEXIST )
		log_warning("Unable to create dir %s.", scratch_loc);
	 if ( system(du_cmd) ) return ERROR;
  }

  /*
  ** Check size of scratch area, should be nearly empty. 
  */
  sprintf(du_cmd, 
			 "[ `/usr/bin/du -ks %s | cut -f1` -lt %d ]",
			 scratch_loc, MAX_SCRATCH_USED);
  rc = system(du_cmd);
  if ( rc != 0 ) {
	 sprintf(rm_cmd, "rm -rf %s/*", scratch_loc);
	 rc = system(rm_cmd);
	 if ( rc != 0 ) {
		if (Dbglevel) 
		  log_msg("Error removing files from %s.", scratch_loc);
		/* Might be good enough to go */
		rc = system(du_cmd); 
		return rc;
	 }
  }

  return OK;
}

