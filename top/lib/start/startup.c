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
** $Id: startup.c,v 1.101.2.3 2002/08/06 16:58:05 ktpedre Exp $ 
*/

/*--------------------------------------------------------------------------
   Startup: this code executes during processing of .init before
   main.  Apps started by a pct request pid map, etc. here.  Stand 
   alone processes just initialize portals.
  --------------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sched.h>

#include <puma.h>
#include <puma_errno.h>
#include <rpc_msgs.h>
#include <sys_portal_lib.h>
#include <cTask/cTask.h>



#include "pct_start.h"
#include "fyod_map.h"
#include "fdTable.h"
#include "collcore.h"
#include "srvr_err.h"
#include "srvr_comm.h"
#include "link.h"
#include "proc_id.h"

#include "p30.h"
#include "myrnal.h"

#ifndef SRVR_STARTUP
#include "protocol_switch.h"
#endif

#ifdef __linux__
extern loff_t __llseek64( int fd, loff_t offset, int whence);
extern int open64(const char *, int, int);
#endif

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

#define NUM_CTRL_BUFS                   (16)
#define CTRL_BUF_SIZE                   (1024)

INT32  CPerrno = 0;

UINT16 _yod_cmd_pid;
UINT16 _yod_cmd_nid;
int    _yod_cmd_ptl;
int    _my_parent_handle;
int    _my_own_handle;


int _my_dna_ptl=0;

int	  _my_PBS_ID;
ppid_type _my_ppid=0;
spid_type _my_pid;
gid_type  _my_gid;
nid_type  _my_rank;
nid_type  _my_pnid;
nid_type  _my_nnodes;
nid_type  _fyod_nid;

nid_type *_my_nid_map;
ppid_type *_my_pid_map;

UINT32 _my_pidnid;
UINT32 _my_gidrank;
UINT32 _my_umask;
int _fyod_map[FYOD_MAP_SZ];

taskInfo_t _my_taskInfo; /* my task info */

INT32  ___startup_complete=0;
INT32  ___proc_type=0;

#define INVAL_ID ((UINT32)(-1))
 
int Dbglevel;

int _yod_io_data_ptl;
int _yod_io_ctl_ptl;

server_id __pct_id;

CHAR _CLcwd[MAXPATHLEN];

extern void initFileIO(hostReply_t *,hostReply_t *,hostReply_t *);

static int     pct_ptl;
static int     pct_nid;
static int     pct_pid;

static int     log_cplant_startup;

enum {fileioReq, nidsReq, fyodMapReq, ppidsReq, lastReq};

static int reqHandle[lastReq];

static void start_error(int error_type);
static void get_context(void);
static void get_group_context(void);

/*
** Set platform dependent NLSPATH at compile time to identify
** location of for_msg.cat for fortran codes.
*/
#ifdef __linux__
#define NLSPATH "/cplant/lib/linux/%N"
#elif __osf__
#define NLSPATH "/cplant/lib/osf/%N"
#endif

static CHAR    fname[MAXPATHLEN];
static CHAR   *_redirectStdout, *_redirectStderr;

#define REDIRECT_STREAM(fd, basename, fdname) { \
    if (strlen(basename) > (MAXPATHLEN-30)){    \
	log_msg("%s (too long)",basename);      \
	start_error(ERR_START_REDIRECT);        \
    }                                           \
    if (strstr(basename, "/enfs/") && !(strchr(basename, ':'))){  \
        sprintf(fname,"enfs:%s/%s.job-%d.rank-%d",      \
                basename,fdname,_my_gid,_my_rank);      \
    }                                                   \
    else{                                               \
         sprintf(fname,"%s/%s.job-%d.rank-%d", \
                basename,fdname,_my_gid,_my_rank); \
    }                                              \
    if (!freopen(fname, "w", fd)){                 \
        log_warning("can't redirect to %s",fname); \
        start_error(ERR_START_REDIRECT);           \
    }                                              \
}


#define PCT_WAIT   60.0

/*
** These references will bring in our definitions of these funtions
** before libc functions are linked in.  Otherwise libc functions calling
** "open", for example, will execute the libc open if no reference has
** occured to "open" up to that point in the link process.
*/
#ifdef __linux__
#include <sys/statfs.h>
#endif
#include <fcntl.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/utsname.h>

