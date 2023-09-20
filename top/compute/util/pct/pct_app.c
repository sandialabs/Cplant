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
** $Id: pct_app.c,v 1.45 2002/02/10 23:16:40 pumatst Exp $
**
** pct_app.c - functions handling interactions and record keeping on
**             app process
*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include "pct.h"
#include "srvr_err.h"
#include "srvr_coll.h"
#include "config.h"
#include "srvr_coll_fail.h"

 
#include <sys/types.h>
/* #include <schedbits.h> */
#include <sched.h>
#define PAGE_SIZE  (sysconf(_SC_PAGE_SIZE))
#define STACK_SIZE  (2 * PAGE_SIZE - 32)

#define CMDSIZE   256

const char *scratch_loc=NULL;
static char cmd[CMDSIZE];
static int localFile;

typedef int (*funcPtr_t)(void *);
 
int my_clone( funcPtr_t func, void *arg );

extern load_msg1 init_msg;
extern load_data_buffer LoadData;
extern int Dbglevel;
extern int pct_portal;

/*
** Clean up old executables from RAM disk
*/
#include <dirent.h>

int 
findkids(struct dirent *dent)
{
    /*
    ** dirent structure is not standard, but name field is
    ** "d_name" on the x86 and alpha systems we've seen
    */
    if (strncmp(dent->d_name, NAME_PREFIX, 5)){
        return 0;
    }
    else{
        return 1;
    }
}

static int
cleanup_scratch(void)
{
struct dirent **kids;
int i, nkids, status;

    if (!scratch_loc){
        scratch_loc = ram_disk();

	if (!scratch_loc){
	    return -1;
	}
    }

    nkids = scandir(scratch_loc, &kids, findkids, NULL);

    if (nkids > 0){
        sprintf(cmd,"rm -f %s/%s*",scratch_loc,NAME_PREFIX);
        system(cmd);
        for (i=0; i<nkids; i++){
            free(kids[i]);
        }
        free(kids);
    }
    else if (nkids == 0){
        status = 0;
    }
    else if (nkids < 0){
        status = -1;
    }

    return status;
}

static proc_list active_proc;

void
init_app_structures()
{
    memset((char *)&(active_proc), 0, sizeof(proc_list));

    active_proc.job_id = UNUSED;
}
static void
pname_done(proc_list *pl)
{
    if (!pl || (pl->pname == NULL)) return;

    if (localFile){
        unlink(pl->pname);
        localFile = 0;
    }

    free(pl->pname);
    pl->pname = NULL;
}
void
recover_proc_list_entry(proc_list *pl)
{
    if (!pl) return;
 
    if (Dbglevel){
        log_msg("recover resources from job %d", pl->job_id);
    }
    update_status(STATUS_FREE, 0);
 
    LOCATION("recover_proc_list_entry","user_exec");
 
    if (pl->user_exec){
        free(pl->user_exec);
    }

    LOCATION("recover_proc_list_entry","pname");

    if (pl->pname){
	pname_done(pl);
    }
    
    LOCATION("recover_proc_list_entry","arglist");
    if (pl->arglist){
        free(pl->arglist);
    }
    LOCATION("recover_proc_list_entry","envlist");
    if (pl->envlist){
        free(pl->envlist);
    }
 
    LOCATION("recover_proc_list_entry","zero proc list entry");
    memset(pl, 0, sizeof(proc_list));
 
    pl->job_id = UNUSED;
}
proc_list *
get_proc_list(int job_id)
{
    if (active_proc.job_id == job_id){
        return &(active_proc);
    }

    return NULL;
}
proc_list *
current_proc_entry()
{
    if (active_proc.job_id != UNUSED){
        return &active_proc;
    }
    return NULL;
}
int
nice_kill_time_left()
{
proc_list *pl;
time_t sec;

    pl = current_proc_entry();

    sec = 0;

    if (pl && pl->nice_kill_2){
        sec = pl->nice_kill_2 - time(NULL);
    }

    return sec;
}
int
nice_kill_timeout()
{
proc_list *pl;
time_t tnow;

    pl = current_proc_entry();

    if (pl->nice_kill_2 == 0) return 0;

    tnow = time(NULL);

    if ( (tnow > pl->nice_kill_2) &&
          !(pl->status & SENT_KILL_2) ){

        return 1;
    }

    return 0;
}

