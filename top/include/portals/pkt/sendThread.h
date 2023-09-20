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
** $Id: sendThread.h,v 1.16 2001/09/21 17:02:55 pumatst Exp $
*/


#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <linux/netdevice.h>
#include <linux/sched.h>
#include <portals/base/ptlDevice.h>
#include <portals/comm_buff.h>
#include <portals/pkt/ptlPkt.h>
#include <portals/base/queue.h>
#include <portals/base/sleep.h>
#include <portals/base/debug.h>
#include <portals/base/updateUserInfo.h>
#include <portals/base/ptlStateInfo.h>
#include <portals/base/sendEntry.h>

#define MAX_PENDING 5 
#define SEC 0

#define NSEC 400000

enum { SENDING, SENT, FAILED };


typedef struct {
    sendEntry_t	info;
    void                *pos;
    int			numPkts;
    int			numPktsSent;
    int			numPktsAcked;
    int 		nbytesLeft;  
    int			error;	
} pktSendEntry_t;

typedef struct {
    void 		*pkt; 
    pktSendEntry_t	*sendEntry;
    unsigned long	timeout;
    int			seqNum;	
} sentPktQent_t;

extern queue_t *GL_sentQ;
extern queue_t *GL_sendQ;
extern void sendThread( void );
extern void sentThread( void );
extern int initSendThread(  void );
extern void sendThreadCleanup(void);
extern void decPending(void);
extern void incPending(void);
extern int canSend(void);

static inline void freeSentEntry( queue_t *entry )
{
    sentPktQent_t  *pktEnt; 

    if ( entry ) {
	pktEnt = (sentPktQent_t *) entry->data;

	if ( slist[pktEnt->sendEntry->info.hdr.dst_nid].pending > 0 ) {
            --slist[pktEnt->sendEntry->info.hdr.dst_nid].pending;
	} else {
	    printk("portals: sent list pending count corrupt\n");	
	}

	QremoveEntry( entry ); 
	decPending(); 

	if ( pktEnt->sendEntry ) {
	    ++pktEnt->sendEntry->numPktsAcked;
	} else {
	    printk("freeSentEntry(), no send entry for pkt\n" );
        } 
        __kfree_skb(pktEnt->pkt);
        kfree( pktEnt );
         
        QfreeEntry( entry );
    } else {
	printk("freeSentEntry(), passed NULL pointer\n" );
    }
}

static inline void freeAllSentEntries( pktSendEntry_t *entry )
{
    queue_t 		*next = GL_sentQ;

    while ( ( next = QgetNext( next ) ) != GL_sentQ->prev ) {

        sentPktQent_t  *pktEnt;
	if ( ( pktEnt = (sentPktQent_t *) next->data ) ) {

	    if ( pktEnt->sendEntry == entry ) {
	        if ( pktEnt->sendEntry->info.hdr.src_pid == 
					entry->info.hdr.src_pid ) {
	            freeSentEntry( next );
		    printk("freeAllSentEntries() freeing entry for %i\n",
					entry->info.hdr.src_pid );
	        } else {
		    printk("not my pid %i\n",
					pktEnt->sendEntry->info.hdr.src_pid);
		}   
	    }
	} else {
	    printk("freeAllSentEntries(), error NULL data pointer\n");	
	}
    }
}


static inline void removeSendEntry( int pid, int exiting )
{
    queue_t 		*next = GL_sendQ;
    pktSendEntry_t	*sendEntry = NULL;

    PRINTK("removeSendEntry() for pid %i exiting %i\n", pid, exiting );;

    while ( ( next = QgetNext( next ) ) != GL_sendQ->prev ) {

	if ( ((pktSendEntry_t *) next->data)->info.hdr.src_pid == pid ) { 

	    PRINTK("removeSendEntry() found entry for %i\n", pid );	
            QremoveEntry( next );
	    sendEntry = (pktSendEntry_t *) next->data; 
            QfreeEntry( next );
	    break;	
	}
    }

    freeAllSentEntries( sendEntry );

    if ( sendEntry  ) { 
	    PRINTK("total %i send %i ack %i\n",sendEntry->numPkts,
						sendEntry->numPktsSent,
						sendEntry->numPktsAcked );
	if ( slist[sendEntry->info.hdr.dst_nid].entry == sendEntry ) {
	    slist[sendEntry->info.hdr.dst_nid].entry = NULL;
	}
    }
    if ( sendEntry  && ! exiting  ) {
	updateUserInfo(sendEntry->info.hdr.src_pid, sendEntry->info.send_flag);
        kfree( sendEntry );
    }
}   

static inline void QsentPkt( pktSendEntry_t *sendEntry, void *buff, int seqNum ) 
{
    queue_t *entry;
    if ((entry= QgetEntry(NULL)) == NULL)   {
	printk("QsentPkt() Out of memory\n");
	return;
    }

    entry->data = kmalloc( sizeof(sentPktQent_t), GFP_ATOMIC);
    if (entry->data == NULL)   {
	printk("QsentPkt() Out of memory\n");
	return;
    }
    PRINTK("QsendPkt buff %p sendEntry %p %i\n",buff, sendEntry,seqNum);
    ((sentPktQent_t *)entry->data)->pkt = buff;
    ((sentPktQent_t *)entry->data)->sendEntry = sendEntry;
    ((sentPktQent_t *)entry->data)->timeout = getTimeout(SEC,NSEC);
    ((sentPktQent_t *)entry->data)->seqNum = seqNum;
    
    QaddTail( GL_sentQ, entry );
}

#endif
