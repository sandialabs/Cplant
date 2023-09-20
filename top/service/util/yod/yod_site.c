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
**  $Id: yod_site.c,v 1.13 2001/09/26 07:05:42 lafisk Exp $
**
**  Compute nodes that run user executables are diskless.  They have
**  a RAM disk set up in which to place user executables prior to
**  the PCT fork and exec.  It is possible that the RAM disk is too
**  small to contain the executable.  
**
**  Unless/until we implement our own exec() that lets us exec() an image
**  from memory, we need for yod to have the ability to copy the 
**  executable file to a location from which the PCT can exec() it.  
**  The nature of this copy will depend on a site's system support
**  backbone, so all code required for it is localized in this file,
**  and in the source for the suid program, yod_site_priv.c.
**
**  ----------- this scheme is ifdef'd TWO_STAGE_COPY - it's a pain ---
**
**  The copies use path name definitions.  The default definitions
**  are in the source file config.c.  They can be overridden by environment
**  variables, or by text entries in a site file.  (See the routines
**  in config.c that determine the path name definitions.)
**
**  Definition of "root PCT": In most cases there is one executable 
**  file.  In the case of heterogeneous load there is more than one.  
**  The PCT hosting the lowest rank member of the application for a 
**  given executable is called the root PCT for that executable.
**
**  Load protocol:  yod tells the PCTs the size of the executable file.
**  The root PCT informs yod as to whether the image will fit in the 
**  RAM disk of all compute nodes hosting that executable.  If so, yod 
**  sends the executable to the root PCT for broadcast to the other 
**  PCTs.  If the executable will not fit in RAM disk of at least one
**  compute node, yod copies it to a file system that is readable from
**  the compute node, and sends the root PCT the pathname to the executable.
**  The root PCT broadcasts this pathname to the other PCTs.
**
**  This copy of the executable will be site specific, depending on how 
**  networks are set up and how file systems are cross mounted between 
**  compute and support machines.  This copy is handled by the code in 
**  this source file.
**
**  It works this way:
**
**     The compute nodes hosting the executables are in some collection
**     of SUs.  yod uses routines in config.c to figure out which list of 
**     SUs are hosting the compute nodes.
**
**     If all the compute nodes are in the same SU, yod copies the
**     executable to a file system that all compute nodes can read.
**     (SINGLE_LEVEL_GLOBAL_STORAGE).  The pathname to this file
**     system from the compute nodes is SINGLE_LEVEL_LOCAL_EXEC_PATH.
**
**     If the compute nodes are in more than one SU, yod first copies
**     the executable to the SSS1 (VM_GLOBAL_STORAGE_FROM_SRV_NODE).
**     It then copies it (with rsh) from there (VM_GLOBAL_STORAGE) to 
**     a directory on each SSS0 (SU_GLOBAL_STORAGE) that is readable from the 
**     compute nodes.  The pathname to this file system from the compute 
**     nodes is LOCAL_EXEC_PATH.
**
**     yod sends the pathname of the executable to the root PCT.
**
**     To make the executable name unique, yod uses the virtual
**     machine name, the Cplant job ID of the application, and the
**     member number of the executable in a heterogeneous load.
**     In a small Cplant without virtual machine names (set VM_NAME_FILE
**     to "none"), yod will simply use the string "Cplant" in the file name.
**
**     yod cleans up the files copied when the application is done.
**
** ----------------------------------------------------------------------
**
**  May 2001 - We normally have a parallel file system up and running now,
**    so let's copy the executable there.  The PCTs can exec it from there.
**  
*/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "yod_data.h"
#include "util.h"
#include "yod_comm.h"
#include "config.h"
#include "srvr_comm.h"
#include "appload_msg.h"
#include "yod_site.h"

extern double dclock(void);
extern int sverrno;

static char cmd[MAXPATHLEN];
static char tempbuf[MAXPATHLEN];
static char vmname[80];

#ifdef TWO_STAGE_COPY

static struct _exec_files{
int sunums[MAX_SU];
char *global_ename;
char *su_ename;
} *exec_files = NULL;

static int
save_su_file_for_cleanup(int member, int su, char *ename)
{
    if (!exec_files[member].su_ename){
	exec_files[member].su_ename = (char *)malloc(strlen(ename) + 1);
	if (!exec_files[member].su_ename) return -1;
        strcpy(exec_files[member].su_ename, ename);
    }
    exec_files[member].sunums[su] = 1;

    if (DBG_FLAGS(DBG_MEMORY)){
        yodmsg("memory: %p (%d) save names of copied executables\n",
                  exec_files[member].su_ename , strlen(ename) + 1);
    }

    return 0;
}
static int
save_global_file_for_cleanup(int member, char *ename)
{
    if (!exec_files[member].global_ename){
	exec_files[member].global_ename = (char *)malloc(strlen(ename) + 1);
	if (!exec_files[member].global_ename) return -1;
        strcpy(exec_files[member].global_ename, ename);
    }

    if (DBG_FLAGS(DBG_MEMORY)){
        yodmsg("memory: %p (%d) save name of global file copied\n",
                  exec_files[member].global_ename, strlen(ename) + 1);
    }

    return 0;
}
#endif

