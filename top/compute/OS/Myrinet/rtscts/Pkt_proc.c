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
** $Id: Pkt_proc.c,v 1.45.2.4 2002/08/05 18:50:12 jbogden Exp $
** Provide Myrinet Packet driver information through /proc
*/

#include <linux/proc_fs.h>
#include <asm/byteorder.h>	/* Import ntohl() */
#include <asm/uaccess.h>
#include <sys/defines.h>
#include "MCPshmem.h"		/* Import MAX_NUM_ROUTES */
#include "Pkt_proc.h"
#include "RTSCTS_recv.h"	/* Import route_status[] */
#include "RTSCTS_route.h"	/* Import ROUTE_* */
#include "RTSCTS_protocol.h"	/* Import pkthdr_t */
#include "RTSCTS_debug.h"	/* Import protocol_debug_proc() */
#include "Pkt_recv.h"		/* Import getRcvPage() */
#include "Pkt_send.h"		/* Import getSndPage() */
#include "MCPshmem.h"
#include "hstshmem.h"
#include "RTSCTS_proc.h"    /* for MAX_PRINT_BUF */


static proc_stat_t __proc_stat;
proc_stat_t *proc_stat= &__proc_stat;

#define DISP_PKTS	(32)	/* how many packets to display */


#ifdef USE_PROC_FS

/* use the print_buf allocated in RTSCTS_proc.c */
extern char print_buf[MAX_PRINT_BUF];


