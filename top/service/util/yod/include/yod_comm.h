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
** $Id: yod_comm.h,v 1.4 2001/09/26 07:05:48 lafisk Exp $
*/
#include "rpc_msgs.h"

INT32 initialize_yod(INT32 listsize, char *nodes, 
	   int session_id, int session_limit, int *pctlist, uid_t euid);
VOID takedown_yod_portals(void);
VOID display_pct_list(void);
INT32 send_pcts_control_message(int msg_type, char *buf, int len, 
	volatile int *count);
INT32 send_pcts_put_message(INT32 msg_type, CHAR *user_data, INT32 user_data_len,
            CHAR *put_data, INT32 put_data_len, INT32 tmout,
            BOOLEAN rootPctOnly, INT32 member);
char *get_stack_trace(int pctRank, int bt_size, int job_id, int tmout);
INT32 send_root_pct_get_message(INT32 msg_type, CHAR *user_data, 
      INT32 user_data_len, CHAR *get_data, INT32 get_data_len, INT32 tmout);

INT32 wait_pcts_put_msg(INT32 bufnum, INT32 tmout, INT32 howmany);
INT32 get_pct_control_message(INT32 *mtype, CHAR **user_data, INT32 *rank);
int all_get_pct_control_message(int *mtypes, char *udataBufs, 
                                  int udataBufLen, int timeout);
INT32 await_pct_msg(INT32 *mtype, CHAR **user_data, INT32 *rank, INT32 tmout);
int read_executable(int member, int timing_data);
int check_link_version(int member);
int send_executable(int member, int job_ID);
INT32 get_app_control_message(control_msg_handle *mh);
void free_app_control_message(control_msg_handle *mh);
INT32 get_next_application_msg(hostCmd_t *msg, int tmout);
int get_app_srvr_portal(void);
int get_pct_portal(void);
INT32 fyod_read_configFile(int *nid);
int send_to_bebopd(char *buf, int len, int sendtype, int gettype);

int start_notify(const char *what);
int notify(char *line);
int end_notify(void);

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

#define BEBOPD_PORTAL        2		/* Fixed portal */

#define PUT_ROOT_PCT_ONLY    1
#define PUT_ALL_PCTS         0
