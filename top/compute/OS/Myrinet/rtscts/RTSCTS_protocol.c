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
** $Id: RTSCTS_protocol.c,v 1.30.2.3 2002/05/22 21:40:03 jbogden Exp $
** Protocol related functions
*/

#include <linux/kernel.h>	/* For sprintf() */
#include <linux/sched.h>	/* For NULL */
#include <linux/tqueue.h>	/* For tq_struct */
#include "RTSCTS_protocol.h"	/* For msgID_* */
#include "RTSCTS_proc.h"	/* For rtscts_stat */
#include "RTSCTS_recv.h"	/* For handleDataResend() */
#include "RTSCTS_p3.h"		/* For handleP3resend() */


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Constants
*/

const char *protocol_name_str[] =   {
    [CTS] =             "CTS        ",
    [DATA] =			"DATA       ",
    [STOP_DATA] =		"STOP DATA  ",
    [LAST_DATA] =		"LAST_DATA  ",
    [MSGEND] =			"MSGEND     ",
    [MSGDROP] =			"MSGDROP    ",
    [PING] =			"PING       ",
    [PINGA] =			"PINGA      ",
    [PINGR] =			"PINGR      ",
    [IP] =              "IP         ",
    [ROUTE_STAT] =		"RT STAT    ",
    [ROUTE_ACK] =		"RT ACK     ",
    [P3_RTS] =			"P3 RTS     ",
    [P3_LAST_RTS] =		"P3 LST RTS ",
    [ROUTE_REQ] =		"RT REQ     ",
    [ROUTE_REQ_REPLY] = "RT REQ RPLY",
#ifdef DO_TIMEOUT_PROTOCOL
    [P3_RESEND] =       "P3 RESEND  ",
#else
    [PAD2] =            "PAD2       ",
#endif /* DO_TIMEOUT_PROTOCOL */
    [GCH] =             "GCH        ",
    [INFO_REQ] =		"INFO REQ   ",
    [INFO_DATA] =		"INFO DATA  ",
    [CACHE_REQ] =       "CACHE REQ  ",
    [CACHE_DATA] =      "CACHE DATA ",
    [P3_PING_REQ] =     "P3 PING REQ",
    [P3_PING_ACK] =     "P3 PING ACK",
#ifdef EXTENDED_P3_RTSCTS
    [P3_NULL] =         "P3 NULL    ",
    [P3_SYNC] =         "P3 SYNC    ",
#endif /* EXTENDED_P3_RTSCTS */
    [LAST_ENTRY_DO_NOT_MOVE] =	"INVAL TYPE ",
};


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Keep track of the status of other nodes
*/
remote_status_t remote_status[MAX_NUM_ROUTES];
unsigned int lastMsgNum[MAX_NUM_ROUTES];
unsigned int send_msgNum[MAX_NUM_ROUTES];



/* >>>  ----------------------------------------------------------------- <<< */
/*
** Local globals
*/
#ifdef DO_TIMEOUT_PROTOCOL
    static struct tq_struct timeout_task;
    volatile static int reschedule= 1;
    volatile static int ready_for_exit= 0;
#endif /* DO_TIMEOUT_PROTOCOL */


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Local functions
*/
#ifdef DO_TIMEOUT_PROTOCOL
    static void timeout_send_queues(void);
    static void clean_recv_queues(void);
#endif /* DO_TIMEOUT_PROTOCOL */


/* >>>  ----------------------------------------------------------------- <<< */

int
msgID2pnid(unsigned int msgID)
{
    return (msgID >> msgID_SHIFT) - 1;
}

int 
msgID2msgNUM(unsigned int msgID)
{
    return (msgID & msgID_MASK);
}

/* >>>  ----------------------------------------------------------------- <<< */

char *
show_msgID(unsigned int msgID)
{

static char str[128];

    sprintf(str, "msgID 0x%08x (pnid %d, msg num %d)", msgID,
	msgID2pnid(msgID), msgID2msgNUM(msgID));
    return str;

}  /* end of show_msgID() */

/* >>>  ----------------------------------------------------------------- <<< */

void
init_protocol(void)
{
int i;


    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	lastMsgNum[i]= BOOTMSGNUM;
	send_msgNum[i]= BOOTMSGNUM;
	remote_status[i].status= RMT_BOOT;
	remote_status[i].first_ack= FALSE;
	remote_status[i].last_update= jiffies;
    }
}  /* end of init_protocol() */

/* >>>  ----------------------------------------------------------------- <<< */

