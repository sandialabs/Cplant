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
** $Id: Pkt_proc.h,v 1.15 2001/08/22 16:29:06 pumatst Exp $
*/
#ifndef PKT_PROC_H
#define PKT_PROC_H

#define PRINT_LINE_LEN		(81)	/* Must be exact, including \n */
#define PRINT_USAGE_LINE_LEN	(23)	/* Must be exact, including \n */

#ifdef LINUX24
extern int myrPktProc(char *buf, char **start, off_t off, int len, int *eof, void *data);
extern int routesProc(char *buf, char **start, off_t off, int len, int *eof, void *data);
extern int routesUsageProc(char *buf, char **start, off_t off, int len, int *eof, void *data);
extern int read_proto_debug_proc(char *buf, char **start, off_t off, int len, int *eof, void *data);
#ifdef BUFDEBUG
extern int listSndBuf(char *buf, char **start, off_t off, int len, int *eof, void *data);
extern int listRcvBuf(char *buf, char **start, off_t off, int len, int *eof, void *data);
#endif /* BUFDEBUG */
#else
extern int myrPktProc(char *buf, char **start, off_t off, int len, int unused);
extern int routesProc(char *buf, char **start, off_t off, int len, int unused);
extern int routesUsageProc(char *buf, char **start, off_t off, int len,
		int unused);
extern int read_proto_debug_proc(char *buf, char **start, off_t off,
		int len, int unused);

#ifdef BUFDEBUG
extern int listSndBuf(char *buf, char **start, off_t off, int len, int unused);
extern int listRcvBuf(char *buf, char **start, off_t off, int len, int unused);
#endif /* BUFDEBUG */
#endif

extern void myrPktProcInit(void);
extern void routesProcInit(void);
extern void routes_usedProcInit(void);


typedef struct   {
    unsigned long snd_attempts;		/* Calls to myrPkt_xmit() */
    unsigned long snd_xmit;		/* Num of successful sends */

    unsigned long snd_dev_closed;	/* No send: dev not open */
    unsigned long snd_MCP_busy;		/* No send: MCP is busy (overrun) */
    unsigned long snd_err_not_sending;	/* Sent page not in sending state */
    unsigned long snd_err_len_mtu;	/* Snd err: len > MTU */
    unsigned long snd_err_idx;		/* Snd err: bad index */
    unsigned long snd_err_len;		/* Snd err: len < 0 */
    unsigned long snd_err_dest;		/* Snd err: invalid destination */

    unsigned long snd_len;		/* Snd err: len too short */
    unsigned long snd_err_internal;	/* Snd err: pointer mixup */
    unsigned long snd_err_internal2;	/* Snd err: internal usage wrong */
    unsigned long snd_err_overrun;	/* Snd err: Snd buffers exhausted */

    unsigned long rcv_ints;		/* Total receive interrupts */
    unsigned long rcv_ints_ignored;	/* ignored intrrpts (no HOST_SIG_BIT) */
    unsigned long rcv_ints_not_mcp;	/* interrupts not from MCP */
    unsigned long rcv_ints_rcv;		/* receive interrupts */
    unsigned long rcv_ints_fault;	/* fault interrupts */
    unsigned long rcv_ints_warning;	/* warning interrupts */
    unsigned long rcv_ints_init;	/* init interrupts */
    unsigned long rcv_ints_dma_setup;	/* LANai 7.x DMA setup interrupts */
    unsigned long rcv_ints_dma_e2l_test;/* test e2l bus DMA */
    unsigned long rcv_ints_dma_l2e_test;/* test l2e bus DMA */
    unsigned long rcv_ints_dma_e2l_integrity;	/* integrity test e2l bus DMA */
    unsigned long rcv_ints_dma_l2e_integrity;	/* integrity test l2e bus DMA */
    unsigned long rcv_ints_no_reason;	/* no reson interrupts */
    unsigned long rcv_ints_unknown;	/* unknown interrupts */

    unsigned long rcv_page_alloc;	/* Rcv page allocated */
    unsigned long rcv_page_free;	/* Rcv page freed */
    unsigned long rcv_timeout;		/* Rcv timeout (waiting for data) */
    unsigned long rcv_len1;		/* Rcv len > MTU */
    unsigned long rcv_len2;		/* MCP out of pages */
    unsigned long rcv_len3;		/* Rcv len > page */
    unsigned long rcv_len4;		/* Rcv len < sizeof(pkthdr_t) */
    unsigned long rcv_len5;		/* Rcv len != pkthdr->len1 */
    unsigned long rcv_page_alloc_fail;	/* Rcv page alloc failed */
    unsigned long rcv_restart;		/* Rcv processed > 1 pkt / interrupt */
    unsigned long rcv_max_restart;	/* Max pkt processed in 1 interrupt */
    unsigned long rcv_dropped;		/* dropped by driver */
    unsigned long rcv_dropped_crc;	/* dropped due to bad CRC */
    unsigned long rcv_dropped_trunc;	/* dropped due to truncation */
    unsigned long rcv_rtscts;		/* calls to rtscts_recv() */

    unsigned long hdr_err_len;		/* error in pkt hdr len field */
    unsigned long hdr_err_type;		/* error in pkt hdr type field */
    unsigned long hdr_err_src_nid;	/* error in pkt hdr src_nid field */
    unsigned long hdr_err_msgID;	/* error in pkt hdr msgID field */
    unsigned long hdr_wrn_type;		/* fixed error in pkt hdr type field */
    unsigned long hdr_wrn_src_nid;	/* fixed err in pkt hdr src_nid field */
    unsigned long hdr_wrn_msgID;	/* fixed error in pkt hdr msgID field */

} proc_stat_t;

extern proc_stat_t *proc_stat;

#endif /* PKT_PROC_H */