typedef void (*fncptr)(void);

#ifdef __osf__
extern fstatfs(), flock(), getdirentries();
#endif

#ifndef SRVR_STARTUP
#if defined __linux__ 
extern void __open_catalog();
#endif
fncptr link_these_in_now_please[100] =
{
#ifdef __linux__
(void(*)(void))__fxstat,
(void(*)(void))__lxstat,
(void(*)(void))__xstat,
(void(*)(void))fstatfs,
(void(*)(void))statfs,
#endif
#if defined __linux__
(void(*)(void))__open_catalog,
#endif
#ifdef __osf__
(void(*)(void))fstat,
(void(*)(void))fstatfs,
(void(*)(void))lstat,
(void(*)(void))stat,
(void(*)(void))mmap,
(void(*)(void))munmap,
#endif
(void(*)(void))access,
(void(*)(void))chdir,
(void(*)(void))chmod,
(void(*)(void))chown,
(void(*)(void))close,
(void(*)(void))creat,
(void(*)(void))dup,
(void(*)(void))dup2,
(void(*)(void))fcntl,
(void(*)(void))flock,
(void(*)(void))fsync,
(void(*)(void))ftruncate,
(void(*)(void))getcwd,
(void(*)(void))getdirentries,
(void(*)(void))getgid,
(void(*)(void))getegid,
(void(*)(void))gethostname,
(void(*)(void))getuid,
(void(*)(void))geteuid,
(void(*)(void))isatty,
(void(*)(void))ioctl,
(void(*)(void))link,
(void(*)(void))lseek,
#ifdef __linux__
(void(*)(void))__llseek64,
(void(*)(void))open64,
#endif
(void(*)(void))mkdir,
(void(*)(void))open,
(void(*)(void))read,
(void(*)(void))rename, 
(void(*)(void))rmdir,
(void(*)(void))setgid,
(void(*)(void))setegid,
(void(*)(void))setuid,
(void(*)(void))seteuid,
(void(*)(void))symlink,
(void(*)(void))sync,
(void(*)(void))truncate,
(void(*)(void))ttyname,
(void(*)(void))umask,
(void(*)(void))uname,
(void(*)(void))unlink,
(void(*)(void))write
};
#endif

/*
** exported because PCT needs this file descriptor
*/
int cplant_fd, cache_fd;

static void
get_context(void)
{
    _my_rank =     0;
    _my_nnodes =   1; 
    _my_gid =      0;

    _my_ppid = 0;
    _my_pid = (spid_type) getpid();

    _my_nid_map = &_my_pnid;
    _my_pid_map = &_my_ppid;
}