proc_list *
get_free_proc_entry()
{
proc_list *pl;

    pl = NULL;

    if (active_proc.job_id == UNUSED){
        pl = &active_proc;
    }

    return pl;
}
int
init_new_job()
{
proc_list *pl;
 
    CLEAR_ERR;
 
    if (Dbglevel >= 2){
        log_msg("job id %d, %d procs, %d members, yod %d/%d/%d",
           init_msg.job_id, init_msg.nprocs,
           init_msg.n_members,
           init_msg.yod_id.nid, init_msg.yod_id.pid, init_msg.yod_id.ptl);
    }
 
    /*************************************************************
    ** Set up pct data structure to keep track of app
    *************************************************************/
 
    if ((init_msg.nprocs <= 0) ||
        (init_msg.nprocs > MAX_NODES) ||
        (init_msg.yod_id.nid < 0) ||
        (init_msg.yod_id.nid > MAX_NODES) ||
        (init_msg.yod_id.pid <= 0 )       ||
        (init_msg.yod_id.ptl > SRVR_MAX_PORTAL)     ){
     
         CPerrno = EINVAL;
         log_msg("ignoring MSG_INIT_LOAD, failed sanity test");
         log_msg("nprocs %d yod id %d/%d/%d",init_msg.nprocs,
            init_msg.yod_id.nid,init_msg.yod_id.pid,init_msg.yod_id.ptl);
         return -1;
    }

    pl = get_free_proc_entry(); /* left over from when PCT hosted more than one job */
 
    if (!pl){
        log_msg("ignoring MSG_INIT_LOAD, out of proc list entries");
        return -1;
    }
 
    update_status(STATUS_BUSY, NEW_JOB);
 
    pl->job_status = NO_STATUS;
    pl->job_id = init_msg.job_id;
    pl->session_id = init_msg.session_id;
    pl->parent_id = init_msg.parent_job_id;
    pl->nprocs = init_msg.nprocs;
    pl->nmembers = init_msg.n_members;
    memcpy(&(pl->srvr), &(init_msg.yod_id), sizeof(server_id));
 
    return 0;
}
int
allocate_load_data(proc_list *pl)
{
int i, rank; 
 
    CLEAR_ERR;

    if ( (LoadData.msg2.data.execlen < 0) ||
         (LoadData.msg2.data.argbuflen < 0) ||
         (LoadData.msg2.envbuflen < 0) ||
         (LoadData.msg2.data.execlen > SSIZE_MAX) ||
         (LoadData.msg2.data.argbuflen > MAX_ARG_LENGTH) ||
         (LoadData.msg2.envbuflen > MAX_ENV_LENGTH) ||
         (LoadData.msg2.ngroups > NGROUPS_MAX) ||
         (LoadData.msg2.ngroups < 1) ||
         (LoadData.msg2.straceMsgLen > 10000) ||
         (LoadData.msg2.straceMsgLen < 0) ||
         (LoadData.msg2.data.fromRank < 0) ||
         (LoadData.msg2.data.toRank >= pl->nprocs) ){

        CPerrno = EINVAL;
        log_warning("allocate_load_data: load parameters not sensible");
        return -1;
    }

    memcpy(&(pl->yod_msg), &(LoadData.msg2), sizeof(load_msg2));

    pl->subgroupSize = LoadData.msg2.data.toRank - LoadData.msg2.data.fromRank + 1;

    for (i=0, rank=LoadData.msg2.data.fromRank; i<pl->subgroupSize; i++){
        pl->subgroupRanks[i] = rank++;
    }

    /*
    ** Allocate storage for program arguments and user environment data
    */
    pl->user_exec = NULL;
    pl->arglist   = (char *)malloc(LoadData.msg2.data.argbuflen);
    pl->envlist   = (char *)malloc(LoadData.msg2.envbuflen);

    if ( !pl->arglist || !pl->envlist){
        CPerrno = ENOMEM;
        log_warning("allocate_load_data: args %d env %d",
           LoadData.msg2.data.argbuflen, LoadData.msg2.envbuflen);
        return -1;
    }
    else{
        pl->my_rank = -1;

        for (i=0; i<pl->nprocs; i++){
            pl->nids[i] = LoadData.thePcts[i];

            if (pl->nids[i] == (int) _my_pnid){
                pl->my_rank = i;
            }
        }
        if (pl->my_rank == -1){
            log_msg("error in node ID list for application, I'm not in it");
            CPerrno = EPROTOCOL;
            return -1;
        }
    }

    if (pl->yod_msg.ngroups > FEW_GROUPS){
	pl->groupList = (gid_t *)malloc( sizeof(gid_t) * pl->yod_msg.ngroups);

	if (!pl->groupList){
            CPerrno = ENOMEM;
            log_warning("allocate_load_data: ngroups %d", pl->yod_msg.ngroups);
            return -1;
	}
    }

    if (LoadData.msg2.straceMsgLen){
        pl->strace = (straceInfo *)malloc(LoadData.msg2.straceMsgLen);

	if (!pl->strace){
            CPerrno = ENOMEM;
            log_warning("allocate_load_data: strace buffer %d\n",LoadData.msg2.straceMsgLen);
            return -1;
	}
    }
    else{
        pl->strace = NULL;
    }
 
    return 0;
}
/*
** return values:
**     1   All PCTs have room in RAM disk to store executable
**     0   At least one PCT has insufficient space, we'll all exec
**            the executable from a remotely mounted disk 
**    -1   error in this routine, give up on the load.
*/
int
determine_executable_location(proc_list *pl)
{
int i, rank, fd, rc, writesz, bal;
int localStorage, tryagain;
 
    CLEAR_ERR;

    LOCATION("determine_executable_location","top");

    /*
    ** Reserve space for the executable in RAM disk, if we can't
    ** do this, we'll exec it from a remotely mounted disk.
    */
    localStorage = 1;     /* we'll be saving executable to RAM disk */
    localFile    = 0;     /* we've a file to clean up on RAM disk   */

    if (!scratch_loc){
        scratch_loc = ram_disk();

	if (!scratch_loc){
	    log_msg("No RAM disk location???");
	    return LAUNCH_ERR_TEMP_NAME;
	}
    }
    
    pl->pname = tempnam(scratch_loc, NAME_PREFIX);
 
    if (pl->pname == NULL){
         log_msg(
      "determine_executable_location: can't generate unique temporary name");
         return LAUNCH_ERR_TEMP_NAME;
    }

    fd = open(pl->pname, O_WRONLY|O_CREAT, S_IRWXU);

    if ((fd < 0) && (errno == ENOSPC)){
        cleanup_scratch();
        fd = open(pl->pname, O_WRONLY|O_CREAT, S_IRWXU); /* and try again */
    }

    if (fd < 0){
        localStorage = 0;
    }
    else{
        localFile = 1;

        /***** this can succeed even if there is insufficient space
        ****** to fill in the file
        rc = lseek(fd, LoadData.msg2.data.execlen - 1, SEEK_SET);
        if (rc < 0){
            localStorage = 0;
        }
        else{
            rc = write(fd, &localStorage, 1);
    
            if (rc != 1){
            localStorage = 0;
            }
        }
        *****/

        rc = lseek(fd, 0, SEEK_SET);
        if (rc < 0){
            localStorage = 0;
        }
        else{
            bal = LoadData.msg2.data.execlen;
            tryagain = 1;

            while (bal > 0){
                writesz = ((CMDSIZE > bal) ? bal : CMDSIZE);
                rc = write(fd, cmd, writesz);
                if (rc != writesz){
                    if ((errno == ENOSPC) && tryagain){
                        close(fd);
                        cleanup_scratch();

                        fd = open(pl->pname, O_WRONLY|O_CREAT, S_IRWXU);
                        lseek(fd, 0, SEEK_SET);

                        bal = LoadData.msg2.data.execlen;
                        tryagain = 0;
                        continue;
                    }
                    else{
                        localStorage = 0;
                        break;
                    }
                }
                bal -= writesz;
            }
        }

        close(fd);
    }

    /*
    ** Each PCT determined whether there was space in it's
    ** RAM disk for the executable.  Let's see what they
    ** each decided.
    */
    LOCATION("determine_executable_location","vote");

    rc = dsrvr_vote(localStorage, collectiveWaitLimit, EXEC_VOTE_TYPE,
            NULL, 0);

    if (rc != DSRVR_OK){
	pname_done(pl);

	if ( (CPerrno == ERECVTIMEOUT) || (CPerrno == ESENDTIMEOUT)) {
	    log_msg("%s",dsrvr_who_failed());
	    send_group_failure_to_yod(current_proc_entry(), 0);
	}
	else {
	    send_failure_to_yod(current_proc_entry(), 0, LAUNCH_ERR_PORTAL_ERR);
	}

        if (rc == DSRVR_RESOURCE_ERROR){  /* too serious to continue */
            log_warning("internal failure in group vote, exit now");
            cleanup_pct();
            log_quit("DONE");
            exit(0);
        }
        else {
            log_warning("failure in group vote, give up on this load");
            return -1;
        }
    }
    /*
    ** Does any PCT hosting the same executable I am have a problem
    ** fitting this file in RAM disk?
    */
    if (localStorage){
        for (i=0; i < pl->subgroupSize; i++){
            rank = pl->subgroupRanks[i];

            if (DSRVR_VOTE_VALUE(rank) == 0){
                localStorage = 0;
	        pl->bt_size=pl->nids[rank]; /* borrow this field for a moment */
                if (Dbglevel){
                    log_msg("PCT rank %d can't fit executable in RAM disk",
                          rank);
                }
                break;
            }
        }
    }
    else{
	pl->bt_size = _my_pnid; /* borrow this field for a moment */
	log_msg("I can't fit executable size %d in RAM disk", 
                 LoadData.msg2.data.execlen);
    }

    if (localStorage){
        /*
        ** yod will send up the executable image.  It will go
        ** in this buffer.
        */
        pl->user_exec = (char *)malloc(LoadData.msg2.data.execlen);
        if (!pl->user_exec ){
            CPerrno = ENOMEM;
            log_warning("determine_executable_location: malloc %d ", 
                    LoadData.msg2.data.execlen);
            pname_done(pl);
            return -1;
        }

    }
    else{
        pl->user_exec = NULL;

        pname_done(pl);
        
        /*
        ** yod will send up the name of the executable, which is
        ** on a file system mounted on our node.  Name will go 
        ** in this buffer.
        */
        pl->pname = (char *)malloc(MAXPATHLEN);
        if (!pl->pname){
            CPerrno = ENOMEM;
            log_warning("determine_executable_location: malloc %d ",
                 MAXPATHLEN);
            return -1; 
        }

    }
    if (Dbglevel){
        if (localStorage){
            log_msg("Executable (%d) will fit in RAM disk",
            LoadData.msg2.data.execlen);
        }
        else{
            log_msg("Executable (%d) will be exec'd from a remote file system",
            LoadData.msg2.data.execlen);
        }
    }
    return localStorage;
}