#ifdef LINUX24
int
myrPktProc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
myrPktProc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
long avg;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	if (mcpshmem == NULL)   {
	    pb += sprintf(pb, "MCP not loaded\n");
	} else   {
	    pb += sprintf(pb, "Sends:\n");
	    pb += sprintf(pb, "    attempted                %16ld\n",
			proc_stat->snd_attempts);
	    pb += sprintf(pb, "    successful               %16ld\n",
			proc_stat->snd_xmit);
	    pb += sprintf(pb, "    total bytes sent         %16ld\n",
			hstshmem->total_snd_bytes);
	    if (proc_stat->snd_xmit != 0)   {
		avg= hstshmem->total_snd_bytes / proc_stat->snd_xmit;
	    } else   {
		avg= hstshmem->total_snd_bytes;
	    }
	    pb += sprintf(pb, "    average len sent         %16ld\n\n", avg);
	    pb += sprintf(pb, "        dev not open         %16ld\n",
			proc_stat->snd_dev_closed);
	    pb += sprintf(pb, "        MCP busy             %16ld\n",
			proc_stat->snd_MCP_busy);
	    pb += sprintf(pb, "        send buffer not ready%16ld\n",
			proc_stat->snd_err_not_sending);
	    pb += sprintf(pb, "        len > MTU            %16ld\n",
			proc_stat->snd_err_len_mtu);
	    pb += sprintf(pb, "        bad buffer index     %16ld\n",
			proc_stat->snd_err_idx);
	    pb += sprintf(pb, "        len < 0              %16ld\n",
			proc_stat->snd_err_len);
	    pb += sprintf(pb, "        invalid destination  %16ld\n",
			proc_stat->snd_err_dest);
	    pb += sprintf(pb, "    ---------------------------------------------\n");
	    pb += sprintf(pb, "    total rejected           %16ld\n\n",
			proc_stat->snd_dev_closed + proc_stat->snd_MCP_busy +
			proc_stat->snd_err_not_sending +
			proc_stat->snd_err_len_mtu + proc_stat->snd_err_idx +
			proc_stat->snd_err_len + proc_stat->snd_err_dest);
	    pb += sprintf(pb, "        len <= 0             %16ld\n",
			proc_stat->snd_len);
	    pb += sprintf(pb, "        internal ptr mixup   %16ld\n",
			proc_stat->snd_err_internal);
	    pb += sprintf(pb, "        internal usage err   %16ld\n",
			proc_stat->snd_err_internal2);
	    pb += sprintf(pb, "        Snd bufs exhausted   %16ld\n",
			proc_stat->snd_err_overrun);
	    pb += sprintf(pb, "    ---------------------------------------------\n");
	    pb += sprintf(pb, "    total errors             %16ld\n\n",
			proc_stat->snd_len + proc_stat->snd_err_internal +
			proc_stat->snd_err_internal2 +
			proc_stat->snd_err_overrun);


	    pb += sprintf(pb, "\nInterrupts:\n");
	    pb += sprintf(pb, "        ignored              %16ld\n",
			proc_stat->rcv_ints_ignored);
	    pb += sprintf(pb, "        not from MCP         %16ld\n",
			proc_stat->rcv_ints_not_mcp);
	    pb += sprintf(pb, "        receive pkt          %16ld\n",
			proc_stat->rcv_ints_rcv);
	    pb += sprintf(pb, "        faults               %16ld\n",
			proc_stat->rcv_ints_fault);
	    pb += sprintf(pb, "        warnings             %16ld\n",
			proc_stat->rcv_ints_warning);
	    pb += sprintf(pb, "        initialization       %16ld\n",
			proc_stat->rcv_ints_init);
	    pb += sprintf(pb, "        setup LANai 7.x DMA  %16ld\n",
			proc_stat->rcv_ints_dma_setup);
	    pb += sprintf(pb, "        test e2l DMA         %16ld\n",
			proc_stat->rcv_ints_dma_e2l_test);
	    pb += sprintf(pb, "        test l2e DMA         %16ld\n",
			proc_stat->rcv_ints_dma_l2e_test);
	    pb += sprintf(pb, "        integrity test e2l   %16ld\n",
			proc_stat->rcv_ints_dma_e2l_integrity);
	    pb += sprintf(pb, "        integrity test l2e   %16ld\n",
			proc_stat->rcv_ints_dma_l2e_integrity);
	    pb += sprintf(pb, "        no reason            %16ld\n",
			proc_stat->rcv_ints_no_reason);
	    pb += sprintf(pb, "        unknown              %16ld\n",
			proc_stat->rcv_ints_unknown);
	    pb += sprintf(pb, "    ---------------------------------------------\n");
	    pb += sprintf(pb, "    total calls to handler   %16ld\n",
			proc_stat->rcv_ints);



	    pb += sprintf(pb, "\nReceives:\n");
	    pb += sprintf(pb, "    successful               %16ld\n",
			proc_stat->rcv_rtscts);
	    pb += sprintf(pb, "    total bytes received     %16ld\n",
			hstshmem->total_rcv_bytes);
	    if (proc_stat->rcv_rtscts != 0)   {
		avg= hstshmem->total_rcv_bytes / proc_stat->rcv_rtscts;
	    } else   {
		avg= hstshmem->total_rcv_bytes;
	    }
	    pb += sprintf(pb, "    average len received     %16ld\n\n",
			avg);
	    pb += sprintf(pb, "    pages allocated          %16ld\n",
			proc_stat->rcv_page_alloc);
	    pb += sprintf(pb, "    unable to alloc page     %16ld\n",
			proc_stat->rcv_page_alloc_fail);
	    pb += sprintf(pb, "    pages freed              %16ld\n",
			proc_stat->rcv_page_free);
	    pb += sprintf(pb, "    handled > 1 pkt/int      %16ld\n",
			proc_stat->rcv_restart);
	    pb += sprintf(pb, "    max pkt handled in 1 int %16ld\n",
			proc_stat->rcv_max_restart);
	    pb += sprintf(pb, "    timeout (in int)         %16ld\n",
			proc_stat->rcv_timeout);
	    pb += sprintf(pb, "    len > MTU                %16ld\n",
			proc_stat->rcv_len1);
	    pb += sprintf(pb, "    MCP indicated error      %16ld\n",
			proc_stat->rcv_len4);
	    pb += sprintf(pb, "    len > pkt buffer         %16ld\n",
			proc_stat->rcv_len3);
	    pb += sprintf(pb, "    MCP ran out of buffers   %16ld\n",
			proc_stat->rcv_len2);
	    pb += sprintf(pb, "    len != pkthdr->len1      %16ld\n",
			proc_stat->rcv_len5);
	    pb += sprintf(pb, "    dropped by driver        %16ld\n",
			proc_stat->rcv_dropped);
	    pb += sprintf(pb, "    dropped by MCP           %16lu\n",
			ntohl(mcpshmem->counters.dumped));
	    pb += sprintf(pb, "    dropped due to bad CRC   %16ld\n",
			proc_stat->rcv_dropped_crc);
	    pb += sprintf(pb, "    dropped due to truncate  %16ld\n",
			proc_stat->rcv_dropped_trunc);
	    pb += sprintf(pb, "    Pkt hdr with fixed type  %16ld\n",
			proc_stat->hdr_wrn_type);
	    pb += sprintf(pb, "    Pkt hdr with fixed srcnid%16ld\n",
			proc_stat->hdr_wrn_src_nid);
	    pb += sprintf(pb, "    Pkt hdr with fixed msgID %16ld\n",
			proc_stat->hdr_wrn_msgID);
	    pb += sprintf(pb, "    Pkt hdr with bad Length  %16ld\n",
			proc_stat->hdr_err_len);
	    pb += sprintf(pb, "    Pkt hdr with bad type    %16ld\n",
			proc_stat->hdr_err_type);
	    pb += sprintf(pb, "    Pkt hdr with bad src_nid %16ld\n",
			proc_stat->hdr_err_src_nid);
	    pb += sprintf(pb, "    Pkt hdr with bad msgID   %16ld\n",
			proc_stat->hdr_err_msgID);
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

#ifdef LINUX24
    *eof = 1;
#endif
    return copy_len;

}  /* end of myrPktProc() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifdef LINUX24
int
read_proto_debug_proc(char *buf, char **start, off_t off, int len, int *eof,
                      void *data )
#else
int
read_proto_debug_proc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
static char *pb;


    *start= buf;

    if (off == 0)   {
	/* Create a "screen image" of what we want to print */
	pb= print_buf;
	protocol_debug_proc(&pb, pb + MAX_PRINT_BUF);
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

}  /* end of read_proto_debug_proc() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifdef BUFDEBUG
#ifdef LINUX24
    int
    listSndBuf(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
    int
    listSndBuf(char *buf, char **start, off_t off, int len, int unused)
#endif
    {

    int copy_len;
    int i;
    int idx;
    int dnid;
    int mcpstat;
    pkthdr_t *pkthdr;
    static char *pb;


	*start= buf;

	if (off == 0)   {
	    /* Create a "screen image" of what we want to print */
	    pb= print_buf;
	    if (mcpshmem == NULL)   {
		pb += sprintf(pb, "MCP not loaded\n");
	    } else   {
		pb += sprintf(pb, "Last %d Send Packet Buffers\n",
			DISP_PKTS);
		for (i= 1; i <= DISP_PKTS; i++)   {
		    pkthdr= (pkthdr_t *)getSndPage(i, &idx, &dnid, &mcpstat);
		    if (pkthdr == NULL)   {
			pb += sprintf(pb, "%2d -->> NULL\n", i);
			continue;
		    }
		    if (pkthdr->msgID == 0)   {
			pb += sprintf(pb, "%2d -->> msg ID 0x0\n", i);
			continue;
		    }
		    pb += sprintf(pb, "%2d  dnid %4d: %s", i, dnid,
			    protocol_name_str[pkthdr->type]);
		    pb += sprintf(pb, "page %4d, seq %4d, msgNum %4d, msg ID 0x%08x, "
			"info 0x%08x   ", idx,
			pkthdr->seq, pkthdr->msgNum, pkthdr->msgID, pkthdr->info);
		    if (mcpstat == 0)   {
			pb += sprintf(pb, "mcp sent pkt\n");
		    } else   {
			pb += sprintf(pb, "mcp send pending\n");
		    }

		    /* Don't overrun the print buffer */
		    if ((pb + 128) > (print_buf + MAX_PRINT_BUF))   {
			break;
		    }
		}
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

#ifdef LINUX24
        *eof = 1;
#endif
	return copy_len;

    }  /* end of listSndBuf() */

    /* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifdef LINUX24
    int
    listRcvBuf(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
    int
    listRcvBuf(char *buf, char **start, off_t off, int len, int unused)
#endif
    {

    int copy_len;
    int i;
    pkthdr_t *pkthdr;
    static char *pb;


	*start= buf;

	if (off == 0)   {
	    /* Create a "screen image" of what we want to print */
	    pb= print_buf;
	    pb += sprintf(pb, "Last %d Receive Packet Buffers\n", DISP_PKTS);
	    for (i= 1; i <= DISP_PKTS; i++)   {
		pkthdr= (pkthdr_t *)getRcvPage(i);
		if (pkthdr == NULL)   {
		    pb += sprintf(pb, "%2d -->> NULL\n", i);
		    continue;
		}
		#ifdef PKTHDR_CLEAR
		    /*
		    ** If the real packet header has been zeroed out,
		    ** then a copy is right behind the original in the
		    ** data portion of the packet.
		    */
		    pkthdr++;
		#endif /* PKTHDR_CLEAR */

		if (pkthdr->msgID == 0)   {
		    pb += sprintf(pb, "%2d -->> msg ID 0x0\n", i);
		    continue;
		}
		if ((pkthdr->type < CTS) ||
			(pkthdr->type >= LAST_ENTRY_DO_NOT_MOVE))  {
		    pb += sprintf(pb, "%2d -->> INVAL %2d, seq %4d, msg ID "
                            "0x%08x, info 0x%08x, info2 0x%08x: ", i,
                            pkthdr->type, pkthdr->seq, pkthdr->msgID,
                            pkthdr->info, pkthdr->info2);
		} else   {
		    pb += sprintf(pb, "%2d -->> %s seq %4d, msgNum %4d, msg ID 0x%08x, "
			    "info 0x%08x, info2 0x%08x: ", i,
			    protocol_name_str[pkthdr->type], pkthdr->seq, pkthdr->msgNum,
			    pkthdr->msgID, pkthdr->info, pkthdr->info2);
		}

		if (pkthdr->version == 0x80000000)   {
		    pb += sprintf(pb, "processed\n");
		} else   {
		    pb += sprintf(pb, "skipped\n");
		}
		/* Don't overrun the print buffer */
		if ((pb + 128) > (print_buf + MAX_PRINT_BUF))   {
		    break;
		}
	    }
	}

	copy_len= MIN(len, (pb - print_buf) - off);
	if (copy_len >= 0)   {
	    memcpy(buf, print_buf + off, copy_len);
	} else   {
	    copy_len= 0;
	}

	return copy_len;

    }  /* end of listRcvBuf() */
