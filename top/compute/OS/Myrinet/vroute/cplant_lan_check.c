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
    $Id: cplant_lan_check.c,v 1.3.2.1 2002/04/08 18:18:18 jbogden Exp $
    
    cplant_lan_check.c
    
    Provides the functionality of Nathan Dauchy's gm_lan_check program
    in the Cplant environment.  When run on a Myrinet node, the node will
    generate a bunch of traffic to itself (rtscts test route packets)
    and check the MCP error stats before and after for trouble. This is
    used to help isolate bad Myrinet cables, NICs, and switch ports.
    
    cplant_lan_check -d <debug level> -n <num pkts> -s <size of payload>
    
    Debug levels are:   0 (default) - only print if there are errors
                        1 - print some of the interesting non-error stuff
                        2 - print everything!
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cTask/cTask.h>
#include "RTSCTS_ioctl.h"
#include "common.h"
#include "vping.h"
#include "defines.h"
#include "lanai_device.h"



static mcpshmem_t *mcpshmem = NULL;
static counters_t counters_before;
static counters_t counters_after;
rts_ioctl_arg_t rts_ioctl_arg;
int mynid = -1;
char hostname[256];

void
usage()
{
    fprintf(stderr,"Usage: cplant_lan_check -d [0,1,2] -n <num pkts> -s <payload size> -f\n");
    fprintf(stderr,"       -d 0 -n 1000 -s 1024 is the default\n"); 
    fprintf(stderr,"       -f enables fast sending (don't wait for acks)\n");                
}

int
debug_print(char *fmt,...) {
    va_list ap;
    
    printf("%d %s ",mynid,hostname);
    
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
    
    return 0;
}

#define PRINTF      debug_print
#define MAX_SIZE    8000

void
printdiff(char *ident,unsigned long before, unsigned long after) {
    if (after > before)
        PRINTF("%s increased: %d\n",ident,after-before);
    else if (before > after)
        PRINTF("%s decreased: %d\n",ident,before-after);
	else
        PRINTF("%s unchanged: %d\n",ident,before-after);

}


/* some timeout values */
#define TIMEOUT		     2   /* in seconds */
#define MAX_TIMEOUTS     5   /* number of timed out packets */


