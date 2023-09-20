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
** $Id: queue.c,v 1.40.2.2 2002/05/22 21:40:03 jbogden Exp $
** manage the queues for the rts/cts module
*/

#include <linux/param.h>	/* For HZ */
#include <linux/sched.h>	/* For jiffies */
#include <sys/defines.h>	/* For FALSE */
#include "RTSCTS_send.h"	/* For ILLMSGNUM */
#include "RTSCTS_proc.h"
#include "queue.h"

static Qentry_t *Qhead[]= { [Qsnd_pending ... Qreceiving] = NULL };
static Qentry_t *Qtail[]= { [Qsnd_pending ... Qreceiving] = NULL };


/* ************************************************************************** */
/*
** Enqueue a message on the queue Q
*/
Qentry_t *
enqueue(Q_t Q, unsigned int msgID)
{

Qentry_t *entry;
unsigned long flags;


    save_flags(flags);
    cli();

    entry= (Qentry_t *)kmalloc(sizeof(Qentry_t), GFP_ATOMIC);
    if (entry == NULL)   {
	printk("enqueue() Out of memory!\n");
    #ifndef NO_ERROR_STATS
	rtscts_stat->badenqueue++;
    #endif /* NO_ERROR_STATS */
	restore_flags(flags);
	return NULL;
    }

    #ifndef NO_STATS
    rtscts_stat->queue_alloc++;
    #endif /* NO_STATS */
    
    entry->msgID= msgID;
    entry->msgNum= ILLMSGNUM;
    entry->next= NULL;
    entry->event= E_CREATE;
    entry->granted_pkts= 0;
    entry->lib_finalize_called= FALSE;
    entry->idle= jiffies;
    entry->retries= 0;
    entry->p3msg_retries= 0;
    entry->task = NULL;

    if (Qhead[Q] == NULL)   {
	Qhead[Q]= entry;
    }
    if (Qtail[Q] != NULL)   {
	Qtail[Q]->next= entry;
    }
    Qtail[Q]= entry;

    #ifndef NO_STATS
    rtscts_stat->queue_add[Q]++;
    rtscts_stat->queue_size[Q]++;
    if (rtscts_stat->queue_size[Q] > rtscts_stat->queue_max[Q])   {
	rtscts_stat->queue_max[Q]= rtscts_stat->queue_size[Q];
    }
    #endif /* NO_STATS */
    
    restore_flags(flags);
    return entry;

}  /* end of enqueue() */

/* ************************************************************************** */

void
free_entry(Qentry_t *entry)
{

unsigned long flags;


    save_flags(flags);
    cli();

    if (entry->next != NULL)   {
	printk("free_entry() still linked?\n");
    }
    
    #ifndef NO_STATS
    rtscts_stat->queue_freed++;
    #endif /* NO_STATS */
    
    if (entry->granted_pkts != 0)   {
	#ifdef KEEP_TRACK
	    printk("free_entry() Returning %ld granted packets to "
		"outstanding_pkts\n", entry->granted_pkts);
	#endif /* KEEP_TRACK */
	rtscts_stat->outstanding_pkts -= entry->granted_pkts;
    }
    memset(entry, 0, sizeof(Qentry_t));
    kfree(entry);
    restore_flags(flags);

}  /* end of free_entry() */

/* ************************************************************************** */

void
enqueue_entry(Q_t Q, Qentry_t *entry)
{

unsigned long flags;


    save_flags(flags);
    cli();

    if (Qhead[Q] == NULL)   {
	Qhead[Q]= entry;
    }
    if (Qtail[Q] != NULL)   {
	Qtail[Q]->next= entry;
    }
    Qtail[Q]= entry;
    entry->next= NULL;

    #ifndef NO_STATS
    rtscts_stat->queue_add[Q]++;
    rtscts_stat->queue_size[Q]++;
    if (rtscts_stat->queue_size[Q] > rtscts_stat->queue_max[Q])   {
	rtscts_stat->queue_max[Q]= rtscts_stat->queue_size[Q];
    }
    #endif /* NO_STATS */
    
    restore_flags(flags);

}  /* end of enqueue_entry() */

