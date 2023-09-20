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
** $Id: lib_myrnal.c,v 1.55 2001/08/22 16:51:31 pumatst Exp $
*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>		/* For kmalloc() */
#include <linux/vmalloc.h>	/* For vmalloc() */
#include <linux/interrupt.h>
#include <asm/uaccess.h>	/* For access_ok() */
#include <sys/defines.h>        /* For TRUE */

#include <p30.h>
#include <lib-p30.h>
#include <p30/lib-dispatch.h>
#include <p30/lib-nal.h>
#include "myrnal.h"
#include "cb_table.h"		/* For cb table functions */
#include "runtime.h"		/* For getrank() getgid() */
#include "devices.h"		/* For p3find_dev() */
#include "lib_myrnal.h"


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
/*
** Count the number of messages sent and received from other nodes. At the
** end of a run they should all add up, if the underlying msg passing is
** correct. A message in this context can be a put, ack, get, or reply.
*/
#define MSG_CNT_DEBUG
#undef MSG_CNT_DEBUG
#ifdef MSG_CNT_DEBUG
    #define MAX_NUM_NIDS	(512)
    unsigned int num_rcvd[MAX_NUM_NIDS];
    unsigned int num_sent[MAX_NUM_NIDS];
#endif /* MSG_CNT_DEBUG */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

typedef struct {
    int num_nodes;	/* How many nodes in my group */
    int slot;		/* Which device in the p3dev table */
} myrnal_data_t;

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/* pid_in, gid_in and rid_in are unused by the Myrinet NAL */
static int
myrnal_send(nal_cb_t *nal, void *private, lib_msg_t *cookie, ptl_hdr_t *hdr,
	int dnid, int pid_in, int gid_in, int rid_in,
	user_ptr data, size_t len)
{

int slot;


    slot= ((myrnal_data_t *)(nal->nal_data))->slot;
    #ifdef VERBOSE
    printk("myrnal_send() buf %p, len %ld, dpnid %d, slot %d, nid %d, pid %d\n",
	data, len, dnid, slot, hdr->nid, hdr->pid);
    #endif /* VERBOSE */

    /*
    ** The following check can go away when the P30 library actually
    ** implements pre-checking of user address spaces. Until then,
    ** we need a little protection here.
    **
    ** Just found out, this doesnn't always work on the Alpha. Some
    ** invalid addresses will fail, while others don't. We'll use
    ** copy_from_user() in the rtscts module which actually accesses
    ** the memory and does a more thorough check.
    */
    if (access_ok(VERIFY_READ, data, len) == 0)   {
	printk("myrnal_send() buf %p, len %ld no read access!\n", data, len);
	return PTL_INV_MD;
    }

    #ifdef MSG_CNT_DEBUG
	if ((dnid >= 0) && (dnid < MAX_NUM_NIDS))   {
	    num_sent[dnid]++;
	}
    #endif /* MSG_CNT_DEBUG */

    return p3send(slot, nal, data, len, dnid, hdr, cookie);

}  /* end of myrnal_send() */

#ifdef MSG_CNT_DEBUG
    #include <p30/lib-types.h>
#endif /* MSG_CNT_DEBUG */

static int
myrnal_recv(nal_cb_t *nal, void *private, lib_msg_t *cookie, 
            user_ptr data,
	    size_t mlen, size_t rlen)
{

int slot;


    #ifdef VERBOSE
    printk("Calling myrnal_recv() mlen %ld, rlen %ld\n", mlen, rlen);
    #endif /* VERBOSE */

    #ifdef MSG_CNT_DEBUG
	/*
	** I know this is extremly ugly, but it is just for debugging. Also
	** see RTSCTS_p3.c in top/compute/OS/Myrinet/rtscts. (Search for xfer.)
	*/
	typedef struct   {
	    struct task_struct *task;
	    void *pkt;
	    unsigned long len;
	    unsigned int msgID;
	    unsigned int msgNum;
	    int type;
	    ptl_hdr_t *hdr;
	} xfer_t;

	xfer_t *xfer;
	int snid;

	xfer= (xfer_t *)private;

	snid= xfer->hdr->src.nid;
	if ((snid >= 0) && (snid < MAX_NUM_NIDS))   {
	    num_rcvd[snid]++;
	}
    #endif /* MSG_CNT_DEBUG */

    slot= ((myrnal_data_t *)(nal->nal_data))->slot;
    return p3recv(slot, nal, private, data, mlen, rlen, cookie);

}  /* end of myrnal_recv() */


