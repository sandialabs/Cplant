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
** $Id: Pkt_send.c,v 1.39 2002/02/26 23:15:52 jbogden Exp $
** Send a packet over Myrinet
*/

#include <linux/mm.h>		/* Import GFP_ATOMIC, get_free_page() */
#include <linux/kernel.h>	/* Import printk() */
#include <linux/netdevice.h>    /* struct device */
#include <asm/io.h>		/* Import virt_to_bus() */
#include <asm/byteorder.h>	/* Import htonl(), ntohl() */

#include <sys/defines.h>
#include <load/sys_limits.h>	/* For MAX_NODES */

#include "MCPshmem.h"
#include "hstshmem.h"
#include "arch_asm.h"		/* Import mb() and wmb() */
#include "RTSCTS_pkthdr.h"	/* Import pkthdr_t() */
#include "RTSCTS_route.h"	/* Import route_used[] */
#include "RTSCTS_debug.h"	/* Import protocol_debug_add() */
#include "RTSCTS_recv.h"	/* rtscts_recv prototype */
#include "Pkt_send.h"
#include "Pkt_module.h"		/* Import MYRPKT_MTU */
#include "Pkt_proc.h"
#include "printf.h"         /* for PRINTF() macro */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/* ************************************************************************** */
/*
** We pre-allocate all send pages. A given page can have one of the following
** statuses:
**     pg_empty		No physical page associated with this entry (yet)
**     pg_avail		The page is available and can be checked out.
**     pg_checked_out   The page is checked out (via get_snd_page()) and
**                          has not been submitted to myrPkt_xmit() yet.
**     pg_sending	The page has been submitted to myrPkt_xmit() and
**                          is being sent out.
*/
typedef enum {pg_avail= 1, pg_checked_out, pg_sending, pg_empty} pg_status_t;
typedef struct   {
    unsigned long page;
    pg_status_t stat;
    int slot;
} snd_page_t;

static int last_pkt= 0;
static int next_avail= 0;
static snd_page_t snd_page_list[MAX_SND_PKT_ENTRIES];

/* prototype for fn called by skb_xmit -- a RTSCTS_OVER_ETHERNET routine */
extern int rtscts_recv(unsigned long page, struct NETDEV *dev);

/******************************************************************************/

#ifdef BUFDEBUG
/*
** Return a pointer to the page that was sent "back" packet sends ago.
** Return NULL if there is no such page. Used by /proc/sndbuf.
*/
unsigned long *
getSndPage(int back, int *idx, int *dnid, int *stat)
{

int lidx;


    if ((last_pkt - back) < 0)   {
	/* Wrap around */
	lidx= MAX_SND_PKT_ENTRIES + (last_pkt - back);
    } else   {
	lidx= last_pkt - back;
    }

    /*
    ** We really want the pages in the order they have been sent; i.e. the
    ** order they are on the MCP side.
    */
    lidx= mcpshmem->snd_pkt_list[lidx].page_idx;
    *idx= lidx;
    *dnid= ntohl(mcpshmem->snd_pkt_list[lidx].dest);

    /* *stat == 0 means MCP has sent packet */
    *stat= ntohl(mcpshmem->snd_pkt_list[lidx].phys_addr);

    return (unsigned long *)snd_page_list[lidx].page;

}  /* end of getSndPage() */
#endif /* BUFDEBUG */

