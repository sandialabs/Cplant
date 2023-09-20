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
** $Id: rpc_msgs.h,v 1.21 2001/11/24 23:32:45 lafisk Exp $
*/

#ifndef RPC_MSGS_H
#define RPC_MSGS_H

#include <sys/types.h>
#include "defines.h"

#define YO_ITS_IO            0x00111100
#define IO_REPLY             0x00222200
#define IO_REPLY_DONE        0x00444400
#define JOB_MANAGEMENT       0x00888800
#define JOB_MANAGEMENT_ACK   IO_REPLY_DONE
#define INTRA_JOB_BARRIER    0x00aaaa00

/******************************************************************************/
/*                Job management messages                                     */
/******************************************************************************/

enum {
    CMD_NO_COMMAND,
    CMD_JOB_CREATE,
    CMD_JOB_STATUS,
    CMD_SYNCHRONIZE_JOBS,
    CMD_SYNCHRONIZE_STATUS,
    CMD_JOB_NID_MAP,
    CMD_JOB_PID_MAP,
    CMD_JOB_SIGNAL,
    CMD_JOB_TERMINATION_STATUS,
    CMD_NODES_REMAINING
    };

#define FIRST_JOB_CMD_NUM       (CMD_JOB_CREATE)
#define LAST_JOB_CMD_NUM        (CMD_NODES_REMAINING)

typedef struct   {
    int sigNum;
    int yodHandle;
} jobRequestCmd_t; 

typedef  struct   {
    int job_id;
    int yodHandle;
    int progress; /* bit map JOB_* values found in config/cplant.h */
    int nprocs;
    int retVal;
} jobStatusAck_t;

typedef  struct   {
    int status;      /* 1 (in progress), 2 (complete), -1 (failure) */
} jobSynchAck_t;



/******************************************************************************/
/*                File I/O messages                                           */
/******************************************************************************/

enum {
        CMD_ACCESS = -0x50, 
        CMD_CHDIR, 
        CMD_CHMOD,
        CMD_CHOWN,
        CMD_CLOSE,
	CMD_REQCLOSE, /* WMD */
        CMD_FCNTL,
        CMD_FSTAT,
        CMD_FSTATFS,
        CMD_FSYNC,
        CMD_FTRUNCATE,
        CMD_GETDIRENTRIES,
        CMD_GETHOSTNAME,
        CMD_IOMODE,
        CMD_LINK,
        CMD_LSEEK,
        CMD_LSTAT,
        CMD_MASS_MURDER,
        CMD_MKDIR,
        CMD_NOOP,
        CMD_OPEN,
        CMD_READ,
        CMD_RENAME,
        CMD_RMDIR,                /* -0x30 */
        CMD_SETIOMODE,
        CMD_STAT,
        CMD_STATFS,
        CMD_SYMLINK,
        CMD_SYNC,
        CMD_TEMPNAM,
        CMD_TMPNAM,
        CMD_TRUNCATE,                /* -0x28 */
        CMD_TTYNAME,
        CMD_UNLINK,
        CMD_WRITE,
	CMD_HEARTBEAT
    };

#define CMD_NONE        (-0x00)
#define FIRST_CMD_NUM        (CMD_ACCESS)

#define LAST_CMD_NUM        (CMD_HEARTBEAT)


/*
** Offsets on the Pentium Pro are 64 bits, while they are 32 bits on the i860
*/
#if defined (__i386__)
#define BIGGEST_OFF        off64_t
#else
#define BIGGEST_OFF        off_t
#endif /* __i386__ */