static int
myrnal_write(nal_cb_t *nal, void  *private, user_ptr dst_addr,
	void *src_addr, size_t len)
{

long spid;
void *task;


    if (get_cb(nal->ni.nid, nal->ni.pid, &task, &spid) != nal)   {
	#ifdef VERBOSE
	/* Process has probably gone away in the meantime */
	printk("myrnal_write() nal != get_cb() nid %d, pid %d\n", nal->ni.nid,
	    nal->ni.pid);
	#endif /* VERBOSE */
	return -1;
    }

    /* on the x86, doing copy_to_user() here invariably causes 
       a page fault -- if in an interrupt, then do_page_fault() 
       chokes (returns a "no context" error). presumably this 
       problem goes away if we add a mechanism for touching the 
       event queues as in the way we "validate" msg buffers... 
       oddly enough, memcpy2() works ok during the interrupt...
    */
    if ((spid != current->pid) || in_interrupt())   {
	#ifdef VERBOSE
	    printk("myrnal_write() Using memcpy2() %p -> %p (len %ld)\n",
		src_addr, dst_addr, len);
	#endif /* VERBOSE */
        if ( memcpy2user(task, dst_addr, src_addr, len) != 0 ) {
	    #ifdef VERBOSE
		printk("myrnal_write(): memcpy2user() failed...\n");
	    #endif /* VERBOSE */
	    return -1;
        }
	return 0;
    } else   {
	if (copy_to_user(dst_addr, src_addr, len) != 0)   {
	    #ifdef VERBOSE
		printk("myrnal_write() %p -> %p (len %ld) FAILED\n", src_addr,
		    dst_addr, len);
	    #endif /* VERBOSE */
	    return -1;
	} else   {
	    #ifdef VERBOSE
	    printk("myrnal_write() %p -> %p (len %ld) OK\n", src_addr, dst_addr,
		len);
	    #endif /* VERBOSE */
	    return 0;
	}
    }

}  /* end of myrnal_write() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

static void *
myrnal_malloc(nal_cb_t *nal, size_t size)
{
    void *rc=NULL;

    if ( (rc = vmalloc(size)) == NULL ) {
	printk("myrnal_malloc(): vmalloc() failed for %ld bytes\n",size);
    }

    return rc; 
}  /* end of myrnal_malloc() */

static void
myrnal_free(nal_cb_t *nal, void *ptr)
{
    vfree(ptr);
}  /* end of myrnal_free() */

static void
invalidate(nal_cb_t *nal, void *base, size_t extent, void *addrkey)
{
#ifdef KERNEL_ADDR_CACHE
  addrCache_tblLink(addrkey, nal->ni.pid);
#else
  return;
#endif
}  /* end of invalidate() */

static int
validate(nal_cb_t *nal, void *base, size_t extent, void **addrkey)
{
#ifdef KERNEL_ADDR_CACHE
  return addrCache_populate( (unsigned long) base, (int) extent, nal->ni.pid, addrkey );
#else
  return 0;  
#endif
}


static void
myrnal_printf(nal_cb_t *nal, const char *fmt, ...)
{

va_list args;
char buf[1024];			/* Same size as the printk buffer */


    va_start(args, fmt);
    vsprintf(buf, fmt, args);	/* No vsnprintf available in kernel */
    va_end(args);
    printk(buf);

}  /* end of myrnal_printf */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

static void
myrnal_cli(nal_cb_t *nal, unsigned long *flags)
{
    save_flags(*flags);
    cli();
}  /* end of myrnal_cli() */


static void
myrnal_sti(nal_cb_t *nal, unsigned long *flags)
{
    restore_flags(*flags);
}  /* end of myrnal_sti() */


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** Translate a gid/rid into a nid/pid
*/
static int
myrnal_gidrid2nidpid(nal_cb_t *nal, ptl_id_t gid, ptl_id_t rid,
	ptl_id_t *nid, ptl_id_t *pid)
{
    /* The Portals 2 module only has one map, so we ignore gid */
    *nid= p3rank2pnid(rid);
    *pid= p3rank2ppid(rid);
    return 0;

}  /* end of myrnal_gidrid2nidpid() */


