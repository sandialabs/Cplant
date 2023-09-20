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
** $Id: puma.h,v 1.23 2001/11/17 01:11:42 lafisk Exp $
*/

#ifndef CLPUMA_H
#define CLPUMA_H

#include "idtypes.h"
#include "defines.h"
#include "cTask/cTask.h"

TITLE(puma_h, "@(#) $Id: puma.h,v 1.23 2001/11/17 01:11:42 lafisk Exp $");

/******************************************************************************/
ppid_type register_ppid(taskInfo_t *info, ppid_type ppid, gid_type gid,
                        char *name);

extern taskInfo_t _my_taskInfo;

extern UINT16 _yod_cmd_nid;
extern UINT16 _yod_cmd_pid;
extern int    _yod_cmd_ptl;

extern int    _my_dna_ptl;

extern int _my_PBS_ID;
extern ppid_type _my_ppid;
extern spid_type _my_pid;
extern  gid_type _my_gid;

extern nid_type *_my_nid_map;
extern ppid_type *_my_pid_map;

extern UINT32 _my_pidnid;
extern UINT32 _my_gidrank;
extern UINT32 _my_umask;
extern nid_type _my_rank;
extern nid_type _my_pnid;
extern nid_type _my_nnodes;

extern INT32 ___startup_complete;
extern INT32 ___proc_type;

extern int  _my_PBS_ID;
extern int  _my_parent_handle;
extern int  _my_own_handle;

extern int __p30_initialized;

#define APP_TYPE  1
#define SERV_TYPE 2

#if !defined(OSF_PORTALS) 
extern int _yod_io_data_ptl;
extern int _yod_io_ctl_ptl;
extern CHAR _CLcwd[];
#endif /* OSF_PORTALS */

int init_dyn_alloc(void);
int end_dyn_alloc(void);

int replace_string(CHAR *source, CHAR **mark, long number);
double dclock(void);
int pack_string(char **ustr, int maxu, char *pstr, int maxp);
char *convert_pound(const char *name, int node_no);

typedef struct {
    nid_type  nid;
    ppid_type pid;
} NIDPID;

/******************************************************************************/

#define MAKE_FULL_PATH(full_path, path, err_value)                \
{                                                                \
                if (path[0] != '/')   {                                \
                    if (((strlen(_CLcwd) + strlen(path)) >         \
                                    (MAXPATHLEN - 2)))   {        \
                        return (err_value);                        \
                    }                                                \
                    sprintf(full_path, "%s/%s", _CLcwd, path);        \
                } else   {                                        \
                    if (strlen(path) >= MAXPATHLEN)   {                \
                        return (err_value);                        \
                    } else   {                                        \
                        strcpy(full_path, path);                \
                    }                                                \
                }                                                \
}

void out_of_band_pct_message(int type, 
                        int int1, int int2, int int3, int int4,
                        void *ptr1, void *ptr2);


#define PCT_DUMP(t,i1,i2,i3,i4,p1,p2) \
    out_of_band_pct_message((int)t, (int)i1, (int)i2, (int)i3, (int)i4,  \
                            (void *)p1, (void *)p2)




#endif /* CLPUMA_H */
