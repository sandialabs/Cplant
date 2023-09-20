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
** $Id: pct_tvdsvr.c,v 1.9 2001/10/25 20:39:20 galagun Exp $
**
** Handle requests to start TotalView debug server 
**
*/

#include<sys/types.h>
#include<sys/wait.h>
#include<sys/param.h>
#include<sys/resource.h>
#include<unistd.h>
#include<asm/unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<string.h>
#include<time.h>
#include<malloc.h>
#include<grp.h>
#include <netinet/in.h>
#include <netdb.h>

#include"pct.h"
#include"srvr_err.h"
#include"srvr_coll.h"
#include"appload_msg.h"
#include"ppid.h"
#include"pct_ports.h"
#include"tvdsvr.h"
#ifdef KERNEL_ADDR_CACHE
#include"cache/cache.h"
#endif

extern char *scratch_loc;
extern int cache_fd;

tvdsvr_exec_info_t *tvdsvrinfo;
pid_t               tvdsvrpid = 0;


void pnid2ip(int pnid, char *ipaddr)
{
  unsigned int net[4]={10,0,0,0};
  unsigned int mask[4]={255,255,0,0}; 
  unsigned int ip;
  unsigned int nw=0, nm=0;

    nw = net[3] + 
             (net[2] << 8) + (net[1] << 16) + (net[0] << 24);

    nm = mask[3] + 
             (mask[2] << 8) + (mask[1] << 16) + (mask[0] << 24);
  
    pnid++;
    if ( pnid > ~nm ) {
      fprintf(stderr, "pnid2ip: pnid+1 0x%x overflows ~netmask 0x%x\n",
                       pnid, ~nm);
    }
    ip = nw + pnid;

    sprintf(ipaddr,"%d.%d.%d.%d", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
                          (ip >> 8 ) & 0xff,  ip & 0xff);
}



int fanout_tvdsvr_control_message(int msg_type, char *user_data, int len,
                                  unsigned int fanout_degree,
                                  int groupRoot, int groupSize)
{
    int chRank[MAX_FANOUT_DEGREE], nkids, i, rc;
 
    if (fanout_degree > MAX_FANOUT_DEGREE){
        fanout_degree = MAX_FANOUT_DEGREE;
    }
 
    for (i=0, nkids=0; i<(int) fanout_degree; i++){
        chRank[nkids] = TREE_CHILD(dsrvrMyGroupRank-groupRoot, fanout_degree, i);
        if (chRank[nkids] < groupSize){
            nkids++;
        }
    }
 
    for (i=0; i<nkids; i++){
 
        chRank[i] += groupRoot;

        rc = srvr_send_to_control_ptl(memberNidByRank(chRank[i]),
                                 memberPidByRank(chRank[i]),
                                 PCT_TVDSVR_PORTAL,
                                 msg_type, user_data, len);
 
        if (rc){
            log_warning("failure to send %x to %d/%d", msg_type,
                            memberNidByRank(chRank[i]),
                             memberPidByRank(chRank[i]));
            return -1;
        }
    }
    return 0;
}

/*
** Return -1 if PCT suffered a fatal error and must exit.
** Return 0 otherwise (even if total view startup fails).
*/

