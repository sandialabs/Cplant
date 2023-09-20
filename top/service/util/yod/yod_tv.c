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
** $Id: yod_tv.c,v 1.6 2001/12/04 20:33:05 galagun Exp $
**
** Debugging information for TotalView
**
*/

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "util.h"
#include "yod_tv.h"
#include "appload_msg.h"
#include "srvr_comm.h"
#include "pct_ports.h"

MPIR_PROCDESC      *MPIR_proctable            = 0;
int                 MPIR_proctable_size       = 0;
volatile int        MPIR_debug_state          = 0;
volatile int        MPIR_debug_gate           = 0;
char               *MPIR_debug_abort_string   = 0;
int                 MPIR_being_debugged       = 0;
int                 MPIR_i_am_starter         = 0;
char               *MPIR_dll_name             = 0;

void MPIR_Breakpoint()
{

}

/* see top/compute/OS/Myrinet/module/README for Myrinet IP addressing */
char *pnid2ip( int pnid )
{
    unsigned int net[4]={10,0,0,0};
    unsigned int mask[4]={255,255,0,0}; 
    unsigned int ipaddr;
    unsigned int nw=0, nm=0;
    char *ip;

    if ( (ip = (char *)malloc( 16 ) ) == NULL ) {
	return NULL;
    }

    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: 0x%p  (%u)  for Totalview\n",ipaddr, 16);
    }


    if ( pnid >= 1000000 ) {
	sprintf(ipaddr,"255.255.255.255");
    }

    nw = net[3] + 
             (net[2] << 8) + (net[1] << 16) + (net[0] << 24);

    nm = mask[3] + 
             (mask[2] << 8) + (mask[1] << 16) + (mask[0] << 24);
  
    pnid++;
    if ( pnid > ~nm ) {
      fprintf(stderr, "pnid2ip: pnid+1 0x%x overflows ~netmask 0x%x\n",
                       pnid, ~nm);
    }
    ipaddr = nw + pnid;

    sprintf(ip,"%d.%d.%d.%d", (ipaddr >> 24) & 0xff, (ipaddr >> 16) & 0xff,
                          (ipaddr >> 8 ) & 0xff,  ipaddr & 0xff);

    return ip;
}

#ifdef _FOR_LIBJOB 
void init_TotalView_symbols( int nprocs, int *nmap, spid_type *pidmap, 
                             loadMbrs *mbrs, int nmembers)
{
    loadMbrs *mbr = mbrs;
    loadMbrs *lastmbr;

#else
void init_TotalView_symbols( int nprocs, int *nmap, spid_type *pidmap )
{
    loadMembers *mbr,*lastmbr;
#endif
    char        *lastslash,*filename;
    int          i;

    if ( (MPIR_proctable = (MPIR_PROCDESC *)malloc( nprocs * sizeof(MPIR_PROCDESC) )) ) {
        lastmbr = NULL;

	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: 0x%p  (%u)  for Totalview table\n",
                      MPIR_proctable,nprocs * sizeof(MPIR_PROCDESC));
	}

	for ( i=0; i<nprocs; i++ ) {

#ifdef _FOR_LIBJOB
	    if (mbr->data.toRank < i){
                 mbr++;
            }
#else
            mbr = which_member( i );
#endif

            if ( mbr != lastmbr ) {
                if ( (lastslash = strrchr( mbr->pname, SLASH )) ) {
                    filename = lastslash + 1;
                } else {
                    filename = mbr->pname;
                }
            }
            lastmbr = mbr;
	    MPIR_proctable[i].host_name       = pnid2ip(nmap[i]);
	    MPIR_proctable[i].executable_name = filename;
	    MPIR_proctable[i].pid             = pidmap[i];
	}
	MPIR_proctable_size = nprocs;
    }
}

void dump_mpir_proctable( int nprocs )
{
    int i;

    for ( i=0; i<nprocs; i++ ) {
	printf("Rank            = %d\n",i);
	printf("Host name       = %s\n",MPIR_proctable[i].host_name);
	printf("Executable name = %s\n",MPIR_proctable[i].executable_name);
	printf("Pid             = %d\n",MPIR_proctable[i].pid);
	printf("---\n");
    }
}