int main(int argc, char *argv[])
{
    int fdr, fdc, rc, clock, btype;
    int unit = 0;
    int verbose = 1; 
    int num_pkts = 1000;
    int payload_size = 1024;
    int fast_send = 0;
    int route_len;
    int i;
    char route[1024];
    int newer_rtscts = 0;
    int ch;
    extern char *optarg;
    extern int optind;
    unsigned long total_time = 0;
    unsigned long total_ack = 0;
	int num_timeout = 0;


    while ((ch= getopt(argc, argv, "d:n:s:f")) != EOF)   {
        switch (ch) {
            case 'd':
                verbose = atoi(optarg);
                break;
            case 'n':
                num_pkts = atoi(optarg);
                break;
            case 's':
                payload_size = atoi(optarg);
                if (payload_size > MAX_SIZE)
                    payload_size = MAX_SIZE;
                break;
            case 'f':
                fast_send = 1;
                break;
            default:
                usage();
                exit(-1);
        }
    }

    /* route_len is 1 since we just want to bounce off the switch to ourself
     * and the route is 0x80 the self route
    */
    route_len = 1;
    route[0] = 0x80;

    /* open cTask device */
    if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
        printf("cplant_lan_check: failed to open /dev/cTask...\n");
        exit(-1);
    }
    mynid = ioctl(fdc, CPL_GET_PHYS_NID, 0L);

    if (mynid < 0) {
        printf("cplan_lan_check: failed to get physical node id...\n");
        exit(-1);
    }

    /* figure out our hostname */
    rc = gethostname(hostname,256);
    if (rc != 0) {
        printf("cplant_lan_check: failed to get hostname...\n");
        exit(-1);
    }
    
    /* map the lanai board -- we want to refer to 
       mcpshmem's ping status entry
    */
    if (verbose == 2) PRINTF("cplant_lan_check: ");
    if (map_lanai(argv[0], (verbose == 0? verbose:verbose-1), unit) != OK) {
        PRINTF("cplant_lan_check: could not map_lanai...\n");
        exit(-1);
    }

    if (verbose == 2) {
        PRINTF("cplant_lan_check: lanai type is ");
        btype = get_lanai_type(unit, TRUE);
        printf("\n");
    }
    else {
        btype = get_lanai_type(unit, FALSE);
    }

    if (verbose == 2) PRINTF("cplant_lan_check: ");
    if ( (mcpshmem = get_mcpshmem( (verbose == 0? verbose:verbose-1), unit, argv[0], FALSE)) 
                                                         == NULL) {
        PRINTF("cplant_lan_check: could not get_mcpshmem...\n");
        exit(-1);
    }
    
    if (verbose == 2) {
        PRINTF("verbose=%d  num_pkts=%d  payload_size=%d bytes  fast_send=%d\n",
               verbose,num_pkts,payload_size,fast_send);
    }
    else if (fast_send) {
        PRINTF("FAST SENDS ARE ENABLED\n");
    }

    /* open rtscts device */
    if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
        PRINTF("cplant_lan_check: failed to open /dev/rtscts...\n");
        exit(-1);
    }

    /* set route len */
    rc = ioctl(fdr, RTS_SET_TEST_ROUTE_LEN, (unsigned long) route_len);
    if ( rc < 0 ) {
        PRINTF("cplant_lan_check: ioctl(RTS_SET_TEST_ROUTE_LEN,%d) failed\n", route_len);
        exit(-1);
    }

    /* Let's probe the rtscts ioctl() and see if it supports the
     * RTS_DO_TEST_ROUTE_W_SIZE command which allows us to send
     * test route packets with some size!
    */
    rts_ioctl_arg.type = 1;
    rc = ioctl(fdr, RTS_DO_TEST_ROUTE_W_SIZE, (unsigned long)&rts_ioctl_arg);
    if (rc == 10) {
        /* It is supported! */
        newer_rtscts = 1;
        if (verbose == 2) PRINTF("Using new RTS_DO_TEST_ROUTE_W_SIZE ioctl() command.\n");
        
        rts_ioctl_arg.type = 2;
        rts_ioctl_arg.buflen = payload_size;
        rts_ioctl_arg.route[0] = 0x80;
    }
    else {
        newer_rtscts = 0;
        /* we only get to send rtscts packets with just the header */
        payload_size = 56;
        if (verbose == 2) PRINTF("Using old RTS_DO_TEST_ROUTE ioctl() command.\n");
    }
    
    /* get the MCP counters before we start */
    if (verbose == 2) PRINTF("Grabbing MCP counters before the flood...\n");
    memcpy(&counters_before,&mcpshmem->counters,sizeof(counters_t));

    if (verbose == 2) PRINTF("Flooding with %ld packets for %ld total bytes...\n",
                        (unsigned long)num_pkts,(unsigned long)(num_pkts*payload_size));

    for (i=0; i<num_pkts; i++) {    
        if (newer_rtscts == 0) {
            /* ping -- this call will copy the route into host shared memory */
            rc = ioctl(fdr, RTS_DO_TEST_ROUTE, (unsigned long) route);
            if ( rc < 0 ) {
                PRINTF("cplant_lan_check: ioctl(RTS_DO_TEST_ROUTE) failed\n");
                exit(-1);
            }
        }
        else {
            rc = ioctl(fdr, RTS_DO_TEST_ROUTE_W_SIZE, (unsigned long)&rts_ioctl_arg);
            if ( rc < 0 ) {
                PRINTF("cplant_lan_check: ioctl(RTS_DO_TEST_ROUTE_W_SIZE) failed\n");
                exit(-1);
            }        
        }

        if (!fast_send) {        
            rc = -1;
            clock = 0;
            setRTC(unit,btype,0);
            while ( rc == -1 && clock < TIMEOUT*ONE_SECOND ) {
                rc = (int)ntohl(mcpshmem->ping_stat);
                clock = getRTC(unit,btype);
            }

            if ( rc == -1 ) {
                /* PRINTF("FAILED -- no result after %d usecs\n", clock/2); */
                num_timeout++;
                if (num_timeout > MAX_TIMEOUTS) {
                    PRINTF("FAILED - exceeded %d timed out packets sending!\n",
                           MAX_TIMEOUTS);
                    PRINTF("The link seems dead, cable plugged in?\n");
                    exit(-1);
                }
            }
            else if ( rc != mynid ) {
                PRINTF("FAILED -- got ping response with bad node id %d\n", rc);
                exit(-1);
            }
            else {
                total_ack++;
                total_time += clock/2;
            }
        }

    }

    /* get the MCP counters after we finish */
    if (verbose == 2) PRINTF("Grabbing MCP counters after the flood...\n");
    memcpy(&counters_after,&mcpshmem->counters,sizeof(counters_t));

    /* compare the before and after MCP counter snapshots */
    if (ntohl(counters_after.fres) != ntohl(counters_before.fres)) {
        printdiff("network resets      ",(unsigned long)ntohl(counters_before.fres),(unsigned long)ntohl(counters_after.fres));
    }
    if (ntohl(counters_after.reset) != ntohl(counters_before.reset)) {
        printdiff("LANai resets        ",(unsigned long)ntohl(counters_before.reset),(unsigned long)ntohl(counters_after.reset));
    }
    if (ntohl(counters_after.crc) != ntohl(counters_before.crc) || verbose) {
        printdiff("crc errors          ",(unsigned long)ntohl(counters_before.crc),(unsigned long)ntohl(counters_after.crc));
    }
    if (ntohl(counters_after.dumped_crc) != ntohl(counters_before.dumped_crc)) {
        printdiff("crc errors on dumped",(unsigned long)ntohl(counters_before.dumped_crc),(unsigned long)ntohl(counters_after.dumped_crc));
    }
    if (ntohl(counters_after.truncated) != ntohl(counters_before.truncated)) {
        printdiff("truncated           ",(unsigned long)ntohl(counters_before.truncated),(unsigned long)ntohl(counters_after.truncated));
    }
    if (ntohl(counters_after.toolong) != ntohl(counters_before.toolong)) {
        printdiff("toolong             ",(unsigned long)ntohl(counters_before.toolong),(unsigned long)ntohl(counters_after.toolong));
    }
    if (ntohl(counters_after.send_timeout) != ntohl(counters_before.send_timeout)) {
        printdiff("send timeout        ",(unsigned long)ntohl(counters_before.send_timeout),(unsigned long)ntohl(counters_after.send_timeout));
    }
    if (ntohl(counters_after.link2) != ntohl(counters_before.link2)) {
        printdiff("link problems       ",(unsigned long)ntohl(counters_before.link2),(unsigned long)ntohl(counters_after.link2));
    }

    if (ntohl(counters_after.sends) != ntohl(counters_before.sends) || verbose) {
        printdiff("sends               ",(unsigned long)ntohl(counters_before.sends),(unsigned long)ntohl(counters_after.sends));
    }
    if (ntohl(counters_after.rcvs) != ntohl(counters_before.rcvs) || verbose) {
        printdiff("rcvs                ",(unsigned long)ntohl(counters_before.rcvs),(unsigned long)ntohl(counters_after.rcvs));
    }
    if (ntohl(counters_after.sends) != ntohl(counters_before.rcvs) && verbose && fast_send) {
       PRINTF("Sent packets are different from received packets!\n");
       PRINTF("If there were no hst_dly or dumped increases, this could\n");
       PRINTF("be an indication of a bad link which could be the cable.\n");
    }

    if (ntohl(counters_after.hst_dly) != ntohl(counters_before.hst_dly) && (verbose==2 || fast_send==0)) {
        PRINTF("Host delay, dumped, & num bytes dumped errors indicate the rtscts module and Linux could not\n");
        PRINTF("keep up with the rate of receives. If you flooded with a bunch of packets (>9000 maybe???), then\n");
        PRINTF("this should be normal. It's not likely to be a hardware problem. If fast sends were disabled,\n");
        PRINTF("these errors could indicate trouble.\n");
        printdiff("host delay          ",(unsigned long)ntohl(counters_before.hst_dly),(unsigned long)ntohl(counters_after.hst_dly));
    }
    if (ntohl(counters_after.dumped) != ntohl(counters_before.dumped) && (verbose==2 || fast_send==0)) {
        printdiff("dumped              ",(unsigned long)ntohl(counters_before.dumped),(unsigned long)ntohl(counters_after.dumped));
    }
    if (ntohl(counters_after.dumped_num) != ntohl(counters_before.dumped_num) && (verbose==2 || fast_send==0)) {
        printdiff("num bytes dumped    ",(unsigned long)ntohl(counters_before.dumped_num),(unsigned long)ntohl(counters_after.dumped_num));
    }

    if (ntohl(counters_after.wrngprtcl) != ntohl(counters_before.wrngprtcl) && verbose == 2) {
        printdiff("wrong protocol      ",(unsigned long)ntohl(counters_before.wrngprtcl),(unsigned long)ntohl(counters_after.wrngprtcl));
    }
    if (ntohl(counters_after.MyriData) != ntohl(counters_before.MyriData) && verbose == 2) {
        printdiff("MyriData packets    ",(unsigned long)ntohl(counters_before.MyriData),(unsigned long)ntohl(counters_after.MyriData));
    }
    if (ntohl(counters_after.MyriMap) != ntohl(counters_before.MyriMap) && verbose == 2) {
        printdiff("MyriMap packets     ",(unsigned long)ntohl(counters_before.MyriMap),(unsigned long)ntohl(counters_after.MyriMap));
    }
    if (ntohl(counters_after.MyriProbe) != ntohl(counters_before.MyriProbe) && verbose == 2) {
        printdiff("MyriProbe packets   ",(unsigned long)ntohl(counters_before.MyriProbe),(unsigned long)ntohl(counters_after.MyriProbe));
    }
    if (ntohl(counters_after.MyriOption) != ntohl(counters_before.MyriOption) && verbose == 2) {
        printdiff("MyriOption packets  ",(unsigned long)ntohl(counters_before.MyriOption),(unsigned long)ntohl(counters_after.MyriOption));
    }

    if (ntohl(counters_after.mem_parity) != ntohl(counters_before.mem_parity)) {
        printdiff("parity in LANai mem ",(unsigned long)ntohl(counters_before.mem_parity),(unsigned long)ntohl(counters_after.mem_parity));
    }
    
    if (total_ack != num_pkts  &&  !fast_send) {
        printdiff("ping acks           ",num_pkts,total_ack);
    }
    
    if (verbose == 2) {
        if (total_ack != 0)
            PRINTF("average ping time   : %f usec\n",(double)total_time/(double)total_ack);
        else if (!fast_send)
            PRINTF("average ping time   :  There were no acks!!\n");
    }
    
    return 0;
}

