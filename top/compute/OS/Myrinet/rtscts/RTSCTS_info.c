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
** $Id: RTSCTS_info.c,v 1.21.2.1 2002/05/22 21:40:02 jbogden Exp $
** The functions to retrieve information from another node on the Myrinet
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
#include "RTSCTS_route.h"	/* For route_status_t */
#include "RTSCTS_info.h"


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Globals
** !!! Note, at most one request on a given node may be pending at a time.
** !!! e.g. At most one ioctl(RTS_REQ_INFO) before a successful
** !!! ioctl(RTS_GET_INFO) on a given node.
*/
static get_info_req_t get_info= {0, 0, SEG_ERROR, NULL, 0, 0, TRUE};
static seg_error_t seg_error;
static mcp_stat_t mcp_stat;
static eventlog_t seg_event;


/* >>>  ----------------------------------------------------------------- <<< */

int
send_info_req(void *uptr)
{

int rc;


    rc = copy_from_user(&get_info, uptr, sizeof(get_info)); 
    if ( rc < 0) {
	printk("send_info_req(): copy failed\n");
	return EFAULT;
    }

    if ((get_info.node < 0) || (get_info.node >= MAX_NUM_ROUTES))   {
	printk("send_info_req() Illegal destination %d\n", get_info.node);
	return EINVAL;
    }

    if ((get_info.seg < SEG_ROUTE_USAGE) || (get_info.seg >= SEG_SENTINEL))   {
	printk("send_info_req() invalid segment %d requested\n", get_info.seg);
	return EINVAL;
    }

    get_info.pid= current->pid;
    get_info.msgID= next_msgID();
    get_info.done= FALSE;
    get_info.rcvd= 0;
    #ifdef VERBOSE
	printk("send_info_req() pid %d, msgID 0x%08x\n", get_info.pid,
	    get_info.msgID);
    #endif /* VERBOSE */

    sendProtoMSG(INFO_REQ, get_info.msgID, get_info.node, get_info.seg,
		    get_info.clear, 0, NULL);

    return 0;

}  /* end of send_info_req() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handleInfoReq(int src_nid, unsigned int msgID, seg_info_t seg, int clear)
{

void *buf;
long int len;
unsigned int offset;
unsigned int remainder;
int sent;
int i;


    #ifdef VERBOSE
	printk("handleInfoReq() src_nid %d, msgID 0x%08x, seg %d\n", src_nid,
	    msgID, seg);
    #endif /* VERBOSE */

    /* Grab the requested data and send it back to the requester */
    buf= NULL;
    len= 0;
    switch (seg)   {
    case SEG_PING:
        buf = NULL;
        len = 0;
        break;
	case SEG_ROUTE_USAGE:
	    buf= route_used;
	    len= sizeof(route_used);
	    break;
	case SEG_RTSCTS:
	    buf= rtscts_stat;
	    len= sizeof(rtscts_stat_t);
	    break;
	case SEG_MYRPKT:
	    buf= proc_stat;
	    len= sizeof(proc_stat_t);
	    break;
	case SEG_MCP:
	    mcp_stat.mcp_fres= ntohl(mcpshmem->counters.fres);
	    mcp_stat.mcp_reset= ntohl(mcpshmem->counters.reset);
	    mcp_stat.mcp_hst_dly= ntohl(mcpshmem->counters.hst_dly);
	    mcp_stat.mcp_crc= ntohl(mcpshmem->counters.crc);
	    mcp_stat.mcp_truncated= ntohl(mcpshmem->counters.truncated);
	    mcp_stat.mcp_toolong= ntohl(mcpshmem->counters.toolong);
	    mcp_stat.mcp_send_timeout= ntohl(mcpshmem->counters.send_timeout);
	    mcp_stat.mcp_link2= ntohl(mcpshmem->counters.link2);
	    mcp_stat.mcp_mem_parity= ntohl(mcpshmem->counters.mem_parity);

	    buf= &mcp_stat;
	    len= sizeof(mcp_stat_t);
	    break;
	case SEG_ERROR:
	    /* Fill seg_error with current error information */
	    seg_error.module_send_errs= proc_stat->snd_dev_closed +
		proc_stat->snd_MCP_busy + proc_stat->snd_err_not_sending +
		proc_stat->snd_err_len_mtu + proc_stat->snd_err_idx +
		proc_stat->snd_err_len + proc_stat->snd_err_dest +
		proc_stat->snd_len + proc_stat->snd_err_internal +
		proc_stat->snd_err_internal2 + proc_stat->snd_err_overrun;
        
	    seg_error.module_recv_errs= proc_stat->rcv_len1 +
		proc_stat->rcv_len2 + proc_stat->rcv_len3 +
		proc_stat->rcv_len4 + proc_stat->rcv_len5 +
		proc_stat->rcv_page_alloc_fail +
		proc_stat->rcv_dropped + proc_stat->rcv_dropped_crc +
		proc_stat->rcv_dropped_trunc;
	    
        seg_error.module_badcpy= rtscts_stat->badcpy;
	    
        seg_error.module_sendErr= rtscts_stat->sendErr5 +
		rtscts_stat->sendErr6 + rtscts_stat->sendErr7 +
		rtscts_stat->sendErr8 + rtscts_stat->sendErr9 +
        rtscts_stat->sendErr10;

        seg_error.module_recvErr = rtscts_stat->P3parsebad;
        	    
        seg_error.module_protErr= rtscts_stat->protoErr3 +
		rtscts_stat->protoErr4 + rtscts_stat->protoErr5 +
		rtscts_stat->protoErr6 + rtscts_stat->protoErr9 +
		rtscts_stat->protoGCHdoneErr;
	    #ifdef DO_TIMEOUT_PROTOCOL
		seg_error.module_protErr +=
		    rtscts_stat->protoErr15 + rtscts_stat->protoErr16 +
		    rtscts_stat->protoErr17 + rtscts_stat->protoErr18 +
		    rtscts_stat->protoErr19 + rtscts_stat->protoErr21;
	    #endif /* DO_TIMEOUT_PROTOCOL */
	    
        seg_error.module_outstanding_pkts= rtscts_stat->outstanding_pkts;
	    
        seg_error.module_neterrs= rtscts_stat->badseq +
		rtscts_stat->protoWrongMsgNum;

	    buf= &seg_error;
	    len= sizeof(seg_error_t);
	    break;
	case SEG_EVENTS:
	    seg_event.event_num= ntohl(mcpshmem->event_num);
	    seg_event.eventlog_next= ntohl(mcpshmem->eventlog_next);
	    for (i= 0; i < EVENT_MAX; i++)   {
		seg_event.eventlog[i].t0= ntohl(mcpshmem->eventlog[i].t0);
		seg_event.eventlog[i].t1= ntohl(mcpshmem->eventlog[i].t1);
		seg_event.eventlog[i].len= ntohl(mcpshmem->eventlog[i].len);
		seg_event.eventlog[i].mcp_event= ntohl(mcpshmem->eventlog[i].mcp_event);
		seg_event.eventlog[i].RMPvalue= ntohl(mcpshmem->eventlog[i].RMPvalue);
		seg_event.eventlog[i].RMLvalue= ntohl(mcpshmem->eventlog[i].RMLvalue);
		seg_event.eventlog[i].word0= ntohl(mcpshmem->eventlog[i].word0);
		seg_event.eventlog[i].word1= ntohl(mcpshmem->eventlog[i].word1);
		seg_event.eventlog[i].word2= mcpshmem->eventlog[i].word2;
		seg_event.eventlog[i].word3= mcpshmem->eventlog[i].word3;
		seg_event.eventlog[i].word4= mcpshmem->eventlog[i].word4;
		seg_event.eventlog[i].word5= mcpshmem->eventlog[i].word5;
		seg_event.eventlog[i].word6= mcpshmem->eventlog[i].word6;
		seg_event.eventlog[i].word7= mcpshmem->eventlog[i].word7;
		seg_event.eventlog[i].word8= mcpshmem->eventlog[i].word8;
		seg_event.eventlog[i].word9= mcpshmem->eventlog[i].word9;
		seg_event.eventlog[i].word10= mcpshmem->eventlog[i].word10;
		seg_event.eventlog[i].word11= mcpshmem->eventlog[i].word11;
		seg_event.eventlog[i].rcvs= ntohl(mcpshmem->eventlog[i].rcvs);
		seg_event.eventlog[i].snds= ntohl(mcpshmem->eventlog[i].snds);
		seg_event.eventlog[i].isr= ntohl(mcpshmem->eventlog[i].isr);
	    }
	    buf= &seg_event;
	    len= sizeof(eventlog_t);
	    break;
	case SEG_SENTINEL:
	    printk("handleInfoReq() Illegal segment request!\n");
	    return;
    }

    if (len == 0  &&  buf == NULL) {
        /* send a response quick, i.e. a SEG_PING */
        sendProtoMSG(INFO_DATA, msgID, src_nid, 0, 0, 0, NULL); 
    }
    else {
        remainder= len;
        offset= 0;
        while (remainder > 0)   {
	    sent= MIN(remainder, MYRPKT_MTU - sizeof(pkthdr_t));
	    sendProtoMSG(INFO_DATA, msgID, src_nid, offset, sent, sent, buf);
	    #ifdef VERBOSE
	        printk("handleInfoReq() 0x%08x sent %d, offset %d\n", msgID, sent,
		    offset);
	    #endif /* VERBOSE */
	    offset += sent;
	    remainder -= sent;
	    buf= (void *)((char *)buf + sent);
        }
    }

    if (clear)   {
	switch (seg)   {
	    case SEG_ROUTE_USAGE:
		routes_usedProcInit();
		break;
	    case SEG_RTSCTS:
		rtsctsProcInit();
		break;
	    case SEG_MYRPKT:
		myrPktProcInit();
		break;
	    case SEG_MCP:
		mcp_stat.mcp_fres= ntohl(mcpshmem->counters.fres);
		mcp_stat.mcp_reset= ntohl(mcpshmem->counters.reset);
		mcp_stat.mcp_hst_dly= ntohl(mcpshmem->counters.hst_dly);
		mcp_stat.mcp_crc= ntohl(mcpshmem->counters.crc);
		mcp_stat.mcp_truncated= ntohl(mcpshmem->counters.truncated);
		mcp_stat.mcp_toolong= ntohl(mcpshmem->counters.toolong);
		mcp_stat.mcp_send_timeout= ntohl(mcpshmem->counters.send_timeout);
		mcp_stat.mcp_link2= ntohl(mcpshmem->counters.link2);
		mcp_stat.mcp_mem_parity= ntohl(mcpshmem->counters.mem_parity);

		buf= &mcp_stat;
		len= sizeof(mcp_stat_t);
		break;
	    case SEG_ERROR:
		proc_stat->snd_dev_closed =
		    proc_stat->snd_MCP_busy = proc_stat->snd_err_not_sending =
		    proc_stat->snd_err_len_mtu = proc_stat->snd_err_idx =
		    proc_stat->snd_err_len = proc_stat->snd_err_dest =
		    proc_stat->snd_len = proc_stat->snd_err_internal =
		    proc_stat->snd_err_internal2 = proc_stat->snd_err_overrun= 0;
		proc_stat->rcv_len1 =
		    proc_stat->rcv_len2 = proc_stat->rcv_len3 =
		    proc_stat->rcv_len4 = proc_stat->rcv_page_alloc_fail =
		    proc_stat->rcv_dropped = proc_stat->rcv_dropped_crc =
		    proc_stat->rcv_dropped_trunc= 0;
		rtscts_stat->badcpy= 0;
		rtscts_stat->sendErr5 =
		    rtscts_stat->sendErr6 = rtscts_stat->sendErr7 =
		    rtscts_stat->sendErr8 = rtscts_stat->sendErr9= 0;
        rtscts_stat->P3parsebad = 0;
		rtscts_stat->protoErr3 =
		    rtscts_stat->protoErr4 = rtscts_stat->protoErr5 =
		    rtscts_stat->protoErr6 = rtscts_stat->protoErr9 =
		    rtscts_stat->protoGCHdoneErr= 0;
		#ifdef DO_TIMEOUT_PROTOCOL
			rtscts_stat->protoErr15 = rtscts_stat->protoErr16 =
			rtscts_stat->protoErr17 = rtscts_stat->protoErr18 =
			rtscts_stat->protoErr19 = rtscts_stat->protoErr21= 0;
		#endif /* DO_TIMEOUT_PROTOCOL */
		/* rtscts_stat->outstanding_pkts DON'T DO THIS ONE! */
		rtscts_stat->badseq =
		rtscts_stat->protoWrongMsgNum= 0;
	    case SEG_EVENTS:
		/* We don't clear MCP events at this time */
		break;
        case SEG_PING:
            /* do nothing */
            return;
	    case SEG_SENTINEL:
		    return;
	}
    }

}  /* end of handleInfoReq() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handleInfoData(int src_nid, unsigned int msgID, char *data,
	unsigned int offset, int len)
{

int totlen= 0;


    #ifdef VERBOSE
	printk("handleInfoData() 0x%08x, len %d, offset %d, from %d\n",
	    msgID, len, offset, src_nid);
    #endif /* VERBOSE */

    if (msgID != get_info.msgID)   {
	/*
	printk("handleInfoData() msgID 0x%08x not found in get_req struct!\n",
	    msgID);
	*/
	return;
    }

    /* Catch the SEG_PING immediately and record its receipt since
     * there is no data in the packet.
    */
    if (get_info.seg == SEG_PING) {
        get_info.done= TRUE;
        return;
    }
    
    /* otherwise process the INFO_DATA packet normally */
    
    if (offset + len > MAX_TMP_DATA)   {
	printk("handleInfoData() More data than %d bytes\n", MAX_TMP_DATA);
	get_info.done= TRUE;
	return;
    }

    switch (get_info.seg)   {
	case SEG_ROUTE_USAGE:
	    totlen= sizeof(route_used);
	    break;
	case SEG_RTSCTS:
	    totlen= sizeof(rtscts_stat_t);
	    break;
	case SEG_MYRPKT:
	    totlen= sizeof(proc_stat_t);
	    break;
	case SEG_MCP:
	    totlen= sizeof(mcp_stat_t);
	    break;
	case SEG_ERROR:
	    totlen= sizeof(seg_error_t);
	    break;
	case SEG_EVENTS:
	    totlen= sizeof(eventlog_t);
	    break;
    case SEG_PING:
        totlen = 0;
        break;
	case SEG_SENTINEL:
	    printk("handleInfoData() Illegal segment received!\n");
	    return;
    }

    memcpy(get_info.data + offset, data, len);
    get_info.rcvd += len;
    if (get_info.rcvd >= totlen)   {
	get_info.done= TRUE;
    }

}  /* end of handleInfoData() */

/* >>>  ----------------------------------------------------------------- <<< */

int
get_info_data(void *buf)
{
    if (get_info.done  &&  get_info.seg == SEG_PING) {
        return 0;
    }
    else if (!get_info.done)   {
	    return -1;
    }

    copy_to_user(buf, get_info.data, get_info.rcvd);
    return get_info.rcvd;

}  /* end of get_info_data() */

/* >>>  ----------------------------------------------------------------- <<< */