/* ************************************************************************** */
/*
** Find the entry with message ID msgID, remove it from the queue Q, and
** return a pointer to it.
*/
Qentry_t *
dequeue(Q_t Q, unsigned int msgID)
{

Qentry_t *entry;
Qentry_t *prev;
unsigned long flags;


    save_flags(flags);
    cli();
    entry= Qhead[Q];
    prev= NULL;
    while (entry)   {
	if (entry->msgID == msgID)   {
	    /* Found it! */
	    if (prev)   {
		prev->next= entry->next;
	    }
        #ifndef NO_STATS
	    rtscts_stat->queue_rm[Q]++;
	    rtscts_stat->queue_size[Q]--;
        #endif /* NO_STATS */
	    break;
	}

	prev= entry;
	entry= entry->next;
    }

    if (entry == Qtail[Q])   {
	Qtail[Q]= prev;
    }
    if (entry && (entry == Qhead[Q]))   {
	Qhead[Q]= entry->next;
    }
    if (entry != NULL)   {
	entry->next= NULL;
    }
    restore_flags(flags);
    return entry;

}  /* end of dequeue() */

/* ************************************************************************** */
/*
** Dequeue the first entry of queue Q
*/
Qentry_t *
dequeue_first(Q_t Q)
{

Qentry_t *entry;
unsigned long flags;


    save_flags(flags);
    cli();

    entry= Qhead[Q];
    if (entry)   {
	Qhead[Q]= entry->next;
	entry->next= NULL;
    #ifndef NO_STATS
	rtscts_stat->queue_rm[Q]++;
	rtscts_stat->queue_size[Q]--;
    #endif /* NO_STATS */
    }

    if (entry == Qtail[Q])   {
	Qtail[Q]= NULL;
    }

    restore_flags(flags);
    return entry;

}  /* end of dequeue_first() */

/* ************************************************************************** */
/*
** Find the entry with message ID msgID. DON'T remove it from the queue Q.
** Just return a pointer to it.
*/
Qentry_t *
find_queue(Q_t Q, unsigned int msgID)
{

Qentry_t *entry;
unsigned long flags;


    save_flags(flags);
    cli();

    entry= Qhead[Q];
    while (entry)   {
	if (entry->msgID == msgID)   {
	    /* Found it! */
	    break;
	}
	entry= entry->next;
    }

    restore_flags(flags);
    return entry;

}  /* end of find_queue() */

