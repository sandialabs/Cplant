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
** $Id: RTSCTS_proc.c,v 1.46.2.3 2002/05/23 16:03:27 jbogden Exp $
** RTS/CTS module info in /proc/rtscts
*/

#include <sys/defines.h>        /* For MIN() */
#include <asm/uaccess.h>
#include "MCPshmem.h"           /* For MCP_version */
#include "RTSCTS_proc.h"
#include "RTSCTS_protocol.h"    /* For PKT_TIMEOUT, etc. */
#include "Pkt_module.h"
#include "hstshmem.h"           /* For hstshmem->pnid */
#include "queue.h"              /* For proc_queues() */
#include "RTSCTS_pkthdr.h"      /* For pkthdr_t */
#include "versions.h"


static rtscts_stat_t __rtscts_stat;
rtscts_stat_t *rtscts_stat= &__rtscts_stat;

#define MIN_VALUE		(999999999L)
char print_buf[MAX_PRINT_BUF];

/*

	**********************************************************************
	***   I'm going to violate the 80 character max line length rule   ***
	***   in this file. It is just too hard to read otherwise.         ***
	**********************************************************************

*/

#ifdef LINUX24
int
rtsctsProc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
rtsctsProc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	pb += sprintf(pb, "\nRTS/CTS Packet Module on Node %-4d           Protocol Ver. 0x%04x, MCP ver. %4d\n",
	    hstshmem->my_pnid, RTSCTS_PROTOCOL_version >> 16, MCP_version);

    #ifdef DO_TIMEOUT_PROTOCOL
    pb += sprintf(pb,"   DO_TIMEOUT_PROTOCOL         %4d",1);
    #else
    pb += sprintf(pb,"   DO_TIMEOUT_PROTOCOL         %4d",0);
    #endif /* DO_TIMEOUT_PROTOCOL */

    #ifdef REQUEST_RESENDS
    pb += sprintf(pb,"   REQUEST_RESENDS             %4d\n",1);
    #else
    pb += sprintf(pb,"   REQUEST_RESENDS             %4d\n",0);
    #endif /* REQUEST_RESENDS */
    
    #ifdef DROP_PKT_TEST
    pb += sprintf(pb,"   DROP_PKT_TEST               %4d",1);
    #else
    pb += sprintf(pb,"   DROP_PKT_TEST               %4d",0);
    #endif /* DROP_PKT_TEST */

    #ifdef PKTHDR_CLEAR
    pb += sprintf(pb,"   PKTHDR_CLEAR                %4d\n",1);
    #else
    pb += sprintf(pb,"   PKTHDR_CLEAR                %4d\n",0);
    #endif /* DROP_PKT_TEST */

    #ifdef NEW_P3_RESEND
    pb += sprintf(pb,"   NEW_P3_RESEND               %4d",1);
    #else
    pb += sprintf(pb,"   NEW_P3_RESEND               %4d",0);
    #endif

    #ifdef EXTENDED_P3_RTSCTS
    pb += sprintf(pb,"   EXTENDED_P3_RTSCTS          %4d\n",1);
    #else
    pb += sprintf(pb,"   EXTENDED_P3_RTSCTS          %4d\n",0);
    #endif
    
    pb += sprintf(pb,"   sizeof(pkthdr_t) in bytes   %4d",(int)sizeof(pkthdr_t));
    pb += sprintf(pb,"   sizeof(ptl_hdr_t) in bytes  %4d\n",(int)sizeof(ptl_hdr_t));                  
    pb += sprintf(pb,"   Packet timeout          %d/%d * HZ\n",
                  PKT_TIMEOUT*PKT_TIMEOUT_DENOMINATOR/HZ,PKT_TIMEOUT_DENOMINATOR);
    pb += sprintf(pb,"   P3 Message retries          %4d",P3_MSG_RETRIES);
    pb += sprintf(pb,"   P3 Message timeout        HZ* %d\n",P3_MSG_TIMEOUT/HZ);
 
	/* === P3 Messages === */
	pb += sprintf(pb, "\n------- P3 Message Level -------------------------------------------------------\n");
	pb += sprintf(pb, "   Sends/min/max/tot %15ld", rtscts_stat->P3send);
	if (rtscts_stat->P3sendminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->P3sendmaxlen, rtscts_stat->P3sendlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->P3sendminlen, rtscts_stat->P3sendmaxlen, rtscts_stat->P3sendlen);
	}

	pb += sprintf(pb, "   Recvs/min/max/tot %15ld", rtscts_stat->P3recv);
	if (rtscts_stat->P3recvminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->P3recvmaxlen, rtscts_stat->P3recvlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->P3recvminlen, rtscts_stat->P3recvmaxlen, rtscts_stat->P3recvlen);
	}
	pb += sprintf(pb, "   P3 events send/recv lib_parse %15ld %15ld %15ld\n", rtscts_stat->P3eventsent, rtscts_stat->P3eventrcvd, rtscts_stat->P3parse);



	/* === IP Messages === */
	pb += sprintf(pb, "\n------- IP Message (packet) Level ----------------------------------------------\n");
	pb += sprintf(pb, "   Sends/min/max/tot %15ld", rtscts_stat->IPsend);
	if (rtscts_stat->IPsendminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->IPsendmaxlen, rtscts_stat->IPsendlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->IPsendminlen, rtscts_stat->IPsendmaxlen, rtscts_stat->IPsendlen);
	}

	pb += sprintf(pb, "   Recvs/min/max/tot %15ld", rtscts_stat->IPrecv);
	if (rtscts_stat->IPrecvminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->IPrecvmaxlen, rtscts_stat->IPrecvlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->IPrecvminlen, rtscts_stat->IPrecvmaxlen, rtscts_stat->IPrecvlen);
	}


	/* === Packet Level === */
	pb += sprintf(pb, "\n------- Packet Level Stats -----------------------------------------------------\n");
	pb += sprintf(pb, "   Outstanding packets   %15ld", rtscts_stat->outstanding_pkts);
	#ifdef DROP_PKT_TEST
	    pb += sprintf(pb, "   Testing dropped pkts  %15ld\n", rtscts_stat->dbg_drop_pkt);
	#else
	    pb += sprintf(pb, "\n");
	#endif /* DROP_PKT_TEST */

	pb += sprintf(pb, "   Successful pkt sends  %15ld", rtscts_stat->PKTsend);
	pb += sprintf(pb, "   Unsuccessful pkt sends%15ld\n", rtscts_stat->PKTsendbad);

	pb += sprintf(pb, "   Sends/min/max/tot %15ld", rtscts_stat->PKTsend);
	if (rtscts_stat->PKTsendminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->PKTsendmaxlen, rtscts_stat->PKTsendlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->PKTsendminlen, rtscts_stat->PKTsendmaxlen, rtscts_stat->PKTsendlen);
	}

	pb += sprintf(pb, "   Recvs/min/max/tot %15ld", rtscts_stat->PKTrecv);
	if (rtscts_stat->PKTrecvminlen == MIN_VALUE)   {
	    pb += sprintf(pb, "         n/a %15ld %15ld\n", rtscts_stat->PKTrecvmaxlen, rtscts_stat->PKTrecvlen);
	} else   {
	    pb += sprintf(pb, " %11ld %15ld %15ld\n", rtscts_stat->PKTrecvminlen, rtscts_stat->PKTrecvmaxlen, rtscts_stat->PKTrecvlen);
	}


	/* === Error Correcting Protocol === */
	#ifdef DO_TIMEOUT_PROTOCOL
	pb += sprintf(pb, "\n------- Error Correcting Protocol: ---------------------------------------------\n");
	pb += sprintf(pb, "   P3 RTS timeout        %15ld\n", rtscts_stat->protoP3tout);
	pb += sprintf(pb, "   Didn't send data (?)  %15ld", rtscts_stat->protoCTSrcv_tout);
	pb += sprintf(pb, "   Didn't send DATA END ?%15ld\n", rtscts_stat->protoDATAsnd_tout);
	pb += sprintf(pb, "   CTS/MSGEND timeout    %15ld", rtscts_stat->protoDATAend_tout);
	pb += sprintf(pb, "   CTS resent            %15ld\n", rtscts_stat->protoCTSresend);
	pb += sprintf(pb, "   Give up on P3 resend  %15ld", rtscts_stat->protoP3giveUp);
	pb += sprintf(pb, "   P3 resend OK          %15ld\n", rtscts_stat->protoP3resend);
	pb += sprintf(pb, "   MSGEND resent         %15ld", rtscts_stat->protoMSGENDresend);
	pb += sprintf(pb, "   MSGDROP resent        %15ld\n", rtscts_stat->protoMSGDROPresend);
	pb += sprintf(pb, "   orphan P3_RESEND      %15ld", rtscts_stat->protoErr15);
	pb += sprintf(pb, "   orphan DATA_RESEND    %15ld\n", rtscts_stat->protoErr16);
	pb += sprintf(pb, "   Ill. LstEvnt Qrcving  %15ld", rtscts_stat->protoErr19);
	pb += sprintf(pb, "   P3 resend failed      %15ld\n", rtscts_stat->protoErr21);
	pb += sprintf(pb, "   Unxpctd evnt Qsnd_pnd %15ld", rtscts_stat->protoErr17);
	pb += sprintf(pb, "   Unxpctd evnt Qsending %15ld\n", rtscts_stat->protoErr18);

    #ifdef EXTENDED_P3_RTSCTS
	pb += sprintf(pb, "   P3 seq sync attempted %15ld", rtscts_stat->protoP3sync);
	pb += sprintf(pb, "   P3 seq sync accepted  %15ld\n", rtscts_stat->protoP3resynced);        
    #endif
	#endif /* DO_TIMEOUT_PROTOCOL */


	/* === Protocol Stats === */
	pb += sprintf(pb, "\n------- Packet Type Stats ------------------------------------------------------\n");
	pb += sprintf(pb, "   Type                 sent    received   Type                 sent    received\n");
	pb += sprintf(pb, "   P3_RTS        %11ld %11ld", rtscts_stat->protoSent[P3_RTS], rtscts_stat->protoRcvd[P3_RTS]);
	pb += sprintf(pb, "   P3_LAST_RTS   %11ld %11ld\n", rtscts_stat->protoSent[P3_LAST_RTS], rtscts_stat->protoRcvd[P3_LAST_RTS]);
	pb += sprintf(pb, "   DATA          %11ld %11ld", rtscts_stat->protoSent[DATA], rtscts_stat->protoRcvd[DATA]);
	pb += sprintf(pb, "   STOP_DATA     %11ld %11ld\n", rtscts_stat->protoSent[STOP_DATA], rtscts_stat->protoRcvd[STOP_DATA]);
	pb += sprintf(pb, "   LAST_DATA     %11ld %11ld", rtscts_stat->protoSent[LAST_DATA], rtscts_stat->protoRcvd[LAST_DATA]);
	pb += sprintf(pb, "   CTS           %11ld %11ld\n", rtscts_stat->protoSent[CTS], rtscts_stat->protoRcvd[CTS]);
	pb += sprintf(pb, "   MSGEND        %11ld %11ld", rtscts_stat->protoSent[MSGEND], rtscts_stat->protoRcvd[MSGEND]);
	pb += sprintf(pb, "   MSGDROP       %11ld %11ld\n", rtscts_stat->protoSent[MSGDROP], rtscts_stat->protoRcvd[MSGDROP]);
	pb += sprintf(pb, "   GCH           %11ld %11ld\n", rtscts_stat->protoSent[GCH], rtscts_stat->protoRcvd[GCH]);
	pb += sprintf(pb, "   IP            %11ld %11ld", rtscts_stat->protoSent[IP], rtscts_stat->protoRcvd[IP]);
	pb += sprintf(pb, "   PING          %11ld %11ld\n", rtscts_stat->protoSent[PING], rtscts_stat->protoRcvd[PING]);
	pb += sprintf(pb, "   PINGA         %11ld %11ld", rtscts_stat->protoSent[PINGA], rtscts_stat->protoRcvd[PINGA]);
	pb += sprintf(pb, "   PINGR         %11ld %11ld\n", rtscts_stat->protoSent[PINGR], rtscts_stat->protoRcvd[PINGR]);
	pb += sprintf(pb, "   ROUTE_STAT    %11ld %11ld", rtscts_stat->protoSent[ROUTE_STAT], rtscts_stat->protoRcvd[ROUTE_STAT]);
	pb += sprintf(pb, "   ROUTE_ACK     %11ld %11ld\n", rtscts_stat->protoSent[ROUTE_ACK], rtscts_stat->protoRcvd[ROUTE_ACK]);
	pb += sprintf(pb, "   ROUTE_REQ     %11ld %11ld", rtscts_stat->protoSent[ROUTE_REQ], rtscts_stat->protoRcvd[ROUTE_REQ]);
	pb += sprintf(pb, "   ROUTE_REQ_REPL%11ld %11ld\n", rtscts_stat->protoSent[ROUTE_REQ_REPLY], rtscts_stat->protoRcvd[ROUTE_REQ_REPLY]);
	pb += sprintf(pb, "   INFO_REQ      %11ld %11ld", rtscts_stat->protoSent[INFO_REQ], rtscts_stat->protoRcvd[INFO_REQ]);
	pb += sprintf(pb, "   INFO_DATA     %11ld %11ld\n", rtscts_stat->protoSent[INFO_DATA], rtscts_stat->protoRcvd[INFO_DATA]);
    
    #ifdef P3_PING
	pb += sprintf(pb, "   P3_PING_ACK   %11ld %11ld", rtscts_stat->protoSent[P3_PING_ACK], rtscts_stat->protoRcvd[P3_PING_ACK]);
	pb += sprintf(pb, "   P3_PING_REQ   %11ld %11ld\n", rtscts_stat->protoSent[P3_PING_REQ], rtscts_stat->protoRcvd[P3_PING_REQ]);
    #endif
    
    #ifdef EXTENDED_P3_RTSCTS
	pb += sprintf(pb, "   P3_NULL       %11ld %11ld", rtscts_stat->protoSent[P3_NULL], rtscts_stat->protoRcvd[P3_NULL]);
	pb += sprintf(pb, "   P3_SYNC       %11ld %11ld\n", rtscts_stat->protoSent[P3_SYNC], rtscts_stat->protoRcvd[P3_SYNC]);
    #endif /* EXTENDED_P3_RTSCTS */
    
	#if defined(DO_TIMEOUT_PROTOCOL) && defined(REQUEST_RESENDS)
    pb += sprintf(pb, "   P3_RESEND     %11ld %11ld\n", rtscts_stat->protoSent[P3_RESEND], rtscts_stat->protoRcvd[P3_RESEND]);
	#endif /* DO_TIMEOUT_PROTOCOL */


	pb += sprintf(pb, "\n------- Protocol Stats ---------------------------------------------------------\n");
	pb += sprintf(pb, "   GCH done successful   %15ld", rtscts_stat->protoGCHdone);
	pb += sprintf(pb, "   GCH done unsuccessful %15ld\n", rtscts_stat->protoGCHdoneErr);
	pb += sprintf(pb, "   Wrong msg num dropped %15ld", rtscts_stat->protoWrongMsgNum);
	pb += sprintf(pb, "   Pkts garbage collected%15ld\n", rtscts_stat->protoGC);
	pb += sprintf(pb, "   Truncated on send side%15ld", rtscts_stat->truncate);
	pb += sprintf(pb, "   truncated send bytes  %15ld\n", rtscts_stat->truncatelen);
	pb += sprintf(pb, "   Truncated on recv side%15ld", rtscts_stat->extradata);
	pb += sprintf(pb, "   truncated recv bytes  %15ld\n", rtscts_stat->extradatalen);
	pb += sprintf(pb, "   Pkts out of sequence  %15ld\n", rtscts_stat->badseq);


	/* === Module Internal & Environment === */
	pb += sprintf(pb, "\n------- Module Internal and Environment ----------------------------------------\n");
	pb += sprintf(pb, "   Send pages allocated  %15ld", rtscts_stat->sendpagealloc);
	pb += sprintf(pb, "   Bad memcpy (usr space)%15ld\n", rtscts_stat->badcpy);
	pb += sprintf(pb, "   Clrds old pending send%15ld\n", rtscts_stat->old_pending);


	/* === Send, Receive, & Protocol Errors === */
	pb += sprintf(pb, "\n------- Send, Receive, & Protocol Errors ---------------------------------------\n");
	pb += sprintf(pb, "   IP pkt len < 0        %15ld", rtscts_stat->sendErr6);
	pb += sprintf(pb, "   IP pkt len > payload  %15ld\n", rtscts_stat->sendErr7);
	pb += sprintf(pb, "   P3 pkt len < 0        %15ld", rtscts_stat->sendErr8);
	pb += sprintf(pb, "   Last pkt != LAST_DATA %15ld\n", rtscts_stat->protoErr9);
	pb += sprintf(pb, "   build_page(): No page %15ld", rtscts_stat->sendErr5);
	pb += sprintf(pb, "   build_page(len > MTU) %15ld\n", rtscts_stat->sendErr9);
	pb += sprintf(pb, "   orphan MSGEND         %15ld", rtscts_stat->protoErr3);
	pb += sprintf(pb, "   orphan MSGDROP        %15ld\n", rtscts_stat->protoErr4);
	pb += sprintf(pb, "   orphan CTS            %15ld", rtscts_stat->protoErr5);
	pb += sprintf(pb, "   orphan Data           %15ld\n", rtscts_stat->protoErr6);
    pb += sprintf(pb, "   GCH done              %15ld", rtscts_stat->protoGCHdoneErr);
    pb += sprintf(pb, "   p3_send() bad dst_nid %15ld\n", rtscts_stat->sendErr10);
    pb += sprintf(pb, "   lib_parse() failed    %15ld\n", rtscts_stat->P3parsebad);


	/* === Queues === */
	pb += sprintf(pb, "\n------- Queue Management -------------------------------------------------------\n");
	pb += sprintf(pb, "   entries alloc/freed/!enque    %15ld %15ld %15ld\n", rtscts_stat->queue_alloc, rtscts_stat->queue_freed, rtscts_stat->badenqueue);

	pb += sprintf(pb, "   Send pending Q: enqued/dequed                 %15ld %15ld\n", rtscts_stat->queue_add[Qsnd_pending], rtscts_stat->queue_rm[Qsnd_pending]);
	pb += sprintf(pb, "   Send pending Q: queue size current/max        %15ld %15ld\n", rtscts_stat->queue_size[Qsnd_pending], rtscts_stat->queue_max[Qsnd_pending]);
	pb += sprintf(pb, "   Sending Q: enqued/dequed                      %15ld %15ld\n", rtscts_stat->queue_add[Qsending], rtscts_stat->queue_rm[Qsending]);
	pb += sprintf(pb, "   Sending Q: queue size current/max             %15ld %15ld\n", rtscts_stat->queue_size[Qsending], rtscts_stat->queue_max[Qsending]);
	pb += sprintf(pb, "   Receiving Q: enqued/dequed                    %15ld %15ld\n", rtscts_stat->queue_add[Qreceiving], rtscts_stat->queue_rm[Qreceiving]);
	pb += sprintf(pb, "   Receiving Q: queue size current/max           %15ld %15ld\n", rtscts_stat->queue_size[Qreceiving], rtscts_stat->queue_max[Qreceiving]);
	pb += sprintf(pb, "\n");


	/*
	** Here come the queues
	*/
	proc_queues(&pb, print_buf + MAX_PRINT_BUF);
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(len, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

#ifdef LINUX24
    *eof = 1;
#endif
    return copy_len;

}  /* end of rtsctsProc() */