int
cleanup_copied_files(int timing_data)
{
int i,j;
double td1;

#ifndef TWO_STAGE_COPY
loadMembers *mbr;
#endif

    if (timing_data) td1 = dclock();

#ifdef TWO_STAGE_COPY
    for (i=0; i<MAX_MEMBERS; i++){
	if (exec_files[i].global_ename){
	    if (DBG_FLAGS(DBG_LOAD_2)){
		yodmsg("Removing %s\n",exec_files[i].global_ename);
            }
	    unlink(exec_files[i].global_ename);
	    free(exec_files[i].global_ename);

            if (DBG_FLAGS(DBG_MEMORY)){
                 yodmsg("memory: %p exec file name FREED\n",exec_files[i].global_ename);
	    }
	}
	if (exec_files[i].su_ename){
	    for (j=0; j<MAX_SU; j++){
		if (exec_files[i].sunums[j]){

		    snprintf(cmd, MAXPATHLEN, "yod_site_priv remove %d %s %s",
				 j, exec_files[i].su_ename, vmname);

	            if (DBG_FLAGS(DBG_LOAD_2)){
			yodmsg("Removing %s on SU %d\n", exec_files[i].su_ename, j);
		    }
		    system(cmd);
		}
	    }
	}
    }
#else
    j = total_members();

    for (i=0; i<j; i++){
        mbr = member_data(i);

	if (mbr->pnameSameAs == -1){

	    if (mbr->execPath){
	        sprintf(tempbuf,"rm -f %s",mbr->execPath);
		if (DBG_FLAGS(DBG_LOAD_2)){
		    yodmsg("Removing copy of executable: %s\n",tempbuf);
		}
		system(tempbuf);

		free(mbr->execPath);
		mbr->execPath = NULL;
	    }
	}
	else{
	    mbr->execPath = NULL;
	}
    }
#endif

    if (timing_data){
	yodmsg("YOD TIMING:  Delete remote copies of executables: %f\n",
		    dclock()-td1);
    }
    return 0;
}

#ifdef TWO_STAGE_COPY
/*
** Assume you have scalable units managed by an admin node with a hard disk, so you
** can copy one executable to each SU and tell the PCTs to come get it.  We did
** this before we had a parallel fs.
*/
static char *vmdefault="default";

