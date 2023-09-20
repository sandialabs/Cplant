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
/* $Id: CMDhandlerTable.h,v 1.6 2001/02/16 05:44:34 lafisk Exp $ */

   
#ifndef CMDHANDLERTABLE_H
#define CMDHANDLERTABLE_H

#include <sys/param.h>
#include "defines.h"
#include "srvr_comm.h"

extern VOID (*host_cmd_handler_tbl[])(control_msg_handle *mh);

VOID CMDhandler_access(control_msg_handle *mh);
VOID CMDhandler_chdir(control_msg_handle *mh);
VOID CMDhandler_chmod(control_msg_handle *mh);
VOID CMDhandler_chown(control_msg_handle *mh);
VOID CMDhandler_close(control_msg_handle *mh);
VOID CMDhandler_echo(control_msg_handle *mh);
VOID CMDhandler_exit(control_msg_handle *mh);
VOID CMDhandler_fcntl(control_msg_handle *mh);
VOID CMDhandler_fstat(control_msg_handle *mh);
VOID CMDhandler_fsync(control_msg_handle *mh);
VOID CMDhandler_getgid(control_msg_handle *mh);
VOID CMDhandler_getuid(control_msg_handle *mh);
VOID CMDhandler_iomode(control_msg_handle *mh);
VOID CMDhandler_link(control_msg_handle *mh);
VOID CMDhandler_lseek(control_msg_handle *mh);
VOID CMDhandler_mass_murder(control_msg_handle *mh);
VOID CMDhandler_mkdir(control_msg_handle *mh);
VOID CMDhandler_noop(control_msg_handle *mh);
VOID CMDhandler_open(control_msg_handle *mh);
VOID CMDhandler_read(control_msg_handle *mh);
VOID CMDhandler_rename(control_msg_handle *mh);
VOID CMDhandler_rmdir(control_msg_handle *mh);
VOID CMDhandler_setiomode(control_msg_handle *mh);
VOID CMDhandler_stat(control_msg_handle *mh);
VOID CMDhandler_sync(control_msg_handle *mh);
VOID CMDhandler_tempnam(control_msg_handle *mh);
VOID CMDhandler_tmpnam(control_msg_handle *mh);
VOID CMDhandler_ttyname(control_msg_handle *mh);
VOID CMDhandler_unlink(control_msg_handle *mh);
VOID CMDhandler_write(control_msg_handle *mh);
VOID CMDhandler_hnode_cnct(control_msg_handle *mh);
VOID CMDhandler_hnode_send(control_msg_handle *mh);
VOID CMDhandler_hnode_recv(control_msg_handle *mh);

VOID CMDhandler_access(control_msg_handle *mh);
VOID CMDhandler_chdir(control_msg_handle *mh);
VOID CMDhandler_chmod(control_msg_handle *mh);
VOID CMDhandler_chown(control_msg_handle *mh);
VOID CMDhandler_close(control_msg_handle *mh);
VOID CMDhandler_echo(control_msg_handle *mh);
VOID CMDhandler_exit(control_msg_handle *mh);
VOID CMDhandler_fcntl(control_msg_handle *mh);
VOID CMDhandler_fstat(control_msg_handle *mh);
VOID CMDhandler_fsync(control_msg_handle *mh);
VOID CMDhandler_ftruncate(control_msg_handle *mh);
VOID CMDhandler_getdirentries(control_msg_handle *mh);
VOID CMDhandler_gethostname(control_msg_handle *mh);
VOID CMDhandler_getgid(control_msg_handle *mh);
VOID CMDhandler_getuid(control_msg_handle *mh);
VOID CMDhandler_getegid(control_msg_handle *mh);
VOID CMDhandler_geteuid(control_msg_handle *mh);
VOID CMDhandler_setgid(control_msg_handle *mh);
VOID CMDhandler_setuid(control_msg_handle *mh);
VOID CMDhandler_setegid(control_msg_handle *mh);
VOID CMDhandler_seteuid(control_msg_handle *mh);
VOID CMDhandler_iomode(control_msg_handle *mh);
VOID CMDhandler_link(control_msg_handle *mh);
VOID CMDhandler_lseek(control_msg_handle *mh);
VOID CMDhandler_lstat(control_msg_handle *mh);
VOID CMDhandler_mass_murder(control_msg_handle *mh);
VOID CMDhandler_mkdir(control_msg_handle *mh);
VOID CMDhandler_noop(control_msg_handle *mh);
VOID CMDhandler_open(control_msg_handle *mh);
VOID CMDhandler_read(control_msg_handle *mh);
VOID CMDhandler_rename(control_msg_handle *mh);
VOID CMDhandler_rmdir(control_msg_handle *mh);
VOID CMDhandler_setiomode(control_msg_handle *mh);
VOID CMDhandler_stat(control_msg_handle *mh);
VOID CMDhandler_symlink(control_msg_handle *mh);
VOID CMDhandler_sync(control_msg_handle *mh);
VOID CMDhandler_tempnam(control_msg_handle *mh);
VOID CMDhandler_tmpnam(control_msg_handle *mh);
VOID CMDhandler_truncate(control_msg_handle *mh);
VOID CMDhandler_ttyname(control_msg_handle *mh);
VOID CMDhandler_unlink(control_msg_handle *mh);
VOID CMDhandler_write(control_msg_handle *mh);
VOID CMDhandler_statfs(control_msg_handle *mh);
VOID CMDhandler_fstatfs(control_msg_handle *mh);
#if defined (__i386__)
VOID CMDhandler_eseek(control_msg_handle *mh);
VOID CMDhandler_esize(control_msg_handle *mh);
VOID CMDhandler_estat(control_msg_handle *mh);
VOID CMDhandler_festat(control_msg_handle *mh);
VOID CMDhandler_lestat(control_msg_handle *mh);
VOID CMDhandler_lsize(control_msg_handle *mh);
VOID CMDhandler_iread(control_msg_handle *mh);
VOID CMDhandler_ireadoff(control_msg_handle *mh);
VOID CMDhandler_readoff(control_msg_handle *mh);
VOID CMDhandler_iwrite(control_msg_handle *mh);
VOID CMDhandler_iwriteoff(control_msg_handle *mh);
VOID CMDhandler_writeoff(control_msg_handle *mh);
VOID CMDhandler_iseof(control_msg_handle *mh);
VOID CMDhandler_iodone(control_msg_handle *mh);
VOID CMDhandler_fstatpfs(control_msg_handle *mh);
VOID CMDhandler_statpfs(control_msg_handle *mh);
#endif /* __i386__ */
VOID CMDhandler_heartbeat(control_msg_handle *mh);

extern const char *CMDstrings[];


#endif /* CMDHANDLERTABLE_H */
