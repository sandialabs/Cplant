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
/* $Id: fyod.c,v 1.38 2001/04/01 23:51:18 pumatst Exp $ */ 
 
/*			FFFFF    Y   Y     OOO     DDD
**			F         Y Y     O   O    D  D
**			FFF	   Y 	  O   O    D   D
**			F	   Y	  O   O    D  D
**			F	   Y	   OOO     DDD
*/
#include <stdio.h>
#include <signal.h>
#include "CMDhandlerTable.h"
#include "fyod.h"
#include <puma.h>
#include <ppid.h>
#include "errno.h"
#include "rpc_msgs.h"
#include "host_msg.h"
#include "util.h"
#include "cplant_host.h"
#include "cplant_db.h"
#include <unistd.h>
#include <sched.h>
 
#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

INT32 		Dbgflag = DBG_PROGRESS + DBG_IO_1;

static char* progName;
 
static int   checkArgs(int argc, char *argv[]);
static void  work( void );
static void  initSignals( void );
static void  cleanup( int notused );
#ifdef __linux__
static void  sigignore (int signum);
#endif
#ifdef __osf__
static int sigignore (int signum);
#endif
 
int Dbglevel=0;
void jdump(int a[], int n);
 
int
main( int argc, char *argv[] )
{
    int i, rc, have_unit=0;	
 
    fprintf (stderr, "\t\t FYOD version  March  1999, (requires PPID) \n");

    _my_ppid = register_ppid(&_my_taskInfo, PPID_FYOD, GID_FYOD, "fyod");
    if ( _my_ppid != PPID_FYOD ) {
        fprintf(stderr, "Can not register myself with PPID=%d\n", PPID_FYOD);
        exit(-1);
    }

    rc = server_library_init();

    if (rc){
        fprintf(stderr,"initializing server library");
        exit(-1);
    }

    fyodInitWhoIam();
    if ( ! checkArgs( argc, argv ) ) {
	fprintf( stderr, "usage: fyod  [-D level]\n" );
	exit( -1 );
    }
 
    for ( i = 0; i < argc; i++ ) {
	if ( ( ! strcmp( argv[ i ], "-D" ) ) && ( i + 1 < argc )) {
	    sscanf( argv[ i + 1 ], "%i", &Dbglevel );	
        }
    }
 
 
    if ( Dbglevel < 2 )    /*   Used to say == 0   */
       Dbgflag = 0;
 
    initSignals();
 
    progName = argv[0];
 
    if (Dbglevel > 2) {
       fprintf( stderr, " _my_pnid is  %d (Myrinet node number)\n", _my_pnid );
       fprintf(stderr, "My PPID is %d\n", _my_ppid);
    }
/*		The following will initialize a portal      */
    host_cmd_init( );
 
    for (i=0; i<MAX_DISKS_PER_FYOD_NODE; i++) {
       fyod_units[i] = -1;
    }

#ifdef USE_DB
    if ( getFyodUnits_db( fyod_units ) < 0 ) {
      fprintf(stderr, "fyod failed to get fyod_units...\n");
      exit(-1);
    }
#else
    if ( getFyodUnits( fyod_units ) < 0 ) {
      fprintf(stderr, "fyod failed to get fyod_units...\n");
      exit(-1);
    }
#endif

    have_unit = 0;
    for (i=0; i<MAX_DISKS_PER_FYOD_NODE; i++) {
       if ( fyod_units[i] >= 0 ) {
         have_unit = 1;
         break;
       }
    }

    if ( !have_unit ) {
      fprintf(stderr, "fyod: no sense in continuing -- according the Cplant\n");
      fprintf(stderr, "database I have not been assigned a disk unit...\n");
      exit(-1);
    }

    work ( );
    cleanup( 0 );
    return 0;  /*NOTREACHED*/
}
static void 
jvd_dmp (char a[]);

