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
/* $Id: CMDhandlerTable.c,v 1.9 2001/02/16 05:42:18 lafisk Exp $ */

#include "defines.h"
#include "CMDhandlerTable.h"

#ifdef LINUX_PORTALS
#include "rpc_msgs.h"
#endif

VOID (*host_cmd_handler_tbl[])(control_msg_handle *mh) =
{
	CMDhandler_access,
	CMDhandler_chdir,
	CMDhandler_chmod,
	CMDhandler_chown,
	CMDhandler_close,
	0,	/* WMD */

#ifndef LINUX_PORTALS
	CMDhandler_echo,
	CMDhandler_exit,
#endif

	CMDhandler_fcntl,
	CMDhandler_fstat,
	CMDhandler_fstatfs,
	CMDhandler_fsync,
	CMDhandler_ftruncate,
	CMDhandler_getdirentries,
	CMDhandler_gethostname,
#if 0
	CMDhandler_getgid,
	CMDhandler_getuid,
	CMDhandler_getegid,
	CMDhandler_geteuid,
	CMDhandler_setgid,
	CMDhandler_setuid,
	CMDhandler_setegid,
	CMDhandler_seteuid,
#endif
	CMDhandler_iomode,
	CMDhandler_link,
	CMDhandler_lseek,
	CMDhandler_lstat,
	CMDhandler_mass_murder,
	CMDhandler_mkdir,
	CMDhandler_noop,
	CMDhandler_open,
	CMDhandler_read,
	CMDhandler_rename,
	CMDhandler_rmdir,
	CMDhandler_setiomode,
	CMDhandler_stat,
	CMDhandler_statfs,
	CMDhandler_symlink,
	CMDhandler_sync,
	CMDhandler_tempnam,
	CMDhandler_tmpnam,
	CMDhandler_truncate,
	CMDhandler_ttyname,
	CMDhandler_unlink,
	CMDhandler_write,

#ifndef LINUX_PORTALS
	CMDhandler_hnode_cnct,
	CMDhandler_hnode_send,
	CMDhandler_hnode_recv,
#endif

#if defined (__i386__) & !defined (LINUX_PORTALS)
	    CMDhandler_eseek,
	    CMDhandler_esize,
	    CMDhandler_estat,
	    CMDhandler_festat,
	    CMDhandler_lestat,
	    CMDhandler_lsize,
	    CMDhandler_iread,
	    CMDhandler_ireadoff,
	    CMDhandler_readoff,
	    CMDhandler_iwrite,
	    CMDhandler_iwriteoff,
	    CMDhandler_writeoff,
	    CMDhandler_iseof,
	    CMDhandler_iodone,
	    CMDhandler_fstatpfs,
	    CMDhandler_statpfs,
#endif __i386__
            CMDhandler_heartbeat,
};

const char *CMDstrings[50] =
{	"CMDhandler_access",
	"CMDhandler_chdir",
	"CMDhandler_chmod",
	"CMDhandler_chown",
	"CMDhandler_close",
	"CMDhandler_reqclose", /* WMD */
	"CMDhandler_fcntl",
	"CMDhandler_fstat",
	"CMDhandler_fstatfs",
	"CMDhandler_fsync",
	"CMDhandler_ftruncate",
	"CMDhandler_getdirentries",
	"CMDhandler_gethostname",
#if 0
	"CMDhandler_getgid",
	"CMDhandler_getuid",
	"CMDhandler_getegid",
	"CMDhandler_geteuid",
	"CMDhandler_setgid",
	"CMDhandler_setuid",
	"CMDhandler_setegid",
	"CMDhandler_seteuid",
#endif
	"CMDhandler_iomode",
	"CMDhandler_link",
	"CMDhandler_lseek",
	"CMDhandler_lstat",
        "CMDhandler_mass_murder",
	"CMDhandler_mkdir",
	"CMDhandler_noop",
	"CMDhandler_open",
	"CMDhandler_read",
	"CMDhandler_rename",
	"CMDhandler_rmdir",
	"CMDhandler_setiomode",
	"CMDhandler_stat",
	"CMDhandler_statfs",
	"CMDhandler_symlink",
	"CMDhandler_sync",
	"CMDhandler_tempnam",
	"CMDhandler_tmpnam",
	"CMDhandler_truncate",
	"CMDhandler_ttyname",
	"CMDhandler_unlink",
	"CMDhandler_write"
};