/* ************************************************************************** */
/*
** The workhorse. It assumes it's parameters have been checked, and submits
** the send request to the MCP.
*/
static void
pkt_send(int dest, unsigned long buf, int len, int page_idx)
{

unsigned long __flags;

    #ifndef NO_STATS
    if ( dest != TEST_ROUTE_ID ) {
      route_used[dest]++;
    }
    #endif /* NO_STATS */

    mb();
    save_flags(__flags);
    cli();
    mcpshmem->snd_pkt_list[last_pkt].len= htonl(len);
    mcpshmem->snd_pkt_list[last_pkt].dest= htonl(dest);
    /* The MCP doesn't look at the next field! */
    mcpshmem->snd_pkt_list[last_pkt].page_idx= page_idx;
    wmb();

    /* phys_addr must be last! */
    mcpshmem->snd_pkt_list[last_pkt].phys_addr= htonl(virt_to_bus((void *)buf));

    snd_page_list[page_idx].slot= last_pkt;
    last_pkt= (last_pkt + 1) % MAX_SND_PKT_ENTRIES;
    restore_flags(__flags);

    #ifndef NO_STATS
    hstshmem->total_snd_msgs++;
    hstshmem->total_snd_bytes += len;
    #endif /* NO_STATS */

}  /* end of pkt_send() */

/* ************************************************************************** */
/*
** To send a packet, the user of this module calls get_snd_page() to get the
** index of an available page. The user fills that page with data, using
** page_addr() to get the starting address of the page. When the page is
** ready to be sent, the user calls this function with the page index as a
** paramter to actually send it. "len" says how many bytes in the page
** should be sent.
*/
int
myrPkt_xmit(unsigned short dst_nid, int page_idx, int len)
{

pkthdr_t *pkthdr;
int idx;

    #ifndef NO_STATS
    proc_stat->snd_attempts++;
    #endif /* NO_STATS */

    if (!hstshmem->rdy_to_snd)   {
	/* No MCP loaded! */
    #ifndef NO_ERROR_STATS
	proc_stat->snd_dev_closed++;
    #endif /* NO_ERROR_STATS */
	snd_page_list[page_idx].stat= pg_avail;
	return -1;
    }

    /* Check to see if the MCP has room for another send. */
    if (mcpshmem->snd_pkt_list[last_pkt].phys_addr)   {
    #ifndef NO_ERROR_STATS
	proc_stat->snd_MCP_busy++;
    #endif /* NO_ERROR_STATS */
	snd_page_list[page_idx].stat= pg_avail;
	return -1;
    }

    /* Make two copies of the three crucial fields in the header */
    pkthdr= (pkthdr_t *)page_addr(page_idx);
    pkthdr->len = (unsigned int) len;
    pkthdr->type3= pkthdr->type2= pkthdr->type;
    pkthdr->msgID3= pkthdr->msgID2= pkthdr->msgID;
    pkthdr->src_nid= hstshmem->my_pnid;
    pkthdr->src_nid3= pkthdr->src_nid2= pkthdr->src_nid;
    pkthdr->len1= pkthdr->len2= len;

    protocol_debug_add(pkthdr->msgID, pkthdr->type, dst_nid, pkthdr->info,
	pkthdr->info2, TRUE);

    /*
    ** We know the MCP is done with the page that was in snd_pkt_list[last_pkt]
    ** because of the above test. Extract the index into the page list and
    ** mark it as available again. If the index is -1, then this has already
    ** been done.
    */
    idx= mcpshmem->snd_pkt_list[last_pkt].page_idx;
    if (idx != -1)   {
	if (snd_page_list[idx].stat != pg_sending)   {
	    printk("myrPkt_xmit() Huh?!? idx %d in entry %d\n", idx, last_pkt);
        #ifndef NO_ERROR_STATS
	    proc_stat->snd_err_not_sending++;
        #endif /* NO_ERROR_STATS */
	    snd_page_list[page_idx].stat= pg_avail;
	    return -1;
	}
	snd_page_list[idx].stat= pg_avail;
	mcpshmem->snd_pkt_list[last_pkt].page_idx= -1;
    }

    if ((page_idx < 0) || (page_idx >= MAX_SND_PKT_ENTRIES))   {
    #ifndef NO_ERROR_STATS
	proc_stat->snd_err_idx++;
    #endif /* NO_ERROR_STATS */
	return -1;
    }

    if (len <= 0)   {
    #ifndef NO_STATS
	proc_stat->snd_len++;
    #endif /* NO_STATS */
	snd_page_list[page_idx].stat= pg_avail;
	return -1;
    }

    if (len > MYRPKT_MTU)   {
	printk("Send len %d > MYRPKT_MTU %ld\n", len, MYRPKT_MTU);
    #ifndef NO_ERROR_STATS
	proc_stat->snd_err_len_mtu++;
    #endif /* NO_ERROR_STATS */
	snd_page_list[page_idx].stat= pg_avail;
	return -1;
    }

    if (dst_nid > MAX_NODES && dst_nid != TEST_ROUTE_ID)   {
	printk("myrPkt_xmit: Invalid destination %d (len %d)\n", dst_nid, len);
    #ifndef NO_ERROR_STATS
	proc_stat->snd_err_dest++;
    #endif /* NO_ERROR_STATS */
	snd_page_list[page_idx].stat= pg_avail;
	return -1;
    }

    pkt_send(dst_nid, snd_page_list[page_idx].page, len, page_idx); 
    snd_page_list[page_idx].stat= pg_sending;

    #ifndef NO_STATS
    proc_stat->snd_xmit++;
    #endif /* NO_STATS */
    return 0;

}  /* end of myrPkt_xmit() */

