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
/* sfyod_support.h - Support routines for sfyod */

/*
 * TITLE(sfyod_support_h, "@(#) $Id: sfyod_support.h,v 1.10 2001/02/16 05:36:00 lafisk Exp $");
 */

#if !defined _INCsfyod_supporth_
#define _INCsfyod_supporth_

#include "defines.h"
#include "srvr_comm.h"

/*
 * The full path to files written with the striped file system will be
 * /sfyod[disk#]/sfs/filename.
 *
 * The full path to meta data will be
 * /sfyod[disk#]/sfs/filename$meta
 */
 
#define Kbytes (1024)
#define Mbytes (Kbytes*Kbytes)

/*#define SFYOD_STRIP_FACTOR 0x100000 /* 1M bytes */
#define SFYOD_STRIP_FACTOR (1*Mbytes) /* 1M bytes */

#define SFYOD_USER_PREFIX "/sfs/"
#define SFYOD_SEPARATE_PREFIX "/zfs/"
#define SFYOD_PREFIX "/var/sfyod#"
#define SFYOD_META_SUFFIX "$meta"
#define SFYOD_LOCKFILE "/tmp/sfyod#Lock"
#define SFYOD_UNITNUMFILE "/.unitNumber"
#define SFYOD_ENV_IOCLIENTS "SFYOD_IOCLIENTS"

#define SFYOD_FILEMODE 0444
#define SFYOD_MAXDISKS 10	/* 0 - 9 */
#define MAX_SFYOD_COUNT 20

#define SFYOD_OK 0
#define SFYOD_ERROR -1
#define SFYOD_LOOP_FOREVER -1
#define SFYOD_DELIMITER ':'
#define SFYOD_FREE_ENTRY 255
#define SFYOD_WILD 255

#define SFYOD_MAX_STRING 256
#define SFYOD_MAX_METAFILE 1024 /* Ridiculous size */

#define SFYOD_PORTAL 59
#define SFYOD_ACK_PORTAL 58
#define SFYOD_WRITE_PORTAL 57

#define TOASCII(num) (num|0x30)

/* sfyod callback function type */
typedef void (*callback)(void);


/*
This structure must not exceed (used to be 40) 128 bytes so 
that it can fit in the header.
It is very simalar to the hostCmd_t structure in rpc_msgs.h.
The first four fields are identical to hostCmd_t for now so that I can use
both the old routines and the new routines on the same messages.
The read_portal and write_portal fields are rarely used in this library.
*/
typedef struct {
	INT8	type;
	int     read_portal;
	int     write_portal;
	int     ack_portal;

/* sfyod additions */
	UINT8  sfyod_type;
	UINT8  unused;
	UINT16  available;

/* We are at a 32-bit word boundry now */

/* Each union can not exceed  32 bytes */
	union {
		struct {
			UINT16 unitNum;
			UINT16 hello_status;
		} hello;

		struct {
			UINT16 rank;
			UINT16 sfyod_nid;
			UINT16 sfyod_pid;
		} assign;

		struct {
			UINT32 count;
		} dump;

		struct {
			UINT16 sfyod_nid;
			UINT16 sfyod_pid;
			UINT16 sfyod_unitNum;
		} update;

	} types;
	
} sfyod_msg;

/* sfyod data structure */
typedef struct data_entry {
	UINT16 nid;
	UINT16 pid;
	UINT16 unitNum;
	UINT16 load; /* For future work */
} data_entry;

typedef struct sfyod_data {
	UINT32 current_count;
	UINT32 max_count;
	data_entry *entries[MAX_SFYOD_COUNT];
} sfyod_data;

typedef struct sfyod_dump {
	data_entry entries[MAX_SFYOD_COUNT];
} sfyod_dump;

/* meta data structure */
typedef struct sfyod_meta {
	UINT16 disks;			/* number of elements in */
					/* unitNumbers[] below */
	UINT16 computeNodes;		/* number of compute nodes */
					/* particapating in I/O */
	UINT32 stripFactor;
	ULONG fileSize;			/* bytes */
	UINT32 unitNumbers[MAX_SFYOD_COUNT];
} sfyod_meta;

/*
 * Since host_rpc command types are < 0, I will use the first positive
 * number for SFYOD message.
 */
#define SFYOD_MESSAGE 1

/* SFYOD Message types */
enum {
	SFYOD_MESSAGE_hello,		/* Tell master sfyod new sfyods */
	SFYOD_MESSAGE_assign,		/* Assign a sfyod to an app. */
	SFYOD_MESSAGE_dump,			/* Request master to dump data base */
	SFYOD_MESSAGE_updateAdd,	/* Tell slave about new sfyod */
	SFYOD_MESSAGE_updateDelete,	/* Tell slave about dead sfyod */
	SFYOD_MESSAGE_goodbye,		/* Tell master sfyod slave going away */
	SFYOD_MESSAGE_killSlaves,	/* Tell slaves to kill themselves */
	SFYOD_MESSAGE_END
};
#define SFYOD_MESSAGES SFYOD_MESSAGE_END

