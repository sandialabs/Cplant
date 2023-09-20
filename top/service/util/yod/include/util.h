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
/* $Id: util.h,v 1.14 2001/11/17 01:19:23 lafisk Exp $ */

#ifndef US_H
#define US_H

#include "srvr_comm.h"
#include "rpc_msgs.h"
#include "puma_errno.h"
#include "yod_data.h"

#define MAX_STRING_LEN  (1024)
#define MAXNAMLEN	(255)
#define MAX_ARGS        (256)

/*
** Function Prototypes
*/
void usage(char *pname);



#ifndef LINUX_PORTALS

/*
** Even after including the appropriate OSF header files, there are
** still functions undefined. Here they are to satisfy gcc -Wall
*/
extern int gethostname(char *address, int address_len);
extern void bcopy(char *source, char *destination, int length);
extern setreuid(int ruid, int euid);
extern setregid(int rgid, int egid);
extern void sync(void);
extern int fsync (int filedes);

#endif /* LINUX_PORTALS */

/*
** Debug output of .service partition side programs. It used to be that the
** higher the value of Dbgflag was, the more output you got. To be more
** selective, we dedicate individual bits to particular debug messages
**
** If DBG_PHASE_1 is set, other messages, such as DBG_MSG_1, will be
** printed during the load. If DBG_PHASE_2 is set, then they will also
** be printed after the load. If neither is set, messages will always
** be printed, independent of the load status.
*/
extern INT32 Dbgflag;
extern INT32 prog_phase;

#if 0
/* "legacy" code - Rolf's debugging scheme for Puma's yod */
#define DBG_PHASE_1	(0x00000001)	/* Output during load */
#define DBG_PHASE_2	(0x00000002)	/* Output after load */
#define DBG_LOAD_1	(0x00000004)	/* Loading, level 1 */
#define DBG_LOAD_2	(0x00000008)	/* Loading, level 2 */
#define DBG_LOAD_3	(0x00000010)	/* Loading, level 3 */
#define DBG_MSG_1	(0x00000020)	/* Selected Messages */
#define DBG_MSG_2	(0x00000040)	/* All messages */
#define DBG_CONFIG	(0x00000080)	/* Accessing the config file */
#define DBG_PROGRESS	(0x00000100)	/* Report on program progress */
#define DBG_HETERO	(0x00000200)	/* Heterogeneous load */
#define DBG_ALLOC_1	(0x00000400)	/* Mesh allocation, level 1 */
#define DBG_ALLOC_2	(0x00000800)	/* Mesh allocation, level 2 */
#define DBG_IO_1	(0x00001000)	/* usmsghandler, level 1 */
#define DBG_IO_2	(0x00002000)	/* usmsghandler, level 2 */
#define DBG_NQS		(0x00004000)	/* NQS messages */
#define DBG_DBG		(0x00008000)	/* Output during debug */
#define DBG_KILL	(0x00010000)	/* Kill faulting process(es) */
#define DBG_IGNORE	(0x00020000)	/* Ignore faulting process(es) */
#define DBG_NODUMP	(0x00040000)	/* Ignore faulting process(es) */
#define DBG_HLIB	(0x00080000)    /* host node lib interactions */
#define DBG_FYOD	(0x00100000)	/* fyod startup */
#define DBG_TIMING	(0x00200000)    /* Display timing information */

#else

#define DBG_PHASE_1	(0x00000001)	/* Output during load */
#define DBG_PHASE_2	(0x00000002)	/* Output after load */
#define DBG_IO_1	(0x00000004)	/* detailed handling of app IO requests */
#define DBG_IO_2	(0x00000008)	/* just list application IO request */
#define DBG_LOAD_1	(0x00000010)	/* Loading, lower level 1 */
#define DBG_LOAD_2	(0x00000020)	/* Loading, higher level 2 */
#define DBG_PROGRESS	(0x00000040)	/* display app progress from load to done */
#define DBG_FAILURE     (0x00000080)	/* display load failure detail */
#define DBG_DBG		(0x00000100)	/* application debugging */
#define DBG_ALLOC	(0x00000200)	/* node allocation details */
#define DBG_HETERO	(0x00000400)	/* Heterogeneous load */
#define DBG_BEBOPD      (0x00000800)	/* bebopd communications */
#define DBG_COMM        (0x00001000)	/* communications structures */
#define DBG_PBS         (0x00002000)	/* PBS info */
#define DBG_FORRTL      (0x00004000)	/* Display app Fortral RTL messages */
#define DBG_ENVIRON     (0x00008000)    /* Display yod's environment */
#define DBG_MEMORY      (0x00010000)    /* display all memory allocated */
#define DBG_JOBS        (0x00020000)    /* job spawn/synchronize info   */

#endif
/*
** Use the following macro to determine whether to print a debug
** statement or not:
**
**     if (DBG_FLAGS(DBG_LOAD_1))
**         fprintf(stderr, "Debug info ...
**
** The macro automatically checks the program phase and the corresponding
** bits in Dbgflag to determine whether to actually print the message or not.
*/
#define DBG_FLAGS(x)							\
    ((Dbgflag & (x)) &&							\
	(((Dbgflag & DBG_PHASE_1) && (prog_phase == 1)) ||		\
	 ((Dbgflag & DBG_PHASE_2) && (prog_phase == 2)) ||		\
	 ((Dbgflag & (DBG_PHASE_1 | DBG_PHASE_2)) == 0)	||		\
	 (prog_phase == 0)						\
	)								\
    )

extern int ioErrno;

#define CMD_SET_VALS(snid,spid,cmd,buf,len,mh) \
    snid    = SRVR_HANDLE_NID(*mh);                     \
    spid    = SRVR_HANDLE_PID(*mh);                     \
    cmd     = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);    \
    buf      = get_work_buf();                          \
    len = SRVR_HANDLE_TRANSFER_LEN(*mh);

extern int ioErrno;

int initialize_work_bufs();
void takedown_work_bufs();
void free_work_buf(int buf);
int get_work_buf();
char *workBufData(int buf);
int send_workbuf(control_msg_handle *mh, int bufnum, int len);
int check_workbuf_to_app(int slot);
void cancel_workbuf_to_app(int slot);
int send_workbuf_and_ack(control_msg_handle *mh,
                    int bufnum, int len, hostReply_t *ack);
int send_workbuf_to_app(control_msg_handle *mh, int buf, int len);
int send_ack_to_app(control_msg_handle *mh, hostReply_t *ack);


#endif /* US_H */
