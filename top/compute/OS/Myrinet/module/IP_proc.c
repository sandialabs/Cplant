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
** $Id: IP_proc.c,v 1.7 2001/09/24 18:31:07 rolf Exp $
** Provide Myrinet IP driver information through /proc
*/

#include <linux/proc_fs.h>
#include "IP_proc.h"
#include "defines.h"		/* For MIN() */
#include "versions.h"		/* For version_strings[] */


static proc_stat_t __proc_stat;
proc_stat_t *proc_stat= &__proc_stat;


#ifdef USE_PROC_FS

    #define MAX_PRINT_BUF		(4 * 1024)
    char print_buf[MAX_PRINT_BUF];


#ifdef LINUX24
    int
    myrIPproc(char *buf, char **start, off_t off, int len, int *eof, void *data)
#else
    int
    myrIPproc(char *buf, char **start, off_t off, int len, int unused)
#endif
    {
    int copy_len;
    static char *pb;

	*start= buf;

	if (off == 0)   {
	    /* Create a "screen image" of what we want to print */
	    pb= print_buf;
	    pb += sprintf(pb, "Sends:\n");
	    pb += sprintf(pb, "    attempted                %12ld\n",
			proc_stat->snd_attempts);
	    pb += sprintf(pb, "    packets sent             %12ld\n",
			proc_stat->snd_xmit);
	    pb += sprintf(pb, "    total bytes              %12ld\n",
			proc_stat->total_snd_bytes);
	    pb += sprintf(pb, "        skb or dev == NULL   %12ld\n",
			proc_stat->snd_bad_arg);
	    pb += sprintf(pb, "        dev not open         %12ld\n",
			proc_stat->snd_dev_closed);
	    pb += sprintf(pb, "        dev busy             %12ld\n",
			proc_stat->snd_dev_busy);
	    pb += sprintf(pb, "        skb->len too short   %12ld\n",
			proc_stat->snd_len);
	    pb += sprintf(pb, "    -------------------------------------\n");
	    pb += sprintf(pb, "    total rejected           %12ld\n\n",
			proc_stat->snd_bad_arg + proc_stat->snd_dev_closed +
			proc_stat->snd_dev_busy + proc_stat->snd_len);
	    pb += sprintf(pb, "        len < 0              %12ld\n",
			proc_stat->snd_err_len);
	    pb += sprintf(pb, "        len > MTU            %12ld\n",
			proc_stat->snd_err_len_mtu);
	    pb += sprintf(pb, "        invalid destination  %12ld\n",
			proc_stat->snd_err_dest);
	    pb += sprintf(pb, "        skb crosses page bnd %12ld\n",
			proc_stat->snd_err_xpage);
	    pb += sprintf(pb, "    -------------------------------------\n");
	    pb += sprintf(pb, "    total errors             %12ld\n\n",
			proc_stat->snd_len + proc_stat->snd_err_len_mtu +
			proc_stat->snd_err_dest + proc_stat->snd_err_xpage);
	    pb += sprintf(pb, "    IP_send() errors         %12ld\n",
			proc_stat->snd_errors);
	    pb += sprintf(pb, "    freed skb                %12ld\n",
			proc_stat->snd_skb_free);
	    pb += sprintf(pb, "    unlinked skb             %12ld\n",
			proc_stat->snd_skb_unlink);



	    pb += sprintf(pb, "\nReceives:\n");
	    pb += sprintf(pb, "    packets to Linux         %12ld\n",
			proc_stat->rcv_pkt);
	    pb += sprintf(pb, "    total bytes to Linux     %12ld\n",
			proc_stat->total_rcv_bytes);
	    pb += sprintf(pb, "    skb allocated            %12ld\n",
			proc_stat->rcv_skb_alloc);
	    pb += sprintf(pb, "    unable to alloc skb      %12ld\n",
			proc_stat->rcv_skb_alloc_fail);
	    pb += sprintf(pb, "    skb crosses page bnd     %12ld\n",
			proc_stat->rcv_skb_alloc_xpage);
	    pb += sprintf(pb, "    len > MTU                %12ld\n",
			proc_stat->rcv_len1);
	    pb += sprintf(pb, "    total dropped            %12ld\n",
			proc_stat->rcv_dropped);
	    pb += sprintf(pb, "    dropped by netif_rx()    %12ld\n",
			proc_stat->netif_rx_dropped);
	    pb += sprintf(pb, "    backlog nearly full      %12ld\n",
			proc_stat->rcv_backoff);
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
        *eof= 1;
#endif
	return copy_len;

    }  /* end of myrIPproc() */

    void
    myrIPprocInit(void)
    {
	memset(proc_stat, 0, sizeof(proc_stat_t));
    }  /* end of myrIPprocInit() */

    /* ---------------------------------------------------------------------- */

#ifdef LINUX24
    int
    versions_proc(char *buf, char **start, off_t off, int len, int *eof,
                  void *data)
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

#ifdef LINUX24
        *eof= 1;
#endif
	return copy_len;

    }  /* end of versions_proc() */

#endif /* USE_PROC_FS */

/*> <----------------------------------><----------------------------------> <*/