int process_tvdsvr_req(control_msg_handle *tvdsvr_req)
{
    int           msg_type, req_len;
    char          *user_data;
    proc_list     *plist;
    int            count;
    int            i, rc;
    int            ackptl, tnid, tpid, iamroot;
    char           ipaddr[32],hostport[128],password[128];

    CLEAR_ERR;

    LOCATION("process_tvdsvr_req", "top");

    msg_type = SRVR_HANDLE_TYPE(*tvdsvr_req);
    user_data = SRVR_HANDLE_USERDEF(*tvdsvr_req);

#ifdef VERBOSE
    log_msg("process_tvdsvr_req: ");
    log_msg("    msg_type => %d",msg_type);
    log_msg("    nid      => %d",SRVR_HANDLE_NID(*tvdsvr_req));
    log_msg("    pid      => %d",SRVR_HANDLE_PID(*tvdsvr_req));
#endif

    if (msg_type == TVDSVR_REQUEST_PCT_EXEC ){        /* !iamroot */
        req_len = *(int *)user_data;
	iamroot = 0;
    }
    else if (msg_type == TVDSVR_REQUEST_YOD_EXEC ){   /* iamroot */

        req_len = SRVR_HANDLE_TRANSFER_LEN(*tvdsvr_req);

        ackptl = *(int *)user_data;
	tnid = SRVR_HANDLE_NID(*tvdsvr_req);
	tpid = SRVR_HANDLE_PID(*tvdsvr_req);

	iamroot = 1;
    }
    else{
        log_msg("got strange message type %d on total view portal - ignore\n", msg_type);
	return 0;
    }
     

    /* figure out how many records there are */   
    count = req_len / sizeof(tvdsvr_exec_info_t);

#ifdef VERBOSE
    log_msg("    req_len  => %d",req_len);
    log_msg("    count    => %d",count);
#endif

    /* allocate space for them */
    if ( (tvdsvrinfo = malloc( req_len ) ) == NULL ) {
        log_warning("malloc() failed");
        req_len = 0;
        return -1;
    }

    /* get current job info */
    if ( (plist = current_proc_entry()) == NULL ) {
        log_msg("got total view request but I'm not running an app process - ignore");
	if (iamroot) {
	   srvr_send_to_control_ptl(tnid, tpid, ackptl, TVDSVR_NACK, NULL, 0);
        }
	              
        return 0;
    }

    /* I'm PCT 0, so get data from yod */
    if (iamroot){

#ifdef VERBOSE
	log_msg("    calling srvr_comm_put_reply()");
#endif
        if ( srvr_comm_put_reply( tvdsvr_req, tvdsvrinfo, req_len ) ) {
            log_warning("srvr_comm_put_reply() failed");

            if (CPerrno == ERECVTIMEOUT){

                log_msg(
		"process_tvdsvr_req: timed out waiting for start-tvdsvr records.\n");

	        srvr_send_to_control_ptl(tnid, tpid, ackptl, TVDSVR_NACK, NULL, 0);

	        return 0;    /* start-tvdsvr being bad, not my problem */
	    }
	    else{
                log_warning("process_tvdsvr_req: srvr_comm_put_reply() error");
                return -1;   /* I've got a problem */
	    }
        }
    }

#ifdef VERBOSE
	log_msg("    calling fanout_tvdsvr_control_message()");
#endif

    /* send data to the other PCT's */
    if ( fanout_tvdsvr_control_message( TVDSVR_REQUEST_PCT_EXEC,
                                        (char *)&req_len, sizeof(int),
                                        2, 0, plist->nprocs ) ) {

	log_warning("process_tvdsvr_req: error fanning out tvdsvr request");
        return -1;
    }

#ifdef VERBOSE
	log_msg("    calling dsrvr_bcast()");
#endif

    /* broadcast data to the other PCT's */
    if ( (rc = dsrvr_bcast( (char *)tvdsvrinfo, req_len, collectiveWaitLimit, 0, NULL, 0 )) ) {


        if (rc == DSRVR_EXTERNAL_ERROR){

	    if (iamroot){
	        srvr_send_to_control_ptl(tnid, tpid, ackptl, TVDSVR_NACK, NULL, 0);
	    }

	    log_msg("process_tvdsvr_req: timed out in broadcast.\n");
	    return 0;
	}
	else{
	    log_warning("process_tvdsvr_req: broadcast failure");
	    return -1;
	}
    } 

    /* find the data I need */
    for ( i=0; i<count; i++ ) {
        if ( tvdsvrinfo[i].pct_pnid == _my_pnid ) break;
    }

    /* not there? */
    if ( i == count ) {
	if (iamroot){
	    srvr_send_to_control_ptl(tnid, tpid, ackptl, TVDSVR_NACK, NULL, 0);
	}
	log_msg("tvdsvr corrupt data, ignoring");
        return 0;
    }

#ifdef VERBOSE
    log_msg("    tvdsvrinfo for pnid = %d",_my_pnid);
    log_msg("    tvdsvrinfo[%d].pct_pnid   = %d",i,tvdsvrinfo[i].pct_pnid);
    log_msg("    tvdsvrinfo[%d].host_pnid  = %d",i,tvdsvrinfo[i].host_pnid);
    log_msg("    tvdsvrinfo[%d].port       = %d",i,tvdsvrinfo[i].port);
    log_msg("    tvdsvrinfo[%d].hipassword = %x",i,tvdsvrinfo[i].hipassword);
    log_msg("    tvdsvrinfo[%d].lopassword = %x",i,tvdsvrinfo[i].lopassword);
#endif

    /* convert physical node id to Myrinet ip address */
    pnid2ip( tvdsvrinfo[i].host_pnid,ipaddr );

    sprintf(hostport,"%s:%d",ipaddr,tvdsvrinfo[i].port);
    sprintf(password,"%x:%x",tvdsvrinfo[i].hipassword,tvdsvrinfo[i].lopassword);

    log_msg("execl(%s %s -callback %s -set_pw %s",
             TVDSVR_LOC,TVDSVR_LOC,hostport,password);

#ifdef KERNEL_ADDR_CACHE
    if ( syscall(__NR_ioctl, cache_fd, CACHE_INVALIDATE, 0) <0) {
      fprintf(stderr, "pct: CACHE_INVALIDATE failed 3\n");
      exit(-1);
    } 
#endif

    if ( (tvdsvrpid = fork()) == 0 ) {

      /* child */

        if ( chdir( scratch_loc ) ) {
            log_msg("could not chdir to %s",scratch_loc);
        } else {
            log_msg("changed cwd to %s",scratch_loc);
        }

        if ( setpriority( PRIO_PROCESS, 0, -10 ) ) {
            log_msg("setpriority() failed");
        }

        if ( execl( TVDSVR_LOC,
                    TVDSVR_LOC,
                    "-callback",
                    hostport,
                    "-set_pw",
                    password,
                    NULL ) ) {
          log_error("could not start tvdsvr");
        }
    }

    free( tvdsvrinfo );

    return 0;
}