int
copy_executable(int member, int job_ID, int *nodeMap, int timing_data,
		 uid_t uid, gid_t gid)
{
int status, rank, i, ii, rc, len;
int suNums[MAX_SU], nsu, su, num, sameAs;
loadMembers *mbr, *mbr2;
char namebuf[MAXPATHLEN], local_path[MAXPATHLEN], targetbuf[MAXPATHLEN];
char *c;
const char *vfile;
FILE *fp;
struct stat sbuf;
double td1, td2, td3, td4;
const char *singleLevelGlobalStorage, *singleLevelExecPath;
const char *localExecPath, *vmGlobalStorageFromSrvNode;

    status = 0;

    if (exec_files == NULL){
        num = total_members();

        exec_files = (struct _exec_files *)malloc(num * sizeof(struct _exec_files));

	if (!exec_files){
	    CPerrno = ENOMEM;
	    return -1;
	}

	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: %p (%d) executable file info\n",
		      exec_files , num * sizeof(struct _exec_files));
	}

    }

    mbr = member_data(member);

    if (!mbr){
	return -1;
    }

    sameAs = mbr->pnameSameAs;

    if (sameAs > -1){

        mbr2 = member_data(sameAs);

	if (!mbr2){
	    yoderrmsg("Invalid member data - yod bug - sorry\n");
	    return -1;
	}

	mbr2->pnameCount--;

	if (mbr2->pnameCount == 0){
	    if (mbr2->exec){
                 free(mbr2->exec);

		 if (DBG_FLAGS(DBG_MEMORY)){
		     yodmsg("memory: %p executable %s FREED\n",mbr2->exec,mbr2->pname);
		 }

            }
	}
	
    }
    else{
        mbr->pnameCount--;

	if (mbr->pnameCount == 0){
	    if (mbr->exec){
                 free(mbr->exec);

		 if (DBG_FLAGS(DBG_MEMORY)){
		     yodmsg("memory: %p executable %s FREED\n",mbr->exec,mbr->pname);
		 }
            }
	}
    }


    /*
    ** Which SUs are the compute nodes in?
    */

    for (i=0; i<MAX_SU; i++){
	suNums[i] = 0;
    }
    nsu = 0;

    for (rank = mbr->data.fromRank; rank <= mbr->data.toRank; rank++){

	su = phys_to_SU_number(nodeMap[rank]);

	if ((su < 0) || (su >= MAX_SU)){
	    yoderrmsg("corrupt member data\n");
	    return -1;
	}
	if (suNums[su] == 0){
	    nsu++;
	    suNums[su] = 1;
        }
    }
    /*
    ** Get VM name, which we need to make name of executable unique.
    ** Don't worry if there's no file containing VM name, we'll assume
    ** this is a small Cplant without multiple VMs.
    */
    vfile = vm_name_file();

    if (!vfile) vfile = vmdefault;

    vmname[0] = 0;

    if (strcmp(vfile, NO_VM) && !stat(vfile, &sbuf)){

	fp = fopen(vfile, "r");

	if (fp){
	    /*
	    ** first word found in this file must be the VM name
	    */
	    len = (sbuf.st_size > MAXPATHLEN) ? MAXPATHLEN : sbuf.st_size;
	    rc = fread(namebuf, len, 1, fp);

	    if (rc == 1){

		for (c=namebuf, ii=0, i=0; ii<len; ii++, c++){ 
		    if (isspace(*c)){
		       if (i) break;
		       else   continue;
		    }
		    if (i==79){   /* corrupt vm name file */
			i = 0;
			break;
		    }
		    vmname[i++] = *c;
		}
		vmname[i] = 0;
	    }
	    fclose(fp);
	}
    }
    if (vmname[0] == 0){
	strcpy(vmname,"Cplant");
    }
    /*
    ** Create executable name.  To make it unique, use virtual machine
    ** name, job ID, and which member of heterogeneous load.
    */
    sprintf(namebuf, "%s%s-%d-%d", NAME_PREFIX, vmname, job_ID, member);

    /*
    ** Copy the executable to global storage.  This file system is
    ** writable from the service nodes and either:
    **
    **   1 SU case: readable from all compute nodes
    **             OR
    **   >1 SU case: is a location from which the executable can 
    **               be rcp'd to the SU global storage
    */
    if (nsu == 1){
        singleLevelGlobalStorage = single_level_global_storage();
        singleLevelExecPath = single_level_exec_path();

        snprintf(targetbuf, MAXPATHLEN, "%s/%s", 
		   make_name(singleLevelGlobalStorage, vmname),
			 namebuf);
        save_global_file_for_cleanup(member, targetbuf);

	snprintf(cmd, MAXPATHLEN, "cp %s %s", mbr->pname, targetbuf);

        snprintf(local_path, MAXPATHLEN, "%s/%s", 
		   make_name(singleLevelExecPath, vmname), 
		   namebuf);
    }
    else{
        localExecPath = local_exec_path();
        vmGlobalStorageFromSrvNode = vm_global_from_srv_node();

        snprintf(targetbuf, MAXPATHLEN, "%s/%s", 
		 make_name(vmGlobalStorageFromSrvNode, vmname),
			 namebuf);
        save_global_file_for_cleanup(member, targetbuf);

	snprintf(cmd, MAXPATHLEN, "cp %s %s", mbr->pname, targetbuf);

        snprintf(local_path, MAXPATHLEN, "%s/%s", 
	     make_name(localExecPath, vmname), namebuf);
    }

    if (timing_data) td1 = dclock();

    if (DBG_FLAGS(DBG_LOAD_2)) yodmsg("%s\n",cmd);

    rc = system(cmd);

    yodmsg("%s ...\n",cmd);

    if (rc || stat(targetbuf, &sbuf)){
	yodmsg("Copy failed.  Talk to system administration please.\n");
	yodmsg("(\"%s\")\n",cmd);
	return -1;
    }

    if (timing_data) td2 = dclock();
    /*
    ** If we have more than one SU, we rcp the executable from global
    ** storage to the system support station dedicated to each SU.  The
    ** PCTs on the compute nodes in that SU can read the executable
    ** from there.
    **
    ** Is this quicker than copying once to global storage and having
    ** all PCTs read from there?  I don't know.
    **
    ** must be root here, so we use an suid program.
    */

    if (nsu > 1){
        if (timing_data) td3 = dclock();
	for (i=0; i<MAX_SU; i++){
	    if (suNums[i]){

		sprintf(cmd,"yod_site_priv put %d %s %s %d %d", 
			 i, namebuf, vmname, uid, gid);

                yodmsg("rcp'ing %s to SU-%d\n", namebuf, i);

		rc = system(cmd);
		if (rc){
		    yodmsg("Copy failed.  Talk to system administration please.\n");
		    yodmsg("(\"%s\")\n",cmd);
		    return -1;
		}

		save_su_file_for_cleanup(member, i, namebuf);
	    }
	}
        if (timing_data) td4 = dclock();
    }
    if (timing_data){
	yodmsg("YOD TIMING:  Copy executable to global storage %f\n",
		    td2-td1);
        if (nsu > 1){
	    yodmsg("YOD TIMING:  Copy executable to local SUs %f\n",
		    td4-td3);
	}
    }
    /*
    ** Now send the root PCT the path name for the executable.
    */

    rc = send_pcts_put_message(MSG_PUT_EXEC_PATH, (CHAR *)&job_ID, sizeof(int),
	  local_path, strlen(local_path)+1, daemonWaitLimit,
	  PUT_ROOT_PCT_ONLY, member);

    if (rc){
	yoderrmsg("Error sending name %s to pcts",local_path);
	status = -1;
    }
    free_names();

    return status;
}
#else

