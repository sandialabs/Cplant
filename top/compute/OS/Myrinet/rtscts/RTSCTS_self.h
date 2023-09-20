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
#ifndef RTSCTS_SELF_H
#define RTSCTS_SELF_H

#define SS_BSIZE 131072

static char data_start[SS_BSIZE];
struct task_struct *global_send_task;
nal_cb_t *global_nal;
lib_msg_t *global_cookie;

static int p3_self_send(nal_cb_t *nal, void *buf, size_t len, int dst_nid, ptl_hdr_t *hdr, lib_msg_t *cookie);
static int p3_self_recv(nal_cb_t *nal, void *private, 
        void *data, size_t mlen, size_t rlen, lib_msg_t *cookie);

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

extern __inline__ int
p3_self_send(nal_cb_t *nal, void *buf, size_t len, int dst_nid, ptl_hdr_t *hdr,
             	lib_msg_t *cookie)
{

unsigned long flags;
unsigned int payload;
int type;
struct task_struct *send_task;
#ifdef KERNEL_ADDR_CACHE
    void *addr_key = NULL;
#endif
nal_cb_t *rnal;
xfer_t xfer;
int rc;

    save_flags(flags);
    cli();

    #ifdef KERNEL_ADDR_CACHE
	if (cookie) {
	    addr_key = cookie->md->addrkey;
	}
    #endif /* KERNEL_ADDR_CACHE */

    #ifdef VERY_VERBOSE
	printk("p3_self_send() buf %p, len %ld, nid %d, hdr %p\n",
	    buf, len, dst_nid, hdr);
    #endif /* VERY_VERBOSE */

    /*
    ** Error checking on parameters
    */
    if (len < 0)   {
	printk("p3_self_send() Invalid length %ld\n", len);
	lib_finalize(nal, (void *)21, cookie);

	restore_flags(flags);
	return -1;
    }


    /*
    ** Everything OK so far. Build IP packet. It consists of the
    ** the packet header, and data (including headers from higher
    ** level protocols).
    **
    ** +----------+------   ---+
    ** | pkthdr_t | data       |    <--- We Don't...
    ** +----------+------   ---+
    **  ^          ^
    **  |          |
    **  |          +--- data_start
    **  +--- pkthdr_start
    */

    payload= len;
    type= P3_LAST_RTS;

    if (get_cb(hdr->src.nid, hdr->src.pid, (void *)&send_task, NULL) != nal)  {
      printk("p3_self_send: get_cb(nid %d, pid %d) != nal\n", hdr->src.nid, hdr->src.pid);
      lib_finalize(nal, (void *)24, cookie);
      restore_flags(flags);
      return -4;
    }

#if 1
    global_send_task = send_task;
    global_nal = nal;
    global_cookie = cookie;
#else
    if (payload > 0)   {
	__gcc_barrier();
#ifdef KERNEL_ADDR_CACHE
	rc= memcpy3_from_user(data_start, buf, payload, addr_key, send_task);
	__gcc_barrier();
	if (rc < 0)   {
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
	    if (!msgID)   {
		#ifdef VERBOSE
		    printk( "p3_send: memcpy3_from_user(%p, %p, %d) failed\n",
			data_start, buf, payload);
		#endif /* VERBOSE */
		send_sig(SIGPIPE, send_task, 1);
		/* lib_finalize(nal, (void *)25, cookie); */
	    }
	    restore_flags(flags);
	    return -5;
	}
#else
	rc= memcpy2(FROM_USER, send_task, data_start, buf, payload);
	__gcc_barrier();
	if (rc != 0)   {
	  printk( "p3_self_send: memcpy2(FROM_USER, %p, %p, %d) failed\n",
			data_start, buf, payload);
	  send_sig(SIGPIPE, send_task, 1);
	  /* lib_finalize(nal, (void *)25, cookie); */
	  restore_flags(flags);
	  return -5;
	}
#endif
    }
    rc= lib_finalize(nal, (void *)28, cookie);
    if (rc != 0)   {
      printk("p3_self_send: lib_finalize() failed for PTL_EVENT_SENT\n");
      send_sig(SIGPIPE, send_task, 1);
    }
#endif

/******** RECEIVE BEGINS *********/

/* begin handleP3 */
    rnal= get_cb(hdr->nid, hdr->pid, (void *)(&xfer.task), NULL);
    if (rnal == NULL)   {
      printk("p3_self_send: nid %d pid %d, has no NAL CB\n", hdr->nid, hdr->pid);
      /* send_sig(SIGPIPE, xfer.task, 1); */

      /* receive will never happen since target proc is gone, make sure PTL_EVENT_SENT happens */
      rc= lib_finalize(nal, (void *)28, cookie);
      if (rc != 0)   {
        printk("p3_self_send: lib_finalize() failed for PTL_EVENT_SENT\n");
        send_sig(SIGPIPE, send_task, 1);
      }

      restore_flags(flags);
      return 0;
    }

    xfer.pkt= (char*)buf;
    xfer.len= len;
    xfer.hdr= hdr;
    xfer.type= type;
    lib_parse(rnal, hdr, (void *)&xfer);
/* end   handleP3 */

    restore_flags(flags);
    return 0;

}  /* end of p3_self_send() */

