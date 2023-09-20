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
** $Id: dispatch.c,v 1.2 2000/06/26 19:54:04 rolf Exp $
*/
/*
 * dispatch_table.c
 *
 * Automatically generated from the api.h file by idl
 *
 */
#include <lib-p30.h>
#include <p30/lib-dispatch.h>


dispatch_table_t dispatch_table[] = {
	/* 0 */	{do_PtlGetId, "PtlGetId"},
	/* 1 */	{do_PtlTransId, "PtlTransId"},
	/* 2 */	{do_PtlNIStatus, "PtlNIStatus"},
	/* 3 */	{do_PtlNIDist, "PtlNIDist"},
	/* 4 */	{do_PtlNIDebug, "PtlNIDebug"},
	/* 5 */	{do_PtlMEAttach, "PtlMEAttach"},
	/* 6 */	{do_PtlMEInsert, "PtlMEInsert"},
	/* 7 */	{do_PtlMEUnlink, "PtlMEUnlink"},
	/* 8 */	{do_PtlTblDump, "PtlTblDump"},
	/* 9 */	{do_PtlMEDump, "PtlMEDump"},
	/* 10 */	{do_PtlMDAttach_internal, "PtlMDAttach_internal"},
	/* 11 */	{do_PtlMDInsert_internal, "PtlMDInsert_internal"},
	/* 12 */	{do_PtlMDBind_internal, "PtlMDBind_internal"},
	/* 13 */	{do_PtlMDUpdate_internal, "PtlMDUpdate_internal"},
	/* 14 */	{do_PtlMDUnlink_internal, "PtlMDUnlink_internal"},
	/* 15 */	{do_PtlEQAlloc_internal, "PtlEQAlloc_internal"},
	/* 16 */	{do_PtlEQFree_internal, "PtlEQFree_internal"},
	/* 17 */	{do_PtlACEntry, "PtlACEntry"},
	/* 18 */	{do_PtlPut, "PtlPut"},
	/* 19 */	{do_PtlGet, "PtlGet"},
	/*    */	{0, ""}
};