#endif /* BUFDEBUG */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

void
myrPktProcInit(void)
{

    memset(proc_stat, 0, sizeof(proc_stat_t));
    hstshmem->total_snd_msgs= 0;
    hstshmem->total_snd_bytes= 0;
    hstshmem->total_rcv_bytes= 0;
    mcpshmem->counters.dumped= 0;

}  /* end of myrPktProcInit() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifdef LINUX24
int
routesProc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
routesProc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
int start_line;
int end_line;
int line;
int rb;
char *rb_ptr;
unsigned int byte;
static char *pb;


    *start= buf;
    pb= print_buf;
    if (mcpshmem == NULL)   {
	pb += sprintf(pb, "MCP not loaded\n");
	copy_len= MIN(len, pb - print_buf);
	if (copy_len < 0)   {
	    copy_len= 0;
	}

	__copy_to_user(buf, print_buf, copy_len);
#ifdef LINUX24
        *eof = 1;
#endif
	return copy_len;
    }

    start_line= off / PRINT_LINE_LEN;
    if (start_line >= MAX_NUM_ROUTES)   {
#ifdef LINUX24
        *eof = 1;
#endif
	return 0;
    }

    end_line= MIN(MAX_PRINT_BUF, len) / PRINT_LINE_LEN + start_line;
    if (off == 0)   {
	pb += sprintf(pb, "dnid  Route                                                         Status      \n");
	end_line--;
    } else   {
	start_line--;
	end_line--;
    }

    if (end_line > MAX_NUM_ROUTES)   {
	end_line= MAX_NUM_ROUTES;
    }


    /* Create a "screen image" of what we want to print */
    for (line= start_line; line < end_line; line++)   {
	pb += sprintf(pb, "%-4d : ", line);
	rb_ptr= (char *)mcpshmem->route_copy[line];

	rb_ptr++; /* Skip the first 0x00 byte for printing */
	for (rb= 1; rb < MAX_ROUTE_LEN; rb++)   {
	    byte= *rb_ptr++;
	    byte= byte & 0xff;
	    if ((byte >= 0x80) && (byte <= 0x8f))   {
		pb += sprintf(pb, "%3d ", byte - 0x80);
	    } else if ((byte >= 0xb1) && (byte <= 0xbf))   {
		pb += sprintf(pb, "%3d ", (int)byte - 0xc0);
	    } else if (byte == 0)   {
		pb += sprintf(pb, "___ ");
	    } else   {
		pb += sprintf(pb, "??? ");
	    }
	}
	switch (route_status[line])   {
	    case ROUTE_UNTESTED:
		pb += sprintf(pb, " not_tested  ");
		break;
	    case ROUTE_NO_ANSWER:
		pb += sprintf(pb, " no_answer   ");
		break;
	    case ROUTE_BAD_DNID:
		pb += sprintf(pb, " bad_dnid    ");
		break;
	    case ROUTE_OK:
		pb += sprintf(pb, " tested_okay ");
		break;
	    case ROUTE_NOT_SET:
		pb += sprintf(pb, " no_route_set");
		break;
	    case ROUTE_CONFUSING_REPLY:
		pb += sprintf(pb, " confsng rply");
		break;
	}
	pb += sprintf(pb, "\n");
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(len, pb - print_buf);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf, copy_len);
    } else   {
	copy_len= 0;
    }

    #ifdef VERBOSE
	printk("routesProc() start line %d, end_line %d, off %ld, len %d, "
	    "copy len %d\n", start_line, end_line, off, len, copy_len);
    #endif /* VERBOSE */

