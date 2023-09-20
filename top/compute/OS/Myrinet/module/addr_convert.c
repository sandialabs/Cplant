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
** $Id: addr_convert.c,v 1.6 2001/09/30 21:25:10 pumatst Exp $
** Translate a MAC address into a physical node ID
*/

#include <linux/kernel.h>	/* For printk() */
#include <linux/netdevice.h>	/* Import struct device */
#include <asm/byteorder.h>	/* For ntohs() */
#include "addr_convert.h"
#include "cTask/cTask.h"


/*> <----------------------------------><----------------------------------> <*/
/*
** Addresses:
** To avoid ARP and keep things simple, we mandate the following
** addressing scheme. Every node has a physical node ID (pnid)
** from the Portals module. They range from 0 to n-1, where n is
** the total number of nodes in the system.
** MAC addresses will contain this pnid + 1 such that when printed in
** the usual hexadecimal notation, the pnid + 1 shows up in decimal.
** For example, on node 112 ifconfig would show the MAC as
** 00:00:80:00:01:13.
** IP addresses, when printed in the usual fashion, display the
** pnid + 1 = host id, added to the network id. 
** Here are some more examples for the network 10.10.0.0:
**     pnid	MAC			IP
**          0	00:00:80:00:00:01	10.10.0.1
**          1	00:00:80:00:00:02	10.10.0.2
**         10   00:00:80:00:00:11	10.10.0.11
**         99	00:00:80:00:01:00	10.10.0.100
**        100   00:00:80:00:01:01	10.10.0.101
**        254   00:00:80:00:02:55	10.10.0.255
**        255   00:00:80:00:02:56	10.10.1.0
**        256   00:00:80:00:02:56	10.10.1.1
 64*256=16384	00:00:80:01:63:85	10.10.64.1
*/

/*> <----------------------------------><----------------------------------> <*/
/*
** Convert a MAC address and return the correspnding pid. We return -1
** if the conversion failed.
** Our "fake" MAC addresses look like 00:00:80:00:01:12 which we translate
** into node 111. Every byte in the MAC address has a range of 0..99. The
** LSB contains the 1's, the second most significant byte contains the 100's
** and so on.
** Note that MAC addresses are displayed in hexadecimal, so we have to
** convert the individual bytes.
*/
int
mac2pnid(unsigned char *mac)
{

int pnid;
int m1, m100, m10000;


    /*
    ** We only decode 3 bytes (nobody has a cluster this big! ;-) Make
    ** sure the others are 00 and 0x80.
    */
    if ((mac[0] != 0) || (mac[1] != 0) || (mac[2] != 0x80))   {
	#ifdef VERBOSE
	    printk("mac2pnid() invalid mac 0x%02x %02x %02x %02x %02x %02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	#endif /* VERBOSE */
	return -1;
    }

    /* Convert them to decimal */
    m1= (mac[5] % 16) + ((mac[5] / 16) * 10);
    m100= (mac[4] % 16) + ((mac[4] / 16) * 10);
    m10000= (mac[3] % 16) + ((mac[3] / 16) * 10);

    /* Do a range check */
    if ((m1 > 99) || (m100 > 99) || (m10000 > 99))   {
	#ifdef VERBOSE
	    printk("mac2pnid() Out of range 0x %02d %02d %02d\n",
		m10000, m100, m1);
	#endif /* VERBOSE */
	return -1;
    }

    /* Now add them up */
    pnid= m1 + 100 * m100 + 10000 * m10000 - 1;


    #ifdef VERBOSE
	printk("mac2pnid() addr 0x%02x:%02x:%02x:%02x:%02x:%02x is %d\n",
	    ntohs(mac[0]), ntohs(mac[1]),ntohs(mac[2]),
	    ntohs(mac[3]), ntohs(mac[4]),ntohs(mac[5]), pnid);
    #endif /* VERBOSE */

    return pnid;

}  /* end of mac2pnid() */

/*> <----------------------------------><----------------------------------> <*/
/*
** Create a MAC address of the form 00:00:80:00:01:12 from a physical node
** ID of 111 (in this example).
** Note that MAC addresses are displayed in hexadecimal, so we have to
** convert the individual bytes.
*/
void
pnid2mac(unsigned char *mac, int pnid)
{


int m1, m100, m10000;
int orig_pnid;


    orig_pnid= pnid;

    /* We only do the first three bytes (1000000 nodes!) */
    if (pnid >= 1000000)   {
	mac[0]= mac[1]= mac[2]= mac[3]= mac[4]= mac[5]= 0xff;
	return;
    }

    pnid++;
    m1= pnid % 100;
    pnid -= m1;
    pnid /= 100;
    m1= (m1 % 10) + ((m1 / 10) * 16);
    m100= pnid % 100;
    pnid -= m100;
    pnid /= 100;
    m100= (m100 % 10) + ((m100 / 10) * 16);
    m10000= pnid % 100;
    m10000= (m10000 % 10) + ((m10000 / 10) * 16);

    mac[5]= m1;
    mac[4]= m100;
    mac[3]= m10000;
    mac[2]= 0x80;
    mac[1]= 0;
    mac[0]= 0;

    #ifdef VERBOSE
	printk("pnid2mac() phys %d is 0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n",
	    orig_pnid, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    #endif /* VERBOSE */

}  /* end of pnid2mac() */

/*> <----------------------------------><----------------------------------> <*/
/*
** Convert a pnid; e.g. 111 to an IP address: 111 --> 10.0.0.112 
                                                    = 167772272
*/
int
pnid2ip(int pnid)
{

int ip=0, network, netmask;

    network = cTaskGetNetwork(); 
    netmask = cTaskGetNetmask(); 

    pnid++;

    if (pnid > ~netmask) {
      printk("pnid+1 0x%x overflows ~netmask 0x%x\n",
                       pnid, ~netmask);
      return -1;
    }

    ip = network + pnid;
    return ip;
}  /* end of pnid2ip() */

/*> <----------------------------------><----------------------------------> <*/

int
ip2pnid(unsigned int ip)
{

int pnid;
unsigned int netmask;

    netmask = cTaskGetNetmask();

    ip = (unsigned int) ntohl(ip);

    pnid = (int) ((ip & ~netmask)-1);

//  printk("ip2pnid: ip= 0x%x, pnid= %d\n", ip, pnid); 

    return pnid;

}  /* end of ip2pnid() */

/*> <----------------------------------><----------------------------------> <*/

/*
** Given an IP address, calculate the MAC address (that will be passed to
** myrIP_xmit())
*/
int
ip2mac(void *buff, struct NETDEV *dev, unsigned int dest, struct sk_buff *skb)
{
  struct ethhdr *eth;
  int pnid;


    eth= (struct ethhdr *)buff;

    #ifdef VERBOSE
	printk("myrIP: ip2mac() dest %d, 0x%x\n", dest, dest);
    #endif /* VERBOSE */

    pnid= ip2pnid(dest);

    memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
    pnid2mac((unsigned char *)&(eth->h_dest), pnid);
    return 0;

}  /* end of ip2mac() */

/*> <----------------------------------><----------------------------------> <*/