int
skb_xmit(unsigned short dst_nid, struct sk_buff* skb)
{
    /* transmit through ethernet */
    if (dst_nid == hstshmem->my_pnid) {
      rtscts_recv((unsigned long) (skb->data+ETH_HLEN), NULL);
      kfree_skb(skb);
    }
    else {
      if (dev_queue_xmit(skb) < 0) {
        printk("skb_xmit: dev xmit failed\n");
        return -1;
      }
    }
    return 0;

}  /* end of skb_xmit() */

/* ************************************************************************** */
/*
**         Send Page Management Functions
*/
/* ************************************************************************** */
/*
** Allocate the send pages and mark them as available
*/
int
pkt_init_send(void)
{

int i;


    /* Allocate the send pages */
    for (i= 0; i < MAX_SND_PKT_ENTRIES; i++)   {
	if (snd_page_list[i].page != 0L)   {
	    #ifdef VERBOSE
		printk("Reusing page 0x%016lx in entry %d\n",
		    snd_page_list[i].page, i);
	    #endif /* VERBOSE */
	} else   {
	    snd_page_list[i].page= get_free_page(GFP_ATOMIC);
	}
	if (snd_page_list[i].page == 0L)   {
	    printk("pkt_init_send() Failed to get %d send pages! (Got %d)\n",
		MAX_SND_PKT_ENTRIES, i);
	    pkt_free_snd_pages();
	    return -1;
	}
	snd_page_list[i].stat= pg_avail;
	snd_page_list[i].slot= -1;		/* Not on MCP yet */
    }
    last_pkt= 0;
    next_avail= 0;
    PRINTF(2)("pkt_init_send() pre-allocated %d send pages\n", i);

    return 0;

}  /* end of pkt_init_send() */

/* ************************************************************************** */
/*
** Free all the allocated pages and mark them as unavailable
*/
void
pkt_free_snd_pages(void)
{

int i;
int cnt;


    cnt= 0;
    for (i= 0; i < MAX_SND_PKT_ENTRIES; i++)   {
	if (mcpshmem != NULL)   {
	    mcpshmem->snd_pkt_list[i].phys_addr= 0;
	    mcpshmem->snd_pkt_list[i].len= 0;
	    mcpshmem->snd_pkt_list[i].dest= 0;
	    mcpshmem->snd_pkt_list[i].page_idx= -1;
	}
	
	if (snd_page_list[i].page != 0L)   {
	    free_page(snd_page_list[i].page);
	    cnt++;
	    snd_page_list[i].page= 0L;
	    snd_page_list[i].stat= pg_empty;
	    snd_page_list[i].slot= -1;
	}
    }
    printk("pkt_free_snd_pages() freed %d send pages\n", cnt);

}  /* end of pkt_free_snd_pages() */