/* ************************************************************************** */
/*
** Find all entries in Q that match task and removed them
*/
void
clean_queue(Q_t Q, void *task)
{

Qentry_t *entry;
Qentry_t *next;
Qentry_t *prev;
unsigned long flags;
entry_event_t temp_event;


    save_flags(flags);
    cli();

    /* The processing on the Qsnd_pending is more complicated than the Qsending
     * and Qreceiving.  So we handle them in separate blocks to keep the code
     * more clean.
    */
    if (Q == Qsnd_pending) {

        entry= Qhead[Q];
        prev= NULL;
        while (entry)   {
	        next= entry->next;
	        if (entry->task == task)   {
                #ifdef EXTENDED_P3_RTSCTS
                /* Found one! In this case, we don't want to delete all the packets
                 * associated with the given task. If the queue entry is for a
                 * P3_RTS or P3_LAST_RTS packet, we want to change it to a P3_NULL
                 * packet and leave it in the queue. So, if and when we are
                 * able to successfully send the P3_NULL packets to the destination,
                 * the destination will know to basically ignore them with the
                 * key EXCEPTION that the receiver updates its Portals sequence
                 * number table (the msgNum array) for the given sender. The
                 * intended effect of this is to keep the sending and receiving
                 * node's Portals sequence numbers in sync. We use the P3_NULL
                 * approach because we can't reliably determine on the sender
                 * side (that's us!) which Portals sequence numbers to drop, so we
                 * just send them all.
                */
                temp_event = entry->event;
                if (temp_event == E_SENT_P3_RTS  ||  temp_event == E_SENT_P3_LAST_RTS ||
                    temp_event == E_SENT_P3_PING_REQ  ||  temp_event == E_SENT_P3_PING_ACK)
                {
                    entry->event = E_SENT_P3_NULL;

                    /* Since we are cleaning up, we know that the portals process
                     * the packet is associated with is going away. We need to
                     * update the packet's queue entry info to reflect this so that
                     * we don't try to do something with the defunct portals
                     * process later and cause nastiness in the kernel.
                    */
                    entry->nal = NULL;
                    entry->task = NULL;
                }

                #else
	            /* Found one! In this case, we delete all packets associated
                 * the given task.
                */
	            if (prev)   {
                    prev->next= entry->next;
	            }

                #ifndef NO_STATS
	            rtscts_stat->queue_rm[Q]++;
	            rtscts_stat->queue_size[Q]--;
                #endif /* NO_STATS */

	            if (entry == Qtail[Q])   {
                    Qtail[Q]= prev;
	            }

	            if (entry == Qhead[Q])   {
                    Qhead[Q]= entry->next;
	            }

	            entry->next= NULL;
	            free_entry(entry);
                #endif /* EXTENDED_P3_RTSCTS */
	        } else   {
	            prev= entry;
	        }
	        entry= next;
        }

    }
    else {
        /* we are cleaning the Qsending or Qreceiving */
        entry= Qhead[Q];
        prev= NULL;
        while (entry)   {
	        next= entry->next;
	        if (entry->task == task)   {
	            /* Found one! In this case, we delete all packets associated
                 * the given task.
                */
	            if (prev)   {
		            prev->next= entry->next;
	            }
                
                #ifndef NO_STATS
	            rtscts_stat->queue_rm[Q]++;
	            rtscts_stat->queue_size[Q]--;
                #endif /* NO_STATS */
                
	            if (entry == Qtail[Q])   {
		            Qtail[Q]= prev;
	            }
                
	            if (entry == Qhead[Q])   {
		            Qhead[Q]= entry->next;
	            }
                
	            entry->next= NULL;
	            free_entry(entry);
	        } else   {
	            prev= entry;
	        }
	        entry= next;
        }        
    }
        
    restore_flags(flags);

}  /* end of clean_queue() */

/* ************************************************************************** */

#ifdef DO_TIMEOUT_PROTOCOL

/*
** Find packets whose idle time is larger than "idle" (in jiffies) starting
** with entry "pos" in the queue.
*/
Qentry_t *
check_idle(Q_t Q, int *pos, unsigned long idle)
{

Qentry_t *entry;
int i;
unsigned long flags;


    save_flags(flags);
    cli();

    entry= Qhead[Q];
    i= *pos;
    /* step to position pos */
    while (entry && i)   {
	i--;
	entry= entry->next;
    }

    /* check the idle time */
    while (entry)   {
	if ((jiffies - entry->idle) > idle)   {
	    break;
	}
	entry= entry->next;
	(*pos)++;
    }

    restore_flags(flags);
    return entry;

}  /* end of check_idle() */

/* ************************************************************************** */
/*
** For a given destination, find the entry with the lowest message Number.
** Return it, if it has been idle for at least "idle" time.
*/
Qentry_t *
find_oldest_msgNum(Q_t Q, int dest, unsigned long idle)
{

Qentry_t *entry;
Qentry_t *old_entry;
unsigned long flags;
unsigned int old_msgNum;


    save_flags(flags);
    cli();

    entry= Qhead[Q];
    old_msgNum= ~0;
    old_entry= NULL;

    while (entry)   {
	if (entry->dst_nid == dest)   {
	    /* Potential candidate */
	    if (entry->msgNum < old_msgNum)   {
		/*
		** If the entry->msNum is much older than one of the other
		** entry->msgNum, then it is probably one that has warpped
		** around and should be ignored.
		*/
		if ((old_msgNum == ~0) ||
			((old_msgNum - entry->msgNum) < 50000))   {
		    old_msgNum= entry->msgNum;
		    old_entry= entry;
		}
	    } else if ((old_msgNum != ~0) &&
		    ((entry->msgNum - old_msgNum) > 50000))   {
		old_msgNum= entry->msgNum;
		old_entry= entry;
	    }
	}
	entry= entry->next;
    }

    if (old_entry && ((jiffies - old_entry->idle) > idle))   {
	entry= old_entry;
    }

    restore_flags(flags);
    return entry;

}  /* end of find_oldest_msgNum() */