/* ************************************************************************** */

#ifdef LINUX24
int
remoteProc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
remoteProc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
int i;
unsigned long int ago;
static char *pb;
unsigned int talking;
int status;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	pb += sprintf(pb, "\nRemote Status Information for Node %-4d\n",
	    hstshmem->my_pnid);


	pb += sprintf(pb, "Node       Snd num       Rcv num  Status\n");
	for (i= 512; i < 664; i++)   {
	    pb += sprintf(pb, "%4d: %12d  %12d  ", i, send_msgNum[i],
		    lastMsgNum[i]);
	    talking= remote_status[i].status & RMT_TALKING;
	    status= remote_status[i].status & ~RMT_TALKING;
	    switch (status)   {
		case RMT_BOOT:
		    pb += sprintf(pb, "We booted "); break;
		case RMT_OK:
		    pb += sprintf(pb, "OK "); break;
		case RMT_PENDING:
		    pb += sprintf(pb, "Send pending "); break;
		case RMT_LOST:
		    pb += sprintf(pb, "Timeout "); break;
		default:
		    pb += sprintf(pb, "??? ");
	    }
	    if (talking)   {
		pb += sprintf(pb, "(rcv OK) ");
	    }
	    ago= (jiffies - remote_status[i].last_update) / HZ;
	    pb += sprintf(pb, "%lds ago\n", ago);
	}
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(len, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

    return copy_len;

}  /* end of remoteProc() */