/*
** The following is a union of all possible commands that can be
** sent to yod. Yod will then get the actual data through a readmem
** or deposit data through a writemem. We'll try to keep the command
** size to 28 bytes, so it fits into the header.
**
** In transit, this structure is overlayed over the Puma header
** (PUMA_MSG_HEAD), so that it is flush against the end of the header.
** Therefore, the last 44 bytes of a Puma header correspond to a
** hostCmd_t structure for most messages between Puma and OSF.
**
** There are pieces of code in the kernel that work under this
** assumption. However, we don't want the kernel be dependent on
** files at the user level. This means, that a change in this
** structure (size and layout of the first four entries), cannot
** be changed without also changing the kernel! Look in OSmsg.c and
** hwsend.c for further comments.
**
 * Since there are 8-byte integers allocated with the alpha compiler, each
 * structure in the union is placed on a 8-byte boundry.  If there is at
 * least one 8-byte integer somewhere in this structure, all structures are
 * started on a 8-byte boundry even if the structure doesn't have a a 8-byte
 * integer or if the structure appears to place the 8-byte integer at the
 * correct boundry.  This compiler behavior will force us to lose 4-bytes
 * immediately after 'ack_portal'.  Also, if a structure in the union
 * doesn't have the 8-byte integer at a 8-byte boundry realative to
 * the structure, another 4-bytes will be lost.
*/

typedef struct   {
    /*
    ** Currently must be SRVR_USR_DATA_LEN bytes to fit in a control message.  
    ** The first 21 bytes are common to all commands.
    */
    INT8                type;           /* type of cmd (open, close, etc.) */
    int                 ctl_ptl;  /* send PUT requests and/or ACKs here */
    int                 my_seq_no;

    INT8                fail;           /* flag failure of cmd to handler */
    INT8                unitNumber;     /* fyod-specific -- should be large 
                                           enough to to hold the maximum number
                                           of fyod raids in the system 
                                           (currently 200)
                                        */
    INT32               uid;
    INT32               gid;
    INT32               euid;
    INT32               egid;

    union   {

        jobRequestCmd_t jobCmd;

        struct   {
            INT32        len;
            INT32        test;
        } echoCmd;

        struct   {
            INT32        mode;
            INT32        flags;
            UINT32       fnameLen;
        } openCmd;

        struct   {
            off64_t      hostFileIndex;
            UINT16       snid;
            UINT16       spid;
    	    UINT16       rank;
        } closeCmd;

        struct   {
            off64_t     hostFileIndex;
            BIGGEST_OFF        curPos;
            INT32        nbytes;
            INT32        fileStatusFlags;
    	    UINT16       rank;
    	    UINT16       nnodes;
        } readCmd, writeCmd, ireadCmd, iwriteCmd;
      
        struct   {
            INT32        nbytes;
            off64_t     hostFileIndex;
            INT32        fileStatusFlags;
            BIGGEST_OFF        offset; 
			UINT16       rank;
			UINT16       nnodes;
        } readoffCmd, writeoffCmd, ireadoffCmd, iwriteoffCmd;

        struct   {
            INT32        id;
        } iodoneCmd;

        struct   {
            off64_t     hostFileIndex;
            INT32        request;
            INT32        arg;        
        } fcntlCmd;

        struct   {
            off64_t     hostFileIndex;
        } fsyncCmd, fstatCmd, ttynameCmd, iomodeCmd, fstatfsCmd, festatCmd;

        struct   {
            off64_t     hostFileIndex;
            UINT32        bufSize;
        } fstatpfsCmd;

        struct   {
            UINT32        fnameLen;
            INT32        mode;
        } mkdirCmd, chmodCmd;

        struct   {
            UINT32        fnameLen;
            long        length;
        } truncateCmd; 

        struct   {
            off64_t     hostFileIndex;
            INT32        offset;
            INT32        whence;
            BIGGEST_OFF        curPos; 
	    UINT16       rank;
	    UINT16       nnodes;
        } lseekCmd;

        struct   {
            off64_t     hostFileIndex;
            INT32        iomode;
        } setiomodeCmd;

        struct   {
            UINT32        fnameLen;
            INT32        amode;
        } accessCmd;

        struct   {
            UINT32        fnameLen;
            INT32        owner;
            INT32        group;
        } chownCmd;

        struct   {
            INT32        directoryLen;
            char        prefix[6];
        } tempnamCmd;

        struct   {
            INT32        s_errno;
        } strerrorCmd;

        struct   {
            UINT32        fnameLen1;
            UINT32        fnameLen2;
        } renameCmd, linkCmd; 

        struct   {
            UINT32        fnameLen;
        } unlinkCmd, statCmd, chdirCmd, rmdirCmd, lstatCmd, statfsCmd,
                  estatCmd, lestatCmd;

        struct   {
            UINT32        fnameLen;
            UINT32        bufSize;
        } statpfsCmd;

       struct   {
            off64_t     hostFileIndex;
            INT32        whence;
            BIGGEST_OFF        offset; 
            BIGGEST_OFF        curPos;
        } eseekCmd, esizeCmd;

       struct   {
            BIGGEST_OFF        offset; 
            off64_t     hostFileIndex;
            INT32        whence;
            BIGGEST_OFF        curPos;
        } lsizeCmd;

        struct   {
            off64_t     hostFileIndex;
            BIGGEST_OFF        curPos;
        } iseofCmd;


        struct   {
            INT32        id;
        } setidCmd;

        struct   {
            UINT32        hnameLen;
        } gethnameCmd;

        struct   {
            off64_t     hostFileIndex;
            size_t       nbytes;
	    off_t        basep;
        } getdirentriesCmd;

        struct   {
            INT32        status;
        } exitCmd; 

        struct   {
            INT32        msg_len;
            INT32        msg_type;
            UINT16        acl_index;
        } hnode_send, hnode_recv;

        struct   {
            off64_t     hostFileIndex;
            long        length;
        } ftruncateCmd; 
    } info;

} hostCmd_t;