#endif /* DO_TIMEOUT_PROTOCOL */

/* ************************************************************************** */

char *
event_str(entry_event_t event)
{
    switch (event)   {
	case E_CREATE:
	    return "New entry";
	case E_SENT_LAST_DATA:
	    return "Sent LAST_DATA";
	case E_SENT_STOP_DATA:
	    return "Sent STOP_DATA";
	case E_SENT_DATA:
	    return "Sent DATA";
	case E_SENT_MSGEND:
	    return "Sent MSGEND";
	case E_SENT_MSGDROP:
	    return "Sent MSGDROP";
	case E_SENT_CTS:
	    return "Sent CTS";
	case E_RCVD_CTS:
	    return "Rcvd CTS";
	case E_RCVD_DATA:
	    return "Rcvd DATA";
	case E_RCVD_STOP_DATA:
	    return "Rcvd STOP_DATA";
	case E_RCVD_LAST_DATA:
	    return "Rcvd LAST_DATA";
	case E_SENT_P3_RTS:
	    return "Sent P3_RTS";
	case E_SENT_P3_LAST_RTS:
	    return "Sent P3_LAST_RTS";
	case E_RCVD_P3_RTS:
	    return "Rcvd P3_RTS";
	case E_RCVD_P3_LAST_RTS:
	    return "Rcvd P3_LAST_RTS";
	case E_INVALID:
	    return "Invalid event";
    case E_SENT_P3_PING_REQ:
        return "Sent P3_PING_REQ";
    case E_RCVD_P3_PING_REQ:
        return "Rcvd P3_PING_REQ";
    case E_SENT_P3_PING_ACK:
        return "Sent P3_PING_ACK";
    case E_RCVD_P3_PING_ACK:
        return "Rcvd P3_PING_ACK";
    #ifdef EXTENDED_P3_RTSCTS
    case E_SENT_P3_NULL:
        return "Sent P3_NULL";
    case E_SENT_P3_SYNC:
        return "Sent P3_SYNC";
    case E_RCVD_P3_NULL:
        return "Rcvd P3_NULL";
    case E_RCVD_P3_SYNC:
        return "Rcvd P3_SYNC";
    #endif
    }

    return "???";

}  /* end of event_str() */

/* ************************************************************************** */

void
disp_queues(void)
{

Qentry_t *entry;
unsigned long int ago;


    printk("Send pending queue:\n");
    entry= Qhead[Qsnd_pending];
    while (entry)   {
	ago= (jiffies - entry->idle) / HZ;
	printk("   msgID 0x%08x, len %d, buf %p\n",
		entry->msgID, entry->len, entry->buf);
	printk("       data sent %d, dnid %d, last event: "
		"%s %lds ago\n", entry->datasent, entry->dst_nid,
		event_str(entry->event), ago);
	entry= entry->next;
    }
    printk("\nSending queue:\n");
    entry= Qhead[Qsending];
    while (entry)   {
	ago= (jiffies - entry->idle) / HZ;
	printk("   msgID 0x%08x, len %d, buf %p\n",
		entry->msgID, entry->len, entry->buf);
	printk("       data sent %d, dnid %d, last event: "
		"%s %lds ago\n", entry->datasent, entry->dst_nid,
		event_str(entry->event), ago);
	entry= entry->next;
    }
    printk("\nReceiving queue:\n");
    entry= Qhead[Qreceiving];
    while (entry)   {
	ago= (jiffies - entry->idle) / HZ;
	printk("   msgID 0x%08x, len %d, buf %p\n",
		entry->msgID, entry->len, entry->buf);
	printk("       data rcvd %d, seq %d, snid %d, last "
		"event: %s %lds ago\n", entry->datasent, entry->seq,
		entry->src_nid, event_str(entry->event), ago);
	entry= entry->next;
    }

}  /* end of disp_queues() */