/* ************************************************************************** */
/*
** Return the (kernel virtual) start address of the page with index "page_idx"
*/
unsigned long
page_addr(int page_idx)
{

unsigned long rc;


    if ((page_idx >= 0) && (page_idx < MAX_SND_PKT_ENTRIES))   {
	if (snd_page_list[page_idx].stat != pg_checked_out)   {
	    /* We should not be called with a page index we haven't given out */
	    printk("page_addr(idx %d) not checked out!\n", page_idx);
	    rc= 0x0;
	} else   {
	    rc= snd_page_list[page_idx].page;
	}
    } else   {
	rc= 0x0;
    }
    return rc;

} /* end of page_addr() */

/* ************************************************************************** */
/*
** Return the index of an available page. The sender fills the page with data
** and then calls myrPkt_xmit() with the index number to send it out.
*/
int
get_snd_page(void)
{

int rc;
int slot;


    rc= -1;
    if (snd_page_list[next_avail].stat == pg_avail)   {
	/* The page is available. Hand it out. */
	rc= next_avail;
	snd_page_list[next_avail].stat= pg_checked_out;
	next_avail= (next_avail + 1) % MAX_SND_PKT_ENTRIES;
    } else if (snd_page_list[next_avail].stat == pg_sending)   {
	slot= snd_page_list[next_avail].slot;
	if (mcpshmem->snd_pkt_list[slot].phys_addr == 0)   {
	    /* The MCP is done with this page. Make sure it is pointing at us */
	    if (mcpshmem->snd_pkt_list[slot].page_idx != next_avail)   {
		/* It's not pointing at us! */
		printk("slot %d is not pointing at us %d\n", slot, next_avail);
        #ifndef NO_ERROR_STATS
		proc_stat->snd_err_internal++;
        #endif /* NO_ERROR_STATS */
	    } else   {
		rc= next_avail;
		snd_page_list[next_avail].stat= pg_checked_out;
		mcpshmem->snd_pkt_list[slot].page_idx= -1;
		next_avail= (next_avail + 1) % MAX_SND_PKT_ENTRIES;
	    }
	} else   {
	    /* The MCP hasn't sent this page yet! Give up.  */
	    #ifdef VERBOSE
		printk("send MCP overrun\n");
	    #endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
	    proc_stat->snd_err_overrun++;
        #endif /* NO_ERROR_STATS */
	}
    } else   {
	/*
	** We checked for pg_avail and pg_sending, that leaves pg_empty and
	** pg_checked_out. pg_empty means we didn't get a physical page for
	** this entry. We should not be here in that case. pg_checked_out
	** means the RTSCTS portion of this module is getting send page
	** entries without submiting them for sending. That's an internal
	** usage error.
	*/
	printk("get_snd_page() internal (usage) error!\n");
	printk("   page index %d, status %d\n", next_avail,
	    snd_page_list[next_avail].stat);
    #ifndef NO_ERROR_STATS
	proc_stat->snd_err_internal2++;
    #endif /* NO_ERROR_STATS */

	/*
	** Take it anyway!
	** This is a workaround. If things work properly, then the
	** above message should never display. If you see it, it probably
	** means a code change somewhere in rtscts has a bug in it ;-)
	*/
	rc= next_avail;
	snd_page_list[next_avail].stat= pg_checked_out;
	next_avail= (next_avail + 1) % MAX_SND_PKT_ENTRIES;
    }

    return rc;

}  /* end of get_snd_page() */

/* ************************************************************************** */

void
free_snd_page(int index)
{
    /* Make sure we have a valid page index before we do this. */
    if (index >=0 && index < MAX_SND_PKT_ENTRIES)
        snd_page_list[index].stat= pg_avail;
        
}  /* end of free_snd_page() */

/* ************************************************************************** */