typedef        struct   {
    INT32                isattyFlag;        
    BIGGEST_OFF                curPos;

    /* server for subsequent requests */
    UINT16                srvrNid;
    UINT16                srvrPid;
    int                   srvrPtl;
} openAck_t;


typedef struct   {
    union   {

       jobStatusAck_t    jobStatusAck;

       jobSynchAck_t     jobSynchAck;

        struct   {
            INT32        work_buf_size;
            INT32        packet_size;
            INT32        pad0;
            INT32        pad1;
        } echoAck;

        openAck_t        openAck;

        struct   {
            BIGGEST_OFF        curPos64;
        } eseekAck, esizeAck, lsizeAck, readoffAck, writeoffAck;
      
        struct   {
            BIGGEST_OFF        curPos;
        } readAck, writeAck, iodoneAck, iowaitAck;
      
        struct   {
            long        ioid;
            BIGGEST_OFF        curPos;
        } ireadAck, ireadoffAck, iwriteAck, iwriteoffAck;
      
        struct   {
            INT32        d0;
            INT32        d1;
            INT32        d2;
            INT32        d3;
            INT32        d4;
            INT32        d5;
            INT32        d6;
            INT32        d7;
        } dummy;

        struct   {
            INT32        u0;
            INT32        u1;
            INT32        u2;
            INT32        u3;
            INT32        u4;
        } hbAck;

        struct   {
            INT32        msg_len;
            INT32        msg_type;
        } hnodeAck;

        struct   {
            NID_TYPE        nid;
            PID_TYPE        pid;
            INT32        slot;
        } hcnctAck;

        struct {
            off_t     basep;
        } getdirentriesAck;
    } info;

#ifdef ANYONE_KNOWS_WHAT_THIS_IS_FOR

    struct   {
        INT32        d0;
        INT32        d1;
        INT32        d2;
        INT32        d3;
        INT32        d4;
        INT32        d5;
        INT32        d6;
        INT32        d7;
    } dummy;
#endif

    INT32        hostErrno;
    off64_t     retVal;

    int    your_seq_no;
} hostReply_t;

/*
** we never use more than one buffer at a time
**
#define IOBUFSIZE    (8*1024)
#define NIOBUFS       4
*/
#define IOBUFSIZE    (4*8*1024)
#define NIOBUFS       1

#define NIOBLOCKS(n)   ((int)((n) / IOBUFSIZE) + (n%IOBUFSIZE ? 1 : 0))

#define YOD_NID         (_yod_cmd_nid)
#define YOD_PID         (_yod_cmd_pid)
#define YOD_PTL         (_yod_cmd_ptl)


INT32 _host_rpc(hostCmd_t *cmd, hostReply_t *ack,
    INT8 cmd_type,
    int srvr_nid, int srvr_pid, int srvr_ptl,
    const CHAR *r_buf, INT32 r_len, CHAR *w_buf, INT32 w_len);


#endif