/* ************************************************************************** */
/*
** Add the queues contents to the print buffer for /proc
*/
void
proc_queues(char **pb_ptr, char *pb_end)
{

char *pb;
Qentry_t *entry;
unsigned long int ago;


    pb= *pb_ptr;

    pb += sprintf(pb, "Send pending queue:\n");
    entry= Qhead[Qsnd_pending];
    while (entry && ((pb + 512) < pb_end))   {
	ago= (jiffies - entry->idle) / HZ;
	pb += sprintf(pb, "   msgID 0x%08x, len %d, buf %p, msgNum %d, retries %d\n",
        entry->msgID,entry->len, entry->buf, entry->msgNum, entry->p3msg_retries);
	pb += sprintf(pb, "       data sent %d, seq %d, dnid %d, last event: %s %lds "
		"ago\n", entry->datasent, entry->seq, entry->dst_nid,
		event_str(entry->event), ago);
	entry= entry->next;
    }
    pb += sprintf(pb, "\nSending queue:\n");
    entry= Qhead[Qsending];
    while (entry && ((pb + 512) < pb_end))   {
	ago= (jiffies - entry->idle) / HZ;
	pb += sprintf(pb, "   msgID 0x%08x, len %d, buf %p, msgNum %d, retries %d\n",
		entry->msgID, entry->len, entry->buf, entry->msgNum, entry->retries);
	pb += sprintf(pb, "       data sent %d, seq %d, dnid %d, last event: "
		"%s %lds ago\n", entry->datasent, entry->seq, entry->dst_nid,
		event_str(entry->event), ago);
	entry= entry->next;
    }
    pb += sprintf(pb, "\nReceiving queue:\n");
    entry= Qhead[Qreceiving];
    while (entry && ((pb + 512) < pb_end))   {
	ago= (jiffies - entry->idle) / HZ;
	pb += sprintf(pb, "   msgID 0x%08x, len %d, buf %p, msgNum %d, retries %d\n",
		entry->msgID, entry->len, entry->buf, entry->msgNum, entry->retries);
	pb += sprintf(pb, "       data rcvd %d, seq %d, snid %d, last "
		"event: %s %lds ago\n", entry->datasent, entry->seq,
		entry->src_nid, event_str(entry->event), ago);
	entry= entry->next;
    }

    if ((pb + 512) >= pb_end)   {
	pb += sprintf(pb, "\n... more ...\n");
    }
    *pb_ptr= pb;

}  /* end of proc_queues() */

/* ************************************************************************** */
/*
** Remove all entries from queue Q
*/
void
reset_queue(Q_t Q)
{

Qentry_t *entry;
unsigned long flags;


    save_flags(flags);
    cli();

    printk("RTSCTS: Dumping queue %d\n", Q);

    while ((entry = dequeue_first(Q)) != NULL)   {
	printk("Queue entry %p:\n", entry);
	printk("  buf=%p msgid=%d\n", entry->buf, entry->msgID);
	printk("  len=%d sent=%d seq=%d\n",
		entry->len, entry->datasent, entry->seq);
	printk("  nal=%p\n", entry->nal);

	free_entry(entry);
    }
    restore_flags(flags);

}  /* end of reset_queue() */

/* ************************************************************************** */

void
reset_all_queues(void)
{

Q_t q;


    for (q= Qsnd_pending; q <= Qreceiving; q++)   {
	reset_queue(q);
    }

}  /* end of reset_all_queues() */

/* ************************************************************************** */
void
disp_Qentry(Qentry_t *entry)
{
    printk("Entry %p\n", (void *)entry);
    printk("    pktlen          %d\n", entry->pktlen);
    printk("    buf             %p\n", entry->buf);
    printk("    msgID           %s\n", show_msgID(entry->msgID));
    printk("    msgNum          %d\n", entry->msgNum);
    printk("    len             %d\n", entry->len);
    printk("    datasent        %d\n", entry->datasent);
    printk("    seq             %d\n", entry->seq);
    printk("    src_nid         %d\n", entry->src_nid);
    printk("    crc             %d\n", entry->crc);
    printk("    event           %s\n", event_str(entry->event));
    printk("    idle            %lds\n", (jiffies - entry->idle) / HZ);
    printk("    nal             %p\n", entry->nal);
    printk("    cookie          %p\n", entry->cookie);
    printk("    granted_pkts    %ld\n", entry->granted_pkts);
    printk("    retries         %d\n", entry->retries);
    printk("    p3msg_retries   %d\n", entry->p3msg_retries);
}  /* end of disp_Qentry() */

/* ************************************************************************** */
