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
** $Id: start-tvdsvr.c,v 1.10 2002/02/21 21:20:38 galagun Exp $
**
** Helper application to start TotalView debug server on compute nodes
**
*/

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

#include"srvr_comm.h"
#include "srvr_err.h"
#include"appload_msg.h"
#include"pct_ports.h"
#include"ppid.h"
#include"tvdsvr.h"

#define COLON 58
#define SLASH "/"


/* wait for the message to be read by the PCT */
int wait_put_message(int bufnum, int tmout, int howmany)
{
    struct timeval tv;
    long t1;
    int rc;

    if (tmout){
        gettimeofday(&tv, NULL);
        t1 = tv.tv_sec;
    }

    while (1){
        rc = srvr_test_read_buf(bufnum, howmany);
        if (rc < 0){
            return -1;
        }

        if (rc == 1) break;  /* all pcts have pulled data */

        if (tmout){ 

             gettimeofday(&tv, NULL);

             if ((tv.tv_sec - t1) > tmout){
                 fprintf(stderr, "timed out waiting for pcts (%d seconds)\n", tmout);
                 return -1;
             }
        }
    }

    return 0;
}


int main( int ac, char **av )
{

    FILE                *file;
    char                *lineptr[1];
    char                *hname;
    char                *passwd;
    char                *colon;
    tvdsvr_exec_info_t  *tvdsvrinfo;
    struct hostent      *hostentp;
    char               **p;
    int                  size;
    int                  i, rc;
    unsigned int         hi,lo;
    int                  port;
    int                  bufnum;
    int                  ctlptl;
    int                  verbose;
    struct in_addr       in;
    char                 line[256];
    char                 hoststr1[128];
    char                 hoststr2[128];
    int                  pctnid, pctpid, pctptl;
    unsigned int         ip;
    unsigned int         netmask = 0xff000000;

    if ( (_my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_TVDSVR,
                                            "start-tvdsvr" )) == 0 ) {
        fprintf(stderr,"register_ppid() failed\n");
        return -1;
    }
    rc = server_library_init();

    if (rc){
        fprintf(stderr,"initializing server library");
        return -1;
    }


    if ( ac < 2 ) {
	fprintf(stderr,"usage: %s filename [verbose]\n",av[0]);
	return(-1);
    }

    verbose = (int)ac == 3;

    /* open TV temp file */
    if ( verbose ) {
      printf("Openining file %s\n",av[1]);
      
 }
    if ( (file = fopen( av[1], "r" )) == NULL ) {
	perror("fopen() failed");
	return errno;
    }

    /* read the number of nodes */
    fscanf(file,"%d",&size);
    if ( verbose ) {
        printf("size = %d\n",size);
    }

    /* allocate tvdsvr info */
    if ( (tvdsvrinfo = (tvdsvr_exec_info_t *)malloc( size * sizeof(tvdsvr_exec_info_t) ) ) ==
	 NULL ) {
	fprintf(stderr,"malloc() failed\n");
	return -1;
    }

    /* read one line for each node */
    for ( i=0; i<size; i++ ) {
    
	if ( fscanf(file,"%s",line) != 1 ) {
	    perror("fscanf() failed");
	    return errno;
	}

	if ( verbose ) {
	  printf("line = %s\n",line);
	}

	*lineptr = line;
	hname = strsep( lineptr, SLASH );
        strcpy(hoststr1,hname);
	passwd = strsep( lineptr, SLASH );
	sscanf(passwd,"%x:%x",&hi,&lo);
	hname = strsep( lineptr, SLASH );
	if (strlen(hname) == 0) {
	  port = 4142;
	} else {
	  colon = rindex( hname, COLON );
	  bzero( hoststr2, sizeof(hoststr2) );
	  strncpy( hoststr2, hname, (int)(colon-hname) );
	  colon++;
	  sscanf( colon,"%d",&port );
	}

        if ( (hostentp = gethostbyname( hoststr1 ) ) == NULL ) {
            perror("gethostbyname() failed\n");
            return errno;
        }
        p = hostentp->h_addr_list;
        memcpy(&in.s_addr, *p, sizeof(in.s_addr));

	ip = (unsigned int) ntohl(in.s_addr);

	tvdsvrinfo[i].pct_pnid = (int) ((ip & ~netmask)-1);
        tvdsvrinfo[i].host_pnid  = _my_pnid;
	tvdsvrinfo[i].port       = port;
	tvdsvrinfo[i].hipassword = hi;
	tvdsvrinfo[i].lopassword = lo;

        if ( verbose ) {
	    printf("tvdsvrinfo[%d].pct_pnid   = %d\n",i,tvdsvrinfo[i].pct_pnid);
	    printf("tvdsvrinfo[%d].host_pnid  = %d\n",i,tvdsvrinfo[i].host_pnid);
	    printf("tvdsvrinfo[%d].port       = %d\n",i,tvdsvrinfo[i].port);
	    printf("tvdsvrinfo[%d].hipassword = %x\n",i,tvdsvrinfo[i].hipassword);
	    printf("tvdsvrinfo[%d].lopassword = %x\n",i,tvdsvrinfo[i].lopassword);
        }
    }

    /* close the temp file */
    fclose( file );

    /*
    ** create a control portal for pct's ack
    */
    ctlptl = srvr_init_control_ptl(1);

    if (ctlptl == SRVR_INVAL_PTL){
	fprintf(stderr,"srvr_init_control_ptl (%s)\n",CPstrerror(CPerrno));
	return -1;
    }

    /* send the info up the first PCT */

    pctnid = tvdsvrinfo[0].pct_pnid;
    pctpid = PPID_PCT;
    pctptl = PCT_TVDSVR_PORTAL;

    bufnum = srvr_comm_put_req( (char *)tvdsvrinfo,
                                     size * sizeof(tvdsvr_exec_info_t),
                                     TVDSVR_REQUEST_YOD_EXEC,
                                     (char *)&ctlptl, sizeof(int),
                                     1, &pctnid, &pctpid, &pctptl);

    if (bufnum < 0){
        fprintf(stderr,"srvr_comm_put_req() failed\n");
        return -1;
    }

    if ( wait_put_message( bufnum, 10, 1 ) ) {
        fprintf(stderr,"Can't contact PCT\n");  /* didn't pick up tvdsvrinfo */
	return -1;
    }

    srvr_release_control_ptl(ctlptl);

    return 0;
}

