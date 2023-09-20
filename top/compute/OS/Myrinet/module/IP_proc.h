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
** $Id: IP_proc.h,v 1.4 2001/08/22 16:01:15 pumatst Exp $
*/
#ifndef IP_PROC_H
#define IP_PROC_H

#ifdef LINUX24
extern int myrIPproc(char *buf, char **start, off_t off, int len, int *eof,
                     void *data);
extern int versions_proc(char *buf, char **start, off_t off, int len, int *eof,
                         void *data);
#else
extern int myrIPproc(char *buf, char **start, off_t off, int len, int unused);
extern int versions_proc(char *buf, char **start, off_t off, int len, int unused);
#endif
extern void myrIPprocInit(void);


typedef struct   {
    unsigned long snd_attempts;		/* Calls to myrIP_xmit() */
    unsigned long snd_bad_arg;		/* Bad skb or dev arg to myrIP_xmit */
    unsigned long snd_dev_closed;	/* No send: dev not open */
    unsigned long snd_dev_busy;		/* No send: dev busy */
    unsigned long snd_len;		/* Snd err: skb->len too short */
    unsigned long snd_errors;		/* Unsuccessful sends (tx_errors) */
    unsigned long snd_err_len;		/* Snd err: len < 0 */
    unsigned long snd_err_len_mtu;	/* Snd err: len > MTU */
    unsigned long snd_err_dest;		/* Snd err: invalid destination */
    unsigned long snd_err_xpage;	/* Snd err: skb crosses page boundary */
    unsigned long snd_skb_free;		/* Send skb freed */
    unsigned long snd_skb_unlink;	/* Send skb unlinked */
    unsigned long snd_xmit;		/* Total number of packets sent */
    unsigned long total_snd_bytes;	/* Total bytes sent */

    unsigned long rcv_dropped;		/* Pkts we dropped & saw being dropped*/
    unsigned long rcv_len1;		/* Rcv len > MTU */
    unsigned long rcv_backoff;		/* backlog is almost full, drop pkt */
    unsigned long rcv_skb_alloc;	/* Rcv skb allocated */
    unsigned long rcv_skb_alloc_fail;	/* Rcv skb couldn't be allocated */
    unsigned long rcv_skb_alloc_xpage;	/* Rcv skb alloc crosses page bndry */
    unsigned long netif_rx_dropped;	/* Dropped by netif_rx() */
    unsigned long total_rcv_bytes;	/* Total bytes passed to netif_rx() */
    unsigned long rcv_pkt;		/* Number of pkts passed to netif_rx */

} proc_stat_t;

extern proc_stat_t *proc_stat;

#endif /* IP_PROC_H */