static void
work()
{
    int 	loop = TRUE;	
    INT32	cmd_type;
    hostCmd_t	*cmd;
    INT32	tbl_offset;
    NID_TYPE	src_nid;
    PID_TYPE	src_pid;
    int		unit;
    int		processed;
    control_msg_handle io_handle;
    int jd;		/* John's debug flag controlling extra print
					on forwarding  */
     int rc;
 
/*         cmd_type, etc is defined in TOP/include/portals/rpc_msgs.h  */
 
    while ( loop ) {		/*  NO EXIT   6/25/98     */
        errno = 0;

        rc = get_next_cmd(&io_handle);

	if (rc == 0){
                                /* Let another process run */
            sched_yield();

	    if ( Dbglevel > 1 )
            (void)sleep((unsigned) 2);

	    continue;
	}
	if (rc < 0){
            fprintf(stderr,"Error seeking next command (%s)\n",
                  CPstrerror(CPerrno));
            exit(-1);
	}

	cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(io_handle);
	src_nid = SRVR_HANDLE_NID(io_handle);
	src_pid = SRVR_HANDLE_PID(io_handle);

	cmd_type = cmd->type;

        if ((cmd != NULL) && (Dbglevel > 4)) {
           printf(" FYOD got a command %p, the type is %x\n",
                   cmd, cmd_type);
        }
 
 
	if ( Dbglevel > 4 )
	   printf("Valid requires  %d  <=  %d  <= %d  \n",
	    (int)FIRST_CMD_NUM, (int)cmd_type, (int)LAST_CMD_NUM );


	if ( (cmd_type >= FIRST_CMD_NUM) && (cmd_type <= LAST_CMD_NUM) ) {
	    tbl_offset= ( cmd_type - FIRST_CMD_NUM );
	    if ( Dbglevel > 3 ) {
		fprintf( stderr,
		"Got new command -0x%x, (tbl %d), snid %d, spid %d\n",
		-cmd_type, tbl_offset, src_nid, src_pid );
	}
/*                  -------------------------------------------------
**                  Currently (March 1998)
**
**                  If the cmd_type is open we will check to see if
**                  the unit number (unitNumber) is one of ours.
**                  If so, we check if it has been forwarded in which
**                  case the nid, pid, data needs attention.
**                  If it is not ours we will forward (here).
**                  --------------------------------------------  */
	    jd=FALSE;
	    processed=FALSE;

	    switch (cmd_type) {
	      case CMD_ACCESS:
	      case CMD_CHDIR:
	      case CMD_CHMOD:
	      case CMD_CHOWN:
	      case CMD_LINK:
	      case CMD_MKDIR:
	      case CMD_OPEN:
	      case CMD_RENAME:
	      case CMD_RMDIR:
	      case CMD_STAT:
	      case CMD_SYMLINK:
	      case CMD_TRUNCATE:
	      case CMD_UNLINK:
		unit = (int) cmd->unitNumber;
		/* if we dont have the requested disk unit flag the
		   command as already failed, prior to handing off
		   to the command handler
		*/
		cmd->fail = 0;
		if ( !I_have(unit) ) {
		  if ( Dbglevel > 4 ) {
		    printf("I dont have unit %d?\n", unit);
		  }
		  cmd->fail = 1;
		}
		else {
		  if ( Dbglevel > 4 ) {
		    printf("I have unit %d\n", unit);
		  }
		}
		break;
	      default:
		break;
	    }

#if 0
	    if ( cmd_type == CMD_OPEN || cmd_type == CMD_STAT || 
		 cmd_type == CMD_ACCESS ) {
	      unit = (int) cmd->unitNumber;
	      if ( Dbglevel > 2 ) {
		fprintf(stderr, "Unit Number is %d\n", unit);
		fprintf(stderr,"src_nid and pid are %d %d\n",
				src_nid, src_pid);
	      } 
	      /* if we dont have the requested disk unit flag the
		 command as already failed, prior to handing off
		 to the command handler
	      */
	      cmd->fail = 0;
	      if ( !I_have(unit) ) {
		PRINTF("I dont have unit %d?\n", unit);
		cmd->fail = 1;
	      }
	      else {
		PRINTF("I have unit %d\n", unit);
	      }
	    } /* end if */
#endif
	    host_cmd_handler_tbl[tbl_offset](&io_handle);

	    if (ioErrno) {

		fprintf(stderr,
		 "I/O error (node %d portal ID %d cmd %d): %s\n",
		      src_nid,src_pid,cmd_type,CPstrerror(ioErrno));

		ioErrno = 0;
	    }
	} 
	else { /* cmd_type out of range */
	  if ( cmd_type != 0 )  {
	    fprintf(stderr, 
	  "Warning: Unknown host cmd 0x%x, from nid %d, pid %d\n", 
	   cmd_type, src_nid, src_pid );
	     jvd_dmp((char *)cmd);
	  }
	}
	free_cmd(&io_handle);
    }
}
 
static void 
initSignals( void )
{
    int i;
    for ( i = 1; i < 32; i++ ) {	
	signal( i, cleanup );
    }
    sigignore( SIGCHLD );
    sigignore( SIGTSTP );
    sigignore( SIGCONT );
}
 
static void 
cleanup( int signum )
{
 
    if ( DBG_FLAGS( DBG_PROGRESS ) || (Dbglevel > 0) ) {
        fprintf( stderr, "pid=%i, cleanup called, sig=%i, node=%i\n",
                        getpid(), signum, _my_rank );
    }
 
    exit( 0 );
}
 
#ifdef __linux__
static void 
sigignore (int signum)
#endif
#ifdef __osf__
static int 
sigignore (int signum)
#endif
{
   signal( signum, SIG_IGN);
}
 
int 
checkArgs( int argc, char *argv[] )
{
    int i;	
 
    /* argc must be odd */	
    if ( ! ( argc % 2 ) )
	return FALSE;
 
    for ( i = 1; i < argc; i += 2 ) {
	if ( ! strcmp( argv[ i ], "-D" ) ) {
	    continue;
	} else if ( ! strcmp( argv[ i ], "-dir" ) ) {
	    continue;
	} else if ( ! strcmp( argv[ i ], "-N" ) ) {
	    continue;
	} else {
	    return FALSE;
	}
    }
    return TRUE;
}
 
static void 
jvd_dmp (char a[])
{
int i;
  for (i=0; i<45; i+=4)
 
    printf (" JVD_DMP  %2i  %2x  %2x  %2x  %2x \n", i, a[i], a[i+1],
	a[i+2], a[i+3]);
 
}


void
CMDhandler_mass_murder(control_msg_handle *mh)
{
    return;
}

#define ACK	( ack.info.hbAck )

/* I am about to put a call in here to a routine located in fyod_fyod_comm.c
	That will work for FYOD, but it feels convoluted.
			February 23, 1999   jvd   */


VOID 
CMDhandler_heartbeat( control_msg_handle *mh)
{
	hostReply_t ack;
        int i, snid, spid;
	hostCmd_t *cmd;

	snid = SRVR_HANDLE_NID(*mh);
	spid = SRVR_HANDLE_PID(*mh);
	cmd  = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

        ack.your_seq_no = cmd->my_seq_no;

    if (DBG_FLAGS(DBG_IO_1))
	{
		fprintf(stderr, "CMDhandler_heartbeat( ) node = %i, pid = %i\n",snid,spid);
	}


	for (i=0; i<n_fyod_units; i++) {
	   *(&ACK.u0+i)=fyod_units[i];
        }
	*(&ACK.u0+n_fyod_units)=-1;

	send_ack_to_app(mh, &ack);
}  

