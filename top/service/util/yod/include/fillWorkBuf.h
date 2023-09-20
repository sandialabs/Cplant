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
/* $Id: fillWorkBuf.h,v 1.3 2001/02/16 05:44:34 lafisk Exp $ */


#ifndef FILL_WORK_BUF_H
#define FILE_WORK_BUF_H

#include "defines.h"



INT32
fillWorkBuf(int buf_num, UINT32 nnbytes, UINT32 offset, 
	UINT16 nid, UINT16 pid, int rptl);

#endif