static void
get_group_context(void)
{
char *c;

    /***********************************************************
    ** Pick up context from environment est. by parent pct
    ***********************************************************/

    pct_ptl = ((c = getenv("__PCT_PORTAL")) ? atoi(c) : SRVR_INVAL_PTL);
    pct_pid = PPID_PCT;
    pct_nid = _my_pnid;

    __pct_id.nid = pct_nid;
    __pct_id.pid = pct_pid;
    __pct_id.ptl = pct_ptl;

    _yod_cmd_nid  = ((c = getenv("__YODNID")) ? atoi(c) : (UINT16)-1);
    _yod_cmd_pid  = ((c = getenv("__YODPID")) ? atoi(c) : (UINT16)-1);
    _yod_cmd_ptl  = ((c = getenv("__YODPTL")) ? atoi(c) : SRVR_INVAL_PTL);

    _my_parent_handle  = ((c = getenv("__MYPARENT")) ? atoi(c) : INVAL);
    _my_own_handle     = ((c = getenv("__MYSELF")) ? atoi(c) : INVAL);
 
    _my_rank =   ((c = getenv("__MYRANK")) ? atoi(c) : INVAL_ID);
    _my_nnodes = ((c = getenv("__NPROCS")) ? atoi(c) : INVAL_ID);
    _my_gid =    ((c = getenv("__GROUPID")) ? atoi(c) : INVAL_ID);

    _my_PBS_ID = ((c = getenv("__PBS_JOB_ID")) ? atoi(c) : INVAL_ID);

    _my_umask =  ((c = getenv("__UMASK")) ? atoi(c) : INVAL_ID);
 
/* similar to call to register_ppid(_my_pcb, PPID_AUTO, _my_gid),
   but this goes directly thru original ioctl call...so that it's
   symmetric w/ the open() above...
 */
    _my_taskInfo.ppid  = PPID_AUTO;
    _my_taskInfo.pnid  = _my_pnid;
    _my_taskInfo.rid   = _my_rank;
    _my_taskInfo.gid   = _my_gid;
    _my_taskInfo.nprocs= _my_nnodes;

    if ( ! (c = getenv("__MYNAME")) ) {
      strcpy(_my_taskInfo.name, "unknown");
    }
    else {
      strcpy(_my_taskInfo.name, c);
    }
    
    if ( ntv_sys(__NR_ioctl, cplant_fd, CPL_SYS_APCB, 
                                    (unsigned long) &_my_taskInfo) < 0) {
      _my_ppid = 0;
    }
    else {
      _my_ppid = _my_taskInfo.ppid;
    }

    if  (_my_ppid < PPID_FLOATING) {
      _my_ppid = 0;
      start_error(ERR_START_PPID);
    }

    _my_pid = getpid();

    _fyod_nid = ((c=getenv("__FYODNID")) ? atoi(c) : (UINT16)-1);

    if ( (_yod_cmd_nid == (UINT16)-1) ||
         (_yod_cmd_pid == (UINT16)-1) ||
         (_yod_cmd_ptl == SRVR_INVAL_PTL)  ||
         (pct_ptl == SRVR_INVAL_PTL) ||
         (_my_nnodes == INVAL_ID) ||
         (_my_gid == INVAL_ID) ||
         (_my_rank == INVAL_ID)
	) {
 
        start_error(ERR_START_ENVIRONMENT);
    }

    c = getenv("PWD");
    if (c) {
        strncpy(_CLcwd, c, MAXPATHLEN-1);
    }

    _redirectStdout = getenv("CPLANT_STDOUT");
    _redirectStderr = getenv("CPLANT_STDERR");
}

/*********************************************************************************
** Startup code for a process that wants to use portals.  Process was started
** one of two ways:
**
**    1. It is a member of an application group started with a yod on
**       a service node.  The yod is the application server, and it
**       notifies pcts on compute nodes to start the app processes.
**       The pcts provide initial data to the app processes here.
**
**    2. It is a stand alone process, like the pct.  It does not have
**       a meaningful group id.  It communicates via portals using its
**       nid and pid.  It has no application server.
**
*********************************************************************************/

#ifdef __GNUC__
void init_process_portals(void) __attribute__ ((constructor));
#endif