#ifdef LINUX24
    *eof = 1;
#endif
    return copy_len;

}  /* end of routesProc() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

#ifdef LINUX24
int
routesUsageProc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
int
routesUsageProc(char *buf, char **start, off_t off, int len, int unused)
#endif
{

int copy_len;
int start_line;
int end_line;
int line;
static char *pb;


    *start= buf;
    pb= print_buf;

    start_line= off / PRINT_USAGE_LINE_LEN;
    if (start_line >= MAX_NUM_ROUTES)   {
#ifdef LINUX24
        *eof = 1;
#endif
	return 0;
    }

    end_line= MIN(MAX_PRINT_BUF, len) / PRINT_USAGE_LINE_LEN + start_line;
    if (off == 0)   {
	pb += sprintf(pb, "dnid  Num Pkts\n");
	end_line--;
    } else   {
	start_line--;
	end_line--;
    }

    if (end_line > MAX_NUM_ROUTES)   {
	end_line= MAX_NUM_ROUTES;
    }


    /* Create a "screen image" of what we want to print */
    for (line= start_line; line < end_line; line++)   {
	pb += sprintf(pb, "%-4d: %16ld\n", line, route_used[line]);
    }

    /* Don't overrun the print buffer */
    if (pb > (print_buf + MAX_PRINT_BUF))   {
	pb= print_buf + MAX_PRINT_BUF;
    }

    copy_len= MIN(len, pb - print_buf);
    if (copy_len >= 0)   {
	memcpy(buf, print_buf, copy_len);
    } else   {
	copy_len= 0;
    }

    #ifdef VERBOSE
	printk("routesUsageProc() start line %d, end_line %d, off %ld, len %d, "
	    "copy len %d\n", start_line, end_line, off, len, copy_len);
    #endif /* VERBOSE */

#ifdef LINUX24
    *eof = 1;
#endif
    return copy_len;

}  /* end of routesUsageProc() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

void
routesProcInit(void)
{

int i;


    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	route_status[i]= ROUTE_UNTESTED;
    }

}  /* end of routesProcInit() */

/* -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */

void
routes_usedProcInit(void)
{

int i;


    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	route_used[i]= 0;
    }

}  /* end of routes_usedProcInit() */

#endif /* USE_PROC_FS */

/* ************************************************************************** */