extern __inline__ int
p3_self_recv(nal_cb_t *nal, void *private, 
        void *data, size_t mlen, size_t rlen, lib_msg_t *cookie)
{

xfer_t *xfer;
size_t len, togo, pay;
int rc;
char *srcbuf, *dstbuf;
#ifdef KERNEL_ADDR_CACHE
void *addr_key = NULL;
#endif

    #ifdef KERNEL_ADDR_CACHE
	if (cookie) {
	    addr_key = cookie->md->addrkey;
	}
    #endif /* KERNEL_ADDR_CACHE */

    #ifdef VERY_VERBOSE
	printk("p3_self_recvBody() data %p, mlen %ld, rlen %ld\n", data, mlen, rlen);
    #endif /* VERY_VERBOSE */

    xfer= (xfer_t *)private;
    if (xfer->type != P3_LAST_RTS)   {
	printk("p3_self_recvBody: Invalid type %d! \n", xfer->type);
	return -1;
    }

    len= MIN(mlen, xfer->len);

    if (len > 0)   {

       togo = len;
       srcbuf = xfer->pkt;
       dstbuf = data;
       while (togo > 0) {

         pay= MIN(togo,SS_BSIZE);

	 __gcc_barrier();
	 rc= memcpy2(FROM_USER, global_send_task, data_start, srcbuf, pay);
	 __gcc_barrier();
	 if (rc < 0) {
           send_sig(SIGPIPE, global_send_task, 1);
         }
	 rc = memcpy2(TO_USER, xfer->task, dstbuf, data_start, pay);
	 if (rc < 0) {
	   send_sig(SIGPIPE, xfer->task, 1);
         }
         srcbuf += pay;
         dstbuf += pay;
         togo -= pay;
       }
    }
    /* this one is done on behalf of the send -- had to wait till we
       were done w/ the buffer
    */
    rc= lib_finalize(global_nal, (void *)28, global_cookie);
    if (rc != 0)   {
      printk("p3_self_recv: lib_finalize() failed for PTL_EVENT_SENT\n");
      send_sig(SIGPIPE, global_send_task, 1);
    }

    if ((xfer->len >= mlen) || (xfer->type == P3_LAST_RTS))   {
	/* That was it, send a MSGEND */

	rc= lib_finalize(nal, (void *)15, cookie);

	if (rc != 0)   {
	  /* lib_finalize() didn't work right. Send signal to app */
	  printk("p3_self_recvBody: lib_finalize() for short msg failed\n");
	  send_sig(SIGPIPE, xfer->task, 1);
	}
    }
    return rlen;

}  /* end of p3_self_recvBody() */

#endif
