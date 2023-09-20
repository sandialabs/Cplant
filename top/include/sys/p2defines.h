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
** $Id: p2defines.h,v 1.1 2001/02/16 05:37:30 lafisk Exp $
**
** Definitions only used by Portals 2.  Delete this soon.
*/

#ifndef P2DEFINES
#define P2DEFINES

#include "defines.h"

typedef union {
#ifdef __PUMACUBE2__
#if defined(__GNUC__) || defined(__NCC__)
        UINT64  dword;
#endif
#endif
    struct {
        UINT32  rl;
        UINT32  ru;
    }           word;
    struct {
        UINT16  rll;
        UINT16  rlu;
        UINT16  rul;
        UINT16  ruu;
    }           hword;
    struct {
        UINT8   rlll;
        UINT8   rllu;
        UINT8   rlul;
        UINT8   rluu;
        UINT8   rull;
        UINT8   rulu;
        UINT8   ruul;
        UINT8   ruuu;
    }           byte;
} REG64;

typedef struct {
    UINT32 i0;
    UINT32 i1;
} INTS;

typedef struct {
    UINT16 s0;
    UINT16 s1;
    UINT16 s2;
    UINT16 s3;
} SHORTS;

typedef struct {
    UINT8 b0;
    UINT8 b1;
    UINT8 b2;
    UINT8 b3;
    UINT8 b4;
    UINT8 b5;
    UINT8 b6;
    UINT8 b7;
} BYTES;

/******************************************************************************/

typedef union {
    INTS ints;
    double ll;
    SHORTS shorts;
    BYTES bytes;
} CHAMELEON;

typedef struct {
    UINT16 node;
    UINT16 index;
} PROCESS_VECTOR;

typedef struct {
    UINT16 node;
    UINT16 index;
    UINT16 portal;
    UINT16 pad16;
} PORTAL_VECTOR;


typedef UINT8 PORTAL_INDEX;
#define NUM_PORTALS (64)
#define HDR_USER_BYTES (12)


#endif