void
#ifdef __osf__
__init_process_portals(void)
#else
init_process_portals(void)
#endif
{
int i, rc, await;
int nlen, plen, lmsg;
hostReply_t fstdio[3];
int ctl_ptl;
control_msg_handle mhandle;

    if (getenv("__SLEEP_AT_STARTUP")){
        sleep(60);
    }

    log_open("startup");

    /* init some common io protocols */

#ifndef SRVR_STARTUP
    for (i=0; i<FYOD_MAP_SZ; i++) {
      _fyod_map[i] = -1;
    }
    io_proto_init();
#endif

    if (getenv("__YODNID")){        /* member of app group, child of pct */
        ___proc_type = APP_TYPE;    /* mustn't write stdin, stdout or stderr */
    }
    else{                           /* stand alone process */
        ___proc_type = SERV_TYPE;
    }

    cplant_fd  = ntv_sys(__NR_open, "/dev/cTask", O_RDWR, 0 );
    cache_fd   = ntv_sys(__NR_open, "/dev/addrCache", O_RDWR, 0 );

    if ( cplant_fd < 0 || cache_fd < 0 ) {
        CPerrno = EOPENCPLANTDEV;
        start_error(ERR_OPEN_CPLANT_DEVICE_FILES);
    }

    /* Set the close-on-exec flags for the devices. */
    
    ntv_sys(__NR_fcntl, cplant_fd, F_SETFD, 1);

    rc = ntv_sys(__NR_ioctl, cplant_fd, CPL_MLOCKALL, MCL_CURRENT | MCL_FUTURE);

    if (rc){
        CPerrno = ELOCK;
        start_error(ERR_START_MLOCKALL);
    }

    if (getenv("__LOG_CPLANT_STARTUP")){
	log_cplant_startup = 1;
    }
    else{
	log_cplant_startup = 0;
    }

    /*****************************************
    ** initialize user pcb and portal table 
    *****************************************/

    CLEAR_ERR;

    _my_pnid = ntv_sys(__NR_ioctl, cplant_fd, CPL_GET_PHYS_NID, (unsigned long)(0));

#ifdef PORTALS_2_INIT
    init_process_pcb(_my_pcb, 0);
#endif
 
    /*****************************************
    ** initialize context variables
    *****************************************/
    
    if (___proc_type == APP_TYPE){
        get_group_context();
    }
    else{
        get_context();   
    }
    /*
    ** A note to the debugger: you can't use printf until after
    ** the call to initFileIO().  For a back door to the PCT, use
    ** PCT_DUMP (any time after get_group_context).  The data will
    ** be displayed in the log file (/var/log/cplant) by the pct.
    **
         PCT_DUMP(1000,        a "message type", make it >= 1000
                  2,0,0,0,     any four ints you like
                  NULL,NULL);  any two pointers
    */

    _my_pidnid =  (UINT32)(_my_pnid | (_my_ppid << 16));
    _my_gidrank = (UINT32)(_my_rank | (_my_gid << 16));

    _my_taskInfo.pnid = _my_pnid;
    _my_taskInfo.gid  = _my_gid;    /* doing this in register_ppid() also
                                        since we need to pass gid to portals
                                        module
                                     */
    _my_taskInfo.nprocs = _my_nnodes;

    _my_taskInfo.rid   = _my_rank;
    _my_taskInfo.ppid  = _my_ppid;
    
    if (___proc_type == SERV_TYPE){     /* stand alone processes are done */

        /*
	** For non-Cplant apps, the remaining setup must be done after
	** main.  Register a Portal ID (fixed or floating - startup
	** doesn't know at this point) with register_ppid, then
	** call server_library_init() for libsrvr point-to-point
	** capability (control messages, puts and gets).  If group
	** formation and collective ops are needed, follow that
	** initialization with srvr_init_coll().
	*/

        ___startup_complete = 1;

        return;
    }

    CLEAR_ERR;

    /************************************************************* 
    ** Startup of application group processes works like this:
    **
    **   Do all pre-main initializations, including P3 lib.
    **   Send a READY message to the pct, which propagates 
    **        back to yod.
    **   Receive a GO_MAIN message from the pct, sent up
    **        from yod when all members have reported READY.
    **        Order of GO_MAIN messages sent is from rank 0 to
    **        rank {last}.
    **   Synchronize the app processes with P3 barrier.
    **   Perform any pre-main collective operations.  
    **   Synchronize again.
    **   Procede to main. 
    *************************************************************/

    /************************************************************* 
    ** Note on Portals 3 / Myrinet interface
    **
    **   Apps use it for:  
    **      barrier in startup (PtlNIBarrier)
    **      getting load data from PCT in startup (libsrvr calls)
    **      File I/O to yod or fyod (libsrvr calls)
    **      MPI (MPI library)
    **
    **   Servers/utilities use it for:  
    **      control messages (libsrvr)
    **      puts and gets (libsrvr)
    **      group formation and communication (libsrvr)
    **
    **   Portal indices:
    **      0      Used by P30 library for collective ops
    **      5      MPI receive portal
    **      6      MPI read portal
    **      7      MPI ack portal
    **      10     Used by libsrvr for control messages 
    **      11     Used by libsrvr for put and get buffers
    **      12     Used by libsrvr for collective ops
    **
    *************************************************************/

    /***********************************************************************
    ** allocate comm structures for IO operations done through yod
    ***********************************************************************/

    rc = server_library_init();

    if (rc){
        log_msg("error in server_library_init");
	start_error(ERR_START_SRVRLIB);
    }

    _yod_io_ctl_ptl = srvr_init_control_ptl(NIOBUFS + 1);

    if (_yod_io_ctl_ptl == SRVR_INVAL_PTL){
        start_error(ERR_START_IO_PTL);
    }

    /***********************************************************************
    **  Send portal ID to parent PCT.  
    ***********************************************************************/

    rc = srvr_send_to_control_ptl(pct_nid, pct_pid, pct_ptl,
            CHILD_PORTAL_ID, (char *)&_my_ppid, sizeof(ppid_type));

    if (rc){
        start_error(ERR_START_PPID);
    }
    /***********************************************************************
    ** New process requests the following from parent process pct:
    **     nid map
    **     stdio descriptors
    **     portal id map
    **
    ** We open a control portal for the GO_MAIN token from the pct.
    ***********************************************************************/

    CLEAR_ERR;

    ctl_ptl = srvr_init_control_ptl(2);

    if (ctl_ptl == SRVR_INVAL_PTL){
        start_error(ERR_START_CTL_PTL);
    }

    nlen = sizeof(nid_type) * _my_nnodes;

    _my_nid_map = (nid_type*) malloc(nlen);
    if (!_my_nid_map){
        CPerrno = ENOMEM;
        start_error(ERR_START_NID_MAP);
    }

    plen = sizeof(ppid_type) * _my_nnodes;
    _my_pid_map = (ppid_type*) malloc(plen);

    if (!_my_pid_map){
        CPerrno = ENOMEM;
        start_error(ERR_START_PID_MAP);
    }

    /*
    ** Request application nid map, portal id map, and stdout, etc. info
    */
    if (log_cplant_startup){
	PCT_DUMP(2002,0,0,0,0,0,0);
    }
    reqHandle[ppidsReq] = srvr_comm_get_req((char *)(_my_pid_map), plen, 
                CHILD_PID_MAP_REQUEST, NULL, 0,
                pct_nid, pct_pid, pct_ptl,
                NONBLOCKING, PCT_WAIT*2);

    if (reqHandle[ppidsReq] < 0){
	PCT_DUMP(9002, 0, 0, 0, 0, 0, 0);
        start_error(ERR_START_PID_MAP);
    }
    
    if (log_cplant_startup){
	PCT_DUMP(2003,0,0,0,0,0,0);
    }
    reqHandle[nidsReq] = srvr_comm_get_req((char *)(_my_nid_map), nlen, 
                CHILD_NID_MAP_REQUEST, NULL, 0,
                pct_nid, pct_pid, pct_ptl,
                NONBLOCKING, PCT_WAIT);

    if (reqHandle[nidsReq] < 0){
        start_error(ERR_START_NID_MAP);
    }


    reqHandle[fyodMapReq] = srvr_comm_get_req((char *)(_fyod_map), 
                                 FYOD_MAP_SZ*sizeof(int), 
			CHILD_FYOD_MAP_REQUEST, NULL, 0,
			pct_nid, pct_pid, pct_ptl,
			NONBLOCKING, PCT_WAIT);

    if (reqHandle[fyodMapReq] < 0){
        start_error(ERR_START_FYOD_MAP);
    }

    if (log_cplant_startup){
	PCT_DUMP(2004,0,0,0,0,0,0);
    }
    reqHandle[fileioReq] = srvr_comm_get_req((char *)fstdio, sizeof(fstdio), 
                CHILD_FILEIO_REQUEST, NULL, 0,
                pct_nid, pct_pid, pct_ptl,
                NONBLOCKING, PCT_WAIT);

    if (reqHandle[fileioReq] < 0){
        start_error(ERR_START_FILEIO);
    }
    if (log_cplant_startup){
	PCT_DUMP(2105,0,0,0,0,0,0);
    }

    /*
    ** Wait for requested data to come in
    */
    for (i=0, await=0; i<lastReq; i++){
        await |= (1 << i);	
    }
    while (await){

	for (i=0; i<lastReq; i++){

	    if (await & (1 << i)){

		rc = srvr_test_write_buf(reqHandle[i]);

		if (rc == 1){             /* got it */
		    await &= ~(1 << i);
		    srvr_delete_buf(reqHandle[i]);
		}
		else if (rc < 0){
		    start_error(ERR_START_DATA_PTL);
		}
	    }
	}
    }

    /* System requires a map of node IDs and portals IDs. */

    rc = ntv_sys(__NR_ioctl, cplant_fd, CPL_SET_NID_MAP, (unsigned long)_my_nid_map);

    if (rc < 0){
        CPerrno = ESYSPTL;
        start_error(ERR_START_NID_MAP);
    }

    rc = ntv_sys(__NR_ioctl, cplant_fd, CPL_SET_PID_MAP, (unsigned long)_my_pid_map);

    if (rc < 0){
        CPerrno = ESYSPTL;
        start_error(ERR_START_PID_MAP);
    }

    /*
    ** from this point forward, output to stdout and stderr go
    ** back to the application server 
    */

    initFileIO(&(fstdio[0]), &(fstdio[1]), &(fstdio[2]));

    /***********************************************************************
    ** Send a ready message back to the pct, and wait for a go-main
    ** token in reply.  Then synchronize with the other members.
    ***********************************************************************/

    CLEAR_ERR;

    if (log_cplant_startup){
	PCT_DUMP(2005,0,0,0,0,0,0);
    }

    rc = srvr_send_to_control_ptl(pct_nid, pct_pid, pct_ptl,
            CHILD_IS_READY_TOKEN, (char *)&ctl_ptl, sizeof(int));

    if (rc){
        start_error(ERR_START_GOMAIN);
    }

    if (log_cplant_startup){
	PCT_DUMP(2006,0,0,0,0,0,0);
    }
    while (1){
        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(ctl_ptl, &mhandle, &lmsg, NULL, NULL);

        if (rc < 0){
            start_error(ERR_START_GOMAIN);
        }
        if (rc == 0) continue;

        if (lmsg != CHILD_GO_MAIN_TOKEN){
            CPerrno = EPROTOCOL;
            start_error(ERR_START_GOMAIN);
        }
        srvr_free_control_msg(ctl_ptl, &mhandle);

        break;
    }

    rc = srvr_release_control_ptl(ctl_ptl);

    if (rc){
        start_error(ERR_START_GOMAIN);
    }

    if (log_cplant_startup){
	PCT_DUMP(2100,0,0,0,0,0,0);
    }

    rc = srvr_p30_barrier(60);  

    if (rc){
        start_error(ERR_START_SYNC);
    }
 
    /*************************************************************
    ** Got any pre-main collective ops?  Do them now.
    *************************************************************/

#if WE_HAVE_PREMAIN_COLLECTIVE_OPS
    CLEAR_ERR;

    if (log_cplant_startup){
	PCT_DUMP(2009,0,0,0,0,0,0);
    }

    rc = _puma_collective_init();

    if (rc){
        start_error(ERR_START_PREMAIN);
    }
 
    comm_buf = ((c = getenv("__NXLIB")) ? atoi(c) : 0);
 
    if (comm_buf){
        /*
        ** initialize nx library   NOT TESTED!!!!!!!
        **
        rc = initialize_nx_portal(comm_buf);

        if (rc){
        }
        */
    }
#endif
 
    /*************************************************************
    ** Synchronize the processes once more, they may use puma
    ** collective ops right after main.
    *************************************************************/

    if (log_cplant_startup){
	PCT_DUMP(2200,0,0,0,0,0,0);
    }
    rc = srvr_p30_barrier(60);

    if (rc){
        start_error(ERR_START_SYNC);
    }

    if (log_cplant_startup){
	PCT_DUMP(2010,0,0,0,0,0,0);
    }

   ___startup_complete = 1;

    /*
    ** redirection must be done after ___startup_complete is
    ** set or our IO library rejects it.
    */
    if (_redirectStdout){
        REDIRECT_STREAM(stdout, _redirectStdout, "stdout");
    }
    if (_redirectStderr){
        REDIRECT_STREAM(stderr, _redirectStderr, "stderr");
    }

    /*
    ** Export NLSPATH to application environment
    ** identifies location of fortran error message catalog
    */
    setenv("NLSPATH", NLSPATH, 0);

    if (getenv("__SLEEP_BEFORE_MAIN")){
        sleep(60);
    }
}