/*
** Translate a nid/pid into a gid/rid
*/
static int
myrnal_nidpid2gidrid(nal_cb_t *nal, ptl_id_t nid, ptl_id_t pid,
	ptl_id_t *gid, ptl_id_t *rid)
{

    /*
    ** Don't really know how to do this cheaply yet.
    ** I also know the p3 library doesn't need this yet
    */
    *gid= -1;
    *rid= -1;
    return 0;

}  /* end of myrnal_nidpid2gidrid() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** Calculate the number of hops between nodes
*/
static int
myrnal_dist( nal_cb_t *nal, ptl_id_t nid, unsigned long *dist )
{
    int slot;

    slot = ((myrnal_data_t *)(nal->nal_data))->slot;

    if ( p3dist( slot, nal, nid, dist ) ) {
	return -1;
    }
    
    return 0;

}

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int
open_lib_myrnal(void)
{

nal_cb_t *cb;
myrnal_data_t *myrnal_data;
int rc;
int rid, gid, num_nodes;
int nid, pid;
int index;


    cb= (nal_cb_t *)kmalloc(sizeof(nal_cb_t), GFP_KERNEL);
    if (!cb)   {
	printk("open_lib_myrnal: kmalloc nal\n");
	return -1;
    }
    cb->nal_data= (void *)kmalloc(sizeof(myrnal_data_t), GFP_KERNEL);
    if (!cb->nal_data)   {
	kfree(cb);
	printk("open_lib_myrnal: kmalloc nal_data\n");
	return -1;
    }

    /* See if this process is registered with the system (old Portals module) */
    if (checktask() != TRUE)   {
	printk("open_lib_myrnal: checktask\n");
	return -1;
    }

    rid= getrank();
    gid= getgid();
    nid= getnid();
    pid= getpid();
    num_nodes= getnum_nodes();

    myrnal_data= (myrnal_data_t *)(cb->nal_data);
    myrnal_data->num_nodes= num_nodes;
    myrnal_data->slot= p3find_dev();	/* Get a device to talk to */
    if (myrnal_data->slot < 0)   {
	printk("open_lib_myrnal: myrnal_data slot\n");
	return -1;
    }

    cb->cb_send= myrnal_send;
    cb->cb_recv= myrnal_recv;
    cb->cb_write= myrnal_write;
    cb->cb_malloc= myrnal_malloc;
    cb->cb_free= myrnal_free;
    cb->cb_invalidate= invalidate;
    cb->cb_validate= validate;
    cb->cb_printf= myrnal_printf;
    cb->cb_cli= myrnal_cli;
    cb->cb_sti= myrnal_sti;
    cb->cb_gidrid2nidpid= myrnal_gidrid2nidpid;
    cb->cb_nidpid2gidrid= myrnal_nidpid2gidrid;
    cb->cb_dist= myrnal_dist;

    index= spid2index(current->pid);
    #ifdef VERBOSE
    printk("open_lib_myrnal() entering index %d, nid %d, pid %d (spid %d)\n",
	index, nid, pid, current->pid);
    #endif /* VERBOSE */

    #ifdef MSG_CNT_DEBUG
	/*
	** We assume only the last process to open the myrnal is of
	** interest...
	*/
	{
	    int i;

	    for (i= 0; i < MAX_NUM_NIDS; i++)   {
		num_rcvd[i]= 0;
		num_sent[i]= 0;
	    }
	}
    #endif /* MSG_CNT_DEBUG */

    if (enter_cb(cb, index, nid, pid) != TRUE)   {
	printk("open_lib_myrnal: enter_cb, index=%d, nid=%d, pid=%d\n",
	                                        index, nid, pid);
	rc= -1;
    } else   {
	rc= lib_init(cb, nid, pid, rid, gid, num_nodes, MYRNAL_MAX_PTL_SIZE,
		MYRNAL_MAX_ACL_SIZE);
        if ( rc ) { 
          rc = -1;
          printk("open_lib_myrnal: lib_init\n");
	}
    }

    if (rc == -1)   {
	/* Something went wrong */
	printk("open_lib_myrnal() freeing because of error\n");
	kfree(cb->nal_data);
	kfree(cb);
    }

    return rc;

}  /* end of open_lib_myrnal() */


/* Free the control block(s) for this process. */
void
close_lib_myrnal(void)
{

nal_cb_t *cb;


    while (TRUE)   {
	cb= getclr_cb(current->pid);
	if (cb != NULL)   {
	    #ifdef VERBOSE
	    printk("close_lib_myrnal() freeing data structs for spid %d\n",
		current->pid);
	    #endif /* VERBOSE */

	    #ifdef MSG_CNT_DEBUG
		/*
		** We assume only the last process to open the myrnal is of
		** interest...
		*/
		{
		    int i;
		    int header_done;


		    header_done= FALSE;
		    for (i= 0; i < MAX_NUM_NIDS; i++)   {
			if ((num_rcvd[i] != 0) || (num_sent[i] != 0))   {
			    if (!header_done)   {
				printk("Nid:    Rcvd    Sent\n");
				header_done= TRUE;
			    }
			    printk("%4d: %6d  %6d\n", i, num_rcvd[i],
				num_sent[i]);
			}
			num_rcvd[i]= 0;
			num_sent[i]= 0;
		    }
		}
	    #endif /* MSG_CNT_DEBUG */

	    lib_fini(cb);
	    kfree(cb->nal_data);
	    kfree(cb);
	} else   {
	    return;
	}
    }

}  /* end of close_lib_myrnal() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
myrnal_up(void)
{
    init_cb();
}  /* end of myrnal_init() */


void
myrnal_down(void)
{
    free_cb();
}  /* end of myrnal_down() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