/*
** copy executable to a parallel file system
*/
int
move_executable(int rank, int timing_data, int job_ID)
{
int i, ii, status, sameAs, len, rc; 
loadMembers *mbr, *mbr2;
const char *pfsPath, *vfile;
char *c;
FILE *fp;
struct stat sbuf;
double td1, td2;

    status = 0;

    mbr = which_member(rank);

    if (!mbr){
	yodmsg("invalid rank\n");
	return -1;
    }

    sameAs = mbr->pnameSameAs;

    if (sameAs > -1){
        mbr2 = member_data(sameAs);

	if (!mbr2){
	    yodmsg("member data structure is corrupt\n");
  	    return -1;
        }

	if (mbr2->execPath){   /* already moved this one */

	    mbr->execPath = mbr2->execPath;

	    return status;
	}

    }
    else{
        mbr2 = NULL;
    }

    /*
    ** Get VM name, which we need to make name of executable unique.
    ** Don't worry if there's no file containing VM name, we'll assume
    ** this is a small Cplant without multiple VMs.
    */
    vfile = vm_name_file();
    vmname[0] = 0;

    if (strcmp(vfile, NO_VM) && !stat(vfile, &sbuf)){

	fp = fopen(vfile, "r");

	if (fp){
	    /*
	    ** first word found in this file must be the VM name
	    */
	    len = (sbuf.st_size > MAXPATHLEN) ? MAXPATHLEN : sbuf.st_size;
	    rc = fread(tempbuf, len, 1, fp);

	    if (rc == 1){

		for (c=tempbuf, ii=0, i=0; ii<len; ii++, c++){ 
		    if (isspace(*c)){
		       if (i) break;
		       else   continue;
		    }
		    if (i==79){   /* corrupt vm name file */
			i = 0;
			break;
		    }
		    vmname[i++] = *c;
		}
		vmname[i] = 0;
	    }
	    fclose(fp);
	}
    }
    if (vmname[0] == 0){
	strcpy(vmname,"Cplant");
    }

    pfsPath = pfs();    /* path to global parallel file system */

    if (pfsPath == NULL){
        yodmsg("Insufficient RAM disk on a node and no parallel file system to copy to");
	return -1;
    }
    /*
    ** Create executable name.  To make it unique, use virtual machine
    ** name, job ID, and which member of heterogeneous load.
    */
    sprintf(tempbuf, "%s/%s%s-job-%d-rank-%d", pfsPath, NAME_PREFIX, vmname, job_ID, rank);

    mbr->execPath = malloc(strlen(tempbuf) + 1);

    if (!mbr->execPath){
        yodmsg("can't malloc");
	return -1;
    }

    strcpy(mbr->execPath, tempbuf);

    if (mbr2){
        mbr2->execPath = mbr->execPath;
    }

    /*
    ** Copy the executable to parallel global file system.
    */

    snprintf(cmd, MAXPATHLEN, "cp %s %s", mbr->pname, tempbuf);

    if (timing_data) td1 = dclock();

    rc = system(cmd);

    yodmsg("%s ...\n",cmd);

    if (rc || stat(tempbuf, &sbuf)){
	yodmsg("Copy failed.  Talk to system administration please.\n");
	yodmsg("(\"%s\")\n",cmd);
	return -1;
    }

    if (timing_data) td2 = dclock();
    
    if (timing_data){
	yodmsg("YOD TIMING:  Copy executable to global storage %f\n",
		    td2-td1);
    }

    return status;
}
#endif
