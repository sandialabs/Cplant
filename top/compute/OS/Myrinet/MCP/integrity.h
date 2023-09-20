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
** $Id: integrity.h,v 1.1 2000/09/12 22:32:07 pumatst Exp $
*/

#ifndef _INTEGRITY_H_
#define _INTEGRITY_H_

#include "MCPshmem.h"

/* test patterns */
int pattern[] = { 0x00000000, 0xffffffff, 0xaaaaaaaa, 0x55555555,
                  0x5a5a5a5a, 0xa5a5a5a5, 0xf0f0f0f0, 0x0f0f0f0f,
                  0xff00ff00, 0x00ff00ff, 0xffff0000, 0x0000ffff,
                  0xaa55aa55, 0x55aa55aa, 0xaaaa5555, 0x5555aaaa
                };

/* test pattern strings */
const char* stringp [] = { "0x00000000", "0xffffffff", "0xaaaaaaaa", 
                            "0x55555555", "0x5a5a5a5a", "0xa5a5a5a5", 
                            "0xf0f0f0f0", "0x0f0f0f0f", "0xff00ff00", 
                            "0x00ff00ff", "0xffff0000", "0x0000ffff",
                            "0xaa55aa55", "0x55aa55aa", "0xaaaa5555", 
                            "0x5555aaaa" 
                         };

/* integrity test buffer strings -- should match enums
   defined in integrity-enum.h */
const char* stringib [] = { "snd_buf_A", "snd_buf_B", 
                            "rcv_buf_A", "rcv_buf_B" 
                          };
#endif