#ifdef DO_TIMEOUT_PROTOCOL

static void
handle_timer(void *ptr)
{

static int threshold= 0;
static int clean= 0;


    /* Do this every 1/32 of a second */
    clean++;
    if (++threshold > (HZ / 32))   {
	/* Go through the queues and check for timeouts */
	threshold= 0;
	timeout_send_queues();
	if (clean > (1 * 60 * HZ))   {
	    /* Once every minute should be OK */
	    clean_recv_queues();
	    clean= 0;
	}
    }

    /* Run again in 1/HZ seconds */
    schedule_rtscts_timeout();

}  /* end of handle_timer() */

/* >>>  ----------------------------------------------------------------- <<< */

void
schedule_rtscts_timeout(void)
{

    timeout_task.routine= handle_timer;
    timeout_task.data= NULL;

    /* Put our little routine onto the timer queue */
    if (reschedule)   {
	queue_task(&timeout_task, &tq_timer);
    } else   {
	ready_for_exit= 1;
    }

}  /* end of schedule_rtscts_timeout() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Wait for the next timer interrupt. Otherwise it is not save to remove this
** module.
*/
void
remove_rtscts_timeout(void)
{

    reschedule= 0;
    while (!ready_for_exit)
	;

}  /* end of remove_rtscts_timeout() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** March through the queues and see if anything has timed out
*/
static void
timeout_send_queues(void)
{

int pos;
int i;
Qentry_t *entry;
unsigned long flags;
unsigned int talking;


    save_flags(flags);
    cli();

    /*
    ** Check the Qsnd_pending queue. The only thing on here should
    ** be the several types of P3 packets which are the beginning of
    ** P3 messages or P3 util packets.
    ** We only want to resend one packet per destination.
    **
    ** I know this is inefficient. I'm in a hurry. Feel free to do better!
    */
    #ifdef EXTENDED_P3_RTSCTS
    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	    entry= find_oldest_msgNum(Qsnd_pending, i, PKT_TIMEOUT);
	    if (entry)   {
            if ( (entry->event == E_SENT_P3_RTS) ||
		         (entry->event == E_SENT_P3_LAST_RTS) ||
                 (entry->event == E_SENT_P3_PING_REQ) ||
                 (entry->event == E_SENT_P3_PING_ACK) ||
                 (entry->event == E_SENT_P3_NULL) )   {

		        #ifdef VERBOSE
                printk("P3 RTS timeout\n");
		        #endif /* VERBOSE */

                #ifndef NO_STATS
		        rtscts_stat->protoP3tout++;
                talking= remote_status[entry->dst_nid].status & RMT_TALKING;
		        remote_status[entry->dst_nid].status= RMT_LOST | talking;
		        remote_status[entry->dst_nid].last_update= jiffies;
                #endif /* NO_STATS */

                /* Has the message timed out as well as the packet?
                 * If so, it's possible that that the destination node
                 * is dropping our P3 packets due to message sequence
                 * numbers getting out of sync.  Let's try to get the
                 * receiver to sync on the Portals message sequence
                 * number we have been retrying.
                */
                if ((entry->p3msg_retries) && 
                    (entry->p3msg_retries % P3_MSG_RETRIES) == 0) {
                    handleP3sync(entry->msgID, entry->msgNum);
                }
                else {                
                    handleP3resend(entry->msgID, entry->msgNum);
                }
	        }
            else   {
		        printk("timeout_send_queues() Unexpected event %s in Qsnd_pending.\n",
                       event_str(entry->event));
                #ifndef NO_ERROR_STATS
		        rtscts_stat->protoErr17++;
                #endif /* NO_ERROR_STATS */
	        }
	    }
    }    
    #else
    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	    entry= find_oldest_msgNum(Qsnd_pending, i, PKT_TIMEOUT);
	    if (entry)   {
            if ( (entry->event == E_SENT_P3_RTS) ||
		         (entry->event == E_SENT_P3_LAST_RTS) ||
                 (entry->event == E_SENT_P3_PING_REQ) ||
                 (entry->event == E_SENT_P3_PING_ACK) )  {

		        #ifdef VERBOSE
                printk("P3 RTS timeout\n");
		        #endif /* VERBOSE */

                #ifndef NO_STATS
		        rtscts_stat->protoP3tout++;
                talking= remote_status[entry->dst_nid].status & RMT_TALKING;
		        remote_status[entry->dst_nid].status= RMT_LOST | talking;
		        remote_status[entry->dst_nid].last_update= jiffies;
                #endif /* NO_STATS */

                handleP3resend(entry->msgID, entry->msgNum);
	        }
            else   {
		        printk("timeout_send_queues() Unexpected event %s in Qsnd_pending.\n",
                       event_str(entry->event));
                #ifndef NO_ERROR_STATS
		        rtscts_stat->protoErr17++;
                #endif /* NO_ERROR_STATS */
	        }
	    }
    }
    #endif

    /*
    ** Check the Qsending queue. The RTS has been acknowledged with a CTS
    ** and data has started to move out. If we see a CTS on this queue, it
    ** means we haven't started sending data yet. This should not really
    ** happen, since we send data as soon as we get the CTS. If we're in
    ** the middle of sending data we might see a DATA event. But again,
    ** it should not have timed out yet.  So, we expect to find STOP_DATA
    ** and LAST_DATA entries only.
    */
    pos= 0;
    while ((entry= check_idle(Qsending, &pos, PKT_TIMEOUT)))   {

	/* OK, got one. Lets see what it is */
	if (entry->event == E_RCVD_CTS)   {
	    #ifdef VERBOSE
		printk("Got CTS but didn't send data. Should not happen!\n");
	    #endif /* VERBOSE */
        #ifndef NO_STATS
        rtscts_stat->protoCTSrcv_tout++;
        talking= remote_status[entry->dst_nid].status & RMT_TALKING;
	    remote_status[entry->dst_nid].status= RMT_LOST | talking;
	    remote_status[entry->dst_nid].last_update= jiffies;
        #endif /* NO_STATS */
	    handleDataResend(entry->msgID); /* Try anyway */
	} else if (entry->event == E_SENT_DATA)   {
	    #ifdef VERBOSE
		printk("Sent data, but not STOP or LAST. Should not happen!\n");
	    #endif /* VERBOSE */
        #ifndef NO_STATS
        rtscts_stat->protoDATAsnd_tout++;
        talking= remote_status[entry->dst_nid].status & RMT_TALKING;
	    remote_status[entry->dst_nid].status= RMT_LOST | talking;
	    remote_status[entry->dst_nid].last_update= jiffies;
        #endif /* NO_ERROR_STATS */
	    handleDataResend(entry->msgID); /* Try anyway */
	} else if ((entry->event == E_SENT_STOP_DATA) ||
		(entry->event == E_SENT_LAST_DATA))   {
	    #ifdef VERBOSE
		printk("timeout waiting for CTS or MSGEND\n");
	    #endif VERBOSE
        #ifndef NO_STATS
        rtscts_stat->protoDATAend_tout++;
        talking= remote_status[entry->dst_nid].status & RMT_TALKING;
	    remote_status[entry->dst_nid].status= RMT_LOST | talking;
	    remote_status[entry->dst_nid].last_update= jiffies;
        #endif /* NO_ERROR_STATS */
	    /*
	    ** We'll resend the first data packet. This will cause
	    ** a sequence error on the receive side and cause a
	    ** reset of the sequence.
	    */
	    handleDataResend(entry->msgID);
	} else   {
	    printk("timeout_send_queues() Unexpected event %d in Qsending\n",
		entry->event);
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr18++;
        #endif /* NO_ERROR_STATS */
	}
	pos++;
    }
    restore_flags(flags);

}  /* end of timeout_send_queues() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Go through the receiving queue and clean out entries that have been
** in there for more than ten minutes. This should only happen if a
** GCH is lost or the sender is killed.
*/
static void
clean_recv_queues(void)
{

int pos;
Qentry_t *entry;
unsigned long flags;

    save_flags(flags);
    cli();

    pos= 0;
    while ((entry= check_idle(Qreceiving, &pos, GCH_TIMEOUT)))   {
	if ((entry->event == E_SENT_MSGEND) ||
		(entry->event == E_SENT_MSGDROP))   {
	    entry= dequeue(Qreceiving, entry->msgID);
	    #ifdef VERBOSE
		printk("clean_recv_queues() removing an entry msgID 0x%08x, "
		    "%lds old, last event %s\n", entry->msgID,
		    (jiffies - entry->idle) / HZ, event_str(entry->event));
	    #endif VERBOSE
	    free_entry(entry);
        #ifndef NO_STATS
	    rtscts_stat->protoGC++;
        #endif /* NO_STATS */
	}
	pos++;
    }
    restore_flags(flags);

}  /* end of clean_recv_queues() */

/* >>>  ----------------------------------------------------------------- <<< */

#endif /* DO_TIMEOUT_PROTOCOL */