#define GO_TOKEN 0x0c0c0c0c

static void
start_error(int error_type)
{
child_nack_msg nmsg;

    if (___proc_type == SERV_TYPE){     /* log and exit */

        log_error("fatal error in startup: %s\n",
                start_error_string(error_type));
    }

    nmsg.lerrno     = errno;
    nmsg.CPerrno   = CPerrno;
    nmsg.start_log = error_type;

    if (_my_ppid != 0){
        srvr_send_to_control_ptl(pct_nid, pct_pid, pct_ptl, CHILD_NACK_TOKEN,
                         (char *)&nmsg, sizeof(child_nack_msg));
    }
    else{
	exit(error_type);
    }

    while(1) sched_yield();   /* we'll get killed by pct eventually */
}

/*--------------------------------------------------------------------------*/
/*
** can call this after we know pct nid/pid/ptl to get pct to dump info,
** useful for debugging before initFileIO call, or if printf is not
** working, can also be used by app to write log messages.
*/

void
out_of_band_pct_message(int type, int int1, int int2, int int3, int int4,
                        void *ptr1, void *ptr2)
{
out_of_band_msg msg;
 
    if (pct_pid == 0){   /* don't use this too early */
        return;
    }
    /*
    ** sending these protocol messages out of band will wreak havoc
    */
    if (((type >= CHILD_FIRST_REQUEST) && (type <= CHILD_LAST_REQUEST)) ||
        (type == CHILD_IS_READY_TOKEN) ||
        (type == CHILD_PORTAL_ID) ||
        (type == CHILD_NACK_TOKEN) )
    {
        type |= 0x10000;
    }

    msg.int1 = int1;
    msg.int2 = int2;
    msg.int3 = int3;
    msg.int4 = int4;
    msg.ptr1 = ptr1;
    msg.ptr2 = ptr2;

    srvr_send_to_control_ptl(pct_nid, pct_pid, pct_ptl, type, 
                        (char *)&msg, sizeof(out_of_band_msg));
}
/*--------------------------------------------------------------------------*/
 
