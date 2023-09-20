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
** $Id: MCP.h,v 1.37 2002/02/14 18:38:12 jbogden Exp $
** Some definitions and types used by the MCP
*/

#ifndef MCP_H
#define MCP_H


#ifndef TRUE
    #define TRUE	(1)
#endif /* TRUE */
#ifndef FALSE
    #define FALSE	(0)
#endif /* FALSE */

#ifdef ENABLE_CRC32
    #if defined (L7) || defined (L9)
	/* Only LANai 7.x have CRC32 capability */
	#define USE_32BIT_CRC
    #else
	#undef USE_32BIT_CRC
    #endif
#endif

/* To be put into the type field of the Myrinet header */
#define MYRI_PORTAL_PACKET_TYPE	(0x0800)	/* 2 bytes */

#define MYRI_SELF_TEST_TYPE	(0x0803)	/* 2 bytes */
#define MYRI_SELF_DIRECT_TYPE	(0x0804)	/* 2 bytes */


#define MYRI_MyriData_TYPE	(0x0005)	/* Myricom data packet */
#define MYRI_MyriMap_TYPE	(0x0006)	/* Myricom map packet */
#define MYRI_MyriProbe_TYPE	(0x0007)	/* Myricom probe packet */
#define MYRI_MyriOption_TYPE	(0x0008)	/* Myricom option packet */

/******************************************************************************/
/*
** Assertions
*/
#if defined(ASSERT)
    #define fail_if(num, expr)    if (expr) fault(num,0,0)

    #define fail_if2(num,expr,param1,param2)    if (expr) fault(num,param1,param2)
#else
    #define fail_if(num, expr)
#endif /* ASSERT */

/* Assertion numbers */
#define a01	(101)







#define a09	(109)
#define a10	(110)


#define a13	(113)
#define a14	(114)
#define a15	(115)
#define a16	(116)
#define a17	(117)
#define a18	(118)
#define a19	(119)
#define a20	(120)
#define a21	(121)
#define a22	(122)
#define a23	(123)
#define a24	(124)
#define a25	(125)
#define a26	(126)
#define a27	(127)
#define a28	(128)
#define a29	(129)
#define a30	(130)
#define a31	(131)
#define a32	(132)
#define a33	(133)
#define a34	(134)
#define a35	(135)
#define a36	(136)
#define a37	(137)
#define a38	(138)
#define a39	(139)
#define a40	(140)
#define a41	(141)
#define a42	(142)
#define a43	(143)
#define a44	(144)

#define a46	(146)
#define a47	(147)
#define a48	(148)
#define a49	(149)

#define a51	(151)
#define a52	(152)
#define a53	(153)
#define a54	(154)
#define a55	(155)

#define a57	(157)
#define a58	(158)
#define a59	(159)
#define a60	(160)
#define a61	(161)
#define a62	(162)
#define a63	(163)
#define a64	(164)
#define a65	(165)

#define a67	(167)
#define a68	(168)

#define a70	(170)
#define a71	(171)
#define a72	(172)
#define a73	(173)
#define a74	(174)
#define a75	(175)
#define a76	(176)
#define a77	(177)
#define a78	(178)

#define a80	(180)
#define a81	(181)
#define a82	(182)
#define a83	(183)
#define a84	(184)
#define a85	(185)
#define a86	(186)
#define a87	(187)
#define a88	(188)
#define a89	(189)
#define a90	(190)
#define a91	(191)
#define a92	(192)
#define a93	(193)
#define a94	(194)
#define a95	(195)
#define a96	(196)

#define a98	(198)
#define a99	(199)
#define a100	(200)
#define a101	(201)
#define a102	(202)
#define a103	(203)
#define a104	(204)

#endif /* MCP_H */