/* ************************************************************************** */

void
rtsctsProcInit(void)
{

    memset(rtscts_stat, 0, sizeof(rtscts_stat_t));
    rtscts_stat->P3recvminlen= MIN_VALUE;
    rtscts_stat->P3sendminlen= MIN_VALUE;
    rtscts_stat->IPrecvminlen= MIN_VALUE;
    rtscts_stat->IPsendminlen= MIN_VALUE;
    rtscts_stat->PKTrecvminlen= MIN_VALUE;
    rtscts_stat->PKTsendminlen= MIN_VALUE;

}  /* end of rtsctsProcInit() */

/* ************************************************************************** */

#ifdef LINUX24
int
versions_proc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
versions_proc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
int i;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	pb += sprintf(pb, "Nbr File name            Ver     Date         "
	    "Time       User\n");
	pb += sprintf(pb, "----------------------------------------------"
	    "-----------------\n");

	i= 0;
	while (version_strings[i].file != NULL)   {
	    pb += sprintf(pb, "%-3d %-20s %-7s %-12s %-10s %-12s\n", i,
		    version_strings[i].file, version_strings[i].version,
		    version_strings[i].date, version_strings[i].time,
		    version_strings[i].user);
	    i++;
	}
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(len, (pb - print_buf) - off);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf + off, copy_len);
    } else   {
	copy_len= 0;
    }

    return copy_len;

}  /* end of versions_proc() */

/* ************************************************************************** */
