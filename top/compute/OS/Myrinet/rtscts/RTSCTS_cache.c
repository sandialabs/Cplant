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
** $Id: RTSCTS_cache.c,v 1.6 2001/08/22 16:29:06 pumatst Exp $
** The functions to retrieve cache info from another node on the Myrinet
*/
#include <linux/kernel.h>	/* For printk() etc. */
#include <asm/uaccess.h>	/* For copy_from_user() */
#include <sys/defines.h>	/* For MIN() */ 
#include "MCPshmem.h"		/* For MAX_NUM_ROUTES */
#include "Pkt_proc.h"		/* For proc_stat_t */
#include "Pkt_module.h"		/* For MYRPKT_MTU */
#include "RTSCTS_proc.h"	/* For rtscts_stat */
#include "RTSCTS_send.h"	/* For sendProtoMSG(), next_msgID() */
#include "RTSCTS_protocol.h"	/* For INFO_REQ */
#include "RTSCTS_cache.h"	/* fn prototypes */

static int cache_requestor=0;
static int ready_for_cache_data=0;
static int cache_data_ready=0;

#ifdef DO_WE_NEED_THIS
    static addr_entry_t addrlist[ADDR_LIMIT];
    static addr_entry_t* usr_addrlist;
    static int num_addresses;
#endif /* DO_WE_NEED_THIS */

static addr_summary_t addrSummary;
static addr_summary_t* usr_addrSummary;

int
send_cache_req(int dest)
{
    sendProtoMSG(CACHE_REQ, next_msgID(), (unsigned short) dest, 
                                                      0, 0, 0, NULL);
    ready_for_cache_data=1;
    return 0;
}

void
handleCacheReq(int src_nid)
{
  cache_requestor = src_nid;
  return;
} 

void
handleCacheData(int src_nid, char *data)
{
  if (!ready_for_cache_data) {
    return; 
  }
  memcpy( (void*)&addrSummary, (void*)data, sizeof(addr_summary_t));

  cache_data_ready = 1;
  ready_for_cache_data = 0;
  return;
}

int send_cache_data(addr_summary_t* addrSummary_in)
{
  if (!cache_requestor) {
    //printk("send_cache_data: ERROR -- no requestor registered\n");
    return -1;
  }
  sendProtoMSG(CACHE_DATA, next_msgID(), (unsigned short) cache_requestor, 
             (unsigned int) sizeof(addr_summary_t),
             0, 
             (         int) sizeof(addr_summary_t),
             (void*) addrSummary_in);
  return 0;
}

int retrieve_cache_data(unsigned long where)
{
  int rc;

  if (!cache_data_ready) {
    return -1;
  }

  usr_addrSummary = (addr_summary_t*) where; 

  rc = copy_to_user((void*)usr_addrSummary, (void*) &addrSummary,
                                   sizeof(addr_summary_t));

  usr_addrSummary = NULL;

  cache_data_ready = 0;
  ready_for_cache_data = 1;
  return 0;
}