#ifdef PORTALS_2_INIT
static VOID
init_process_pcb( PROCESS_PCB_TYPE *proc_pcb, PID_TYPE pid )
{
#if 0
    proc_pcb->local_pid = pid;
    proc_pcb->phys_nid = _my_pcb->phys_nid;
    proc_pcb->host_id = 0;
 
#else
    (void) pid;
#endif
 
    init_portals(proc_pcb);
}
 
/*--------------------------------------------------------------------------*/
 
static VOID
init_portals( PROCESS_PCB_TYPE *proc_pcb )
{
    int i;
    PORTAL_DESCRIPTOR *ptl_desc;
 
    ptl_desc = proc_pcb->portal_table2;
    for( i = 0; i < NUM_PORTALS; i++ ) {
#ifndef LINUX_PORTALS
        ptl_desc->handler = NULL;
#endif
        ptl_desc->mem_op = UNASSIGNED_MD;
        ptl_desc->stat_bits = 0;
        ptl_desc->active_cnt = 0;
        ptl_desc++;
#ifdef __PROSE__
            ptl_desc->priority = -1;
#endif
    }
 
    PTL_CLR_ALLFLAGS ( &(proc_pcb->portals_pending) );
    PTL_CLR_ALLFLAGS ( &(proc_pcb->portals_dropped) );
}
#endif

/*
** Error values defined in pct_start.h
**
** Errors reported in application startup back to pct 
**  parent process.
*/
const char *start_errors[] =
{
"no error",
"mlockall -- application locks more physical memory than is allowed.\n",
"system control portal",
"synchronization portal",
"write portal",
"read portal",
"ack portal",
"file IO setup",
"READY token",
"GOMAIN token",
"synchronization step",
"premain processing",
"NID map",
"PID map",
"FYOD map",
"control portal",
"data portal",
"environment variables",
"ppid registration",
"opening a cplant device",
"stream redirection",
"creating IO portal",
"initializing P3/Myrinet interface",
"initializing libsrvr"
};