extern INT32 sfyod_disableVerbose(void);
extern char *sfyod_dirname(char *path);
extern INT32 sfyod_setConfigFile(char *filename);
extern INT32 sfyod_strsub(char *string, int slen, int old, int new);
extern INT32 sfyod_test_fname(char *fullPath);
extern INT32 sfyod_test_fnameSeparate(char *fullPath);
extern INT32 sfyod_read_configFile(UINT16 *nid, UINT16 *pid, UINT16 *portal);
extern INT32 sfyod_assign_nid(UINT16 *sfyod_nid, UINT16 *sfyod_pid,
	UINT32 master_nid, UINT32 master_pid, UINT32 rank);
extern INT32 sfyod_get_masterNidPid(UINT16 *master_nid, UINT16 *master_pid); 
extern INT32 sfyod_send_nidPid(UINT16 master_nid, UINT16 master_pid, 
	UINT16 nid, UINT16 pid, UINT16 unitNum);
extern INT32 sfyod_goodbye(UINT16 master_nid, UINT16 master_pid, 
	UINT16 nid, UINT16 pid, UINT16 unitNum);
extern PORTAL_INDEX init_serverPortal(PORTAL_INDEX requested,
	INT32 max_num_msgs);
extern PORTAL_INDEX init_ackPortal(void);
extern INT32 send_serverMessage(sfyod_msg *msg, sfyod_msg *ack,
	PORTAL_INDEX a_ptl, UINT16 srvr_nid, UINT16 srvr_pid,
	PORTAL_INDEX srvr_ptl);
extern INT32 send_clientMessage(sfyod_msg *msg, sfyod_msg *ack,
	PORTAL_INDEX a_ptl, UINT16 client_nid, UINT16 client_pid,
	PORTAL_INDEX client_ptl);
extern INT32 wait_serverAck(PORTAL_INDEX portal, INT32 timeout);
extern INT32 wait_clientAck(PORTAL_INDEX portal, INT32 timeout,
	callback func);
extern INT32 free_clientMessage(PORTAL_INDEX portal,
	control_msg_handle *handle);
extern INT32 get_clientMessage(PORTAL_INDEX ptl, control_msg_handle *handle,
	INT32 *msg_type, INT32 *xfer_len, CHAR **user_data);
extern INT32 send_portal(PORTAL_INDEX portal, NID_TYPE dst_nid,
	PID_TYPE dst_pid, CHAR *user_data, INT32 xfer_len);
extern INT32 sfyod_computeNodes(int sfyodCount, int sfyodRank, int nnodes);
extern INT32 sfyod_nidIndex(int sfyod_count, int rank, int nnodes);
extern INT32 sfyod_init_database(sfyod_data *data);
extern data_entry *sfyod_index_entry(sfyod_data *data, int index);
extern INT32 sfyod_find_index(sfyod_data *data, UINT16 nid, UINT16 pid,
	UINT16 unitNum);
extern data_entry *sfyod_find_entry(sfyod_data *data, UINT16 nid, UINT16 pid,
	UINT16 unitNum);
extern INT32 sfyod_insert_new(sfyod_data *data, data_entry *new);
extern data_entry *sfyod_get_next(sfyod_data *data, int index);
extern INT32 sfyod_enter_data(sfyod_data *data, UINT16 nid, UINT16 pid,
	UINT16 unitNum);
extern INT32 sfyod_remove_data(sfyod_data *data, UINT16 nid, UINT16 pid,
	UINT16 unitNum);
extern INT32 sfyod_get_sfyodData(UINT16 nid, UINT16 pid, sfyod_data *data);
extern INT32 sfyod_dump_sfyodData(UINT16 src_nid, UINT16 src_pid,
	sfyod_msg *msg, sfyod_data *data);
extern INT32 sfyod_send_update(int operation, sfyod_data *data,
	UINT16 nid, UINT16 pid, UINT16 unitNum, UINT16 master_nid,
	UINT16 master_pid, callback func);
extern UINT16 sfyod_getNid(data_entry *current);
extern UINT16 sfyod_getPid(data_entry *current);
extern UINT16 sfyod_get_unitNum(data_entry *current);
extern data_entry *sfyod_find_unitNum(sfyod_data *data, UINT16 unitNum);
extern INT32 sfyod_getCount(sfyod_data *data);
extern UINT32 sfyod_getIndex(sfyod_data *data, UINT32 rank);
extern void sfyod_print_sfyodData(sfyod_data *data);
extern void sfyod_show_sfyodData(sfyod_data *data,
	int master_nid, int master_pid);
extern INT32 sfyod_read_unitNum(UINT16 *unitNum, char *diskPath);
extern INT32 sfyod_lockDisk(UINT16 *diskNum, UINT16 *unitNum);
extern INT32 sfyod_removeLock(int disk);
extern INT32 sfyod_assign_meta(UINT16 *unitNum, CHAR *fullpath,
	INT32 rank, INT32 nnodes);
extern INT32 sfyod_get_mysfyod(INT32 rank, INT32 count);
extern INT32 sfyod_killSlaves(sfyod_data *data, int master_nid,
	int master_pid, callback func);
extern INT32 sfyod_open_meta(char *fullpath, INT32 flags, INT32 mode);
extern INT32 sfyod_write_meta(INT32 fd, sfyod_meta *meta);
extern INT32 sfyod_read_meta(INT32 fd, sfyod_meta *meta);

#endif /* _INCsfyod_supporth_ */
