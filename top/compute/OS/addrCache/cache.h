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
/* top/compute/OS/addrCache/cache.h */

#ifndef _ADDR_CACHE_H_
#define _ADDR_CACHE_H_

#include <portals/ppid.h>
#include <cache/cache.h>

typedef struct addr_key_t        addr_key_t;
typedef struct addr_cache_t      addr_cache_t;
typedef struct addr_tbl_t        addr_tbl_t;

/* number of different regions sizes -- used by lib_init 
   and lib_fini -- moved to top/include/cache/cache.h */
//#define NUM_SUBCACHES 3

/* size regions we validate */ 
#define ADDR_CACHE_REG_SZ0  PAGE_SIZE
#define ADDR_CACHE_REG_SZ1  (120 * 1024)
#define ADDR_CACHE_REG_SZ2  (4088 * 1024)

/* number of regions we cache for size_i */
#define ADDR_CACHE_NUM_REG0 4096 
#define ADDR_CACHE_NUM_REG1 1024
#define ADDR_CACHE_NUM_REG2 256

/* number of slots required in translation 
   table for region of size_i -- depends on PAGE_SIZE:
   assuming the region size is a multiple K of
   PAGE_SIZE, it's K+1 (i.e., 1 page may be split between
   the start and end of the region...) */
#define ADDR_CACHE_TBL_SZ0 2
#ifdef __i386__
#define ADDR_CACHE_TBL_SZ1 32
#define ADDR_CACHE_TBL_SZ2 1024
#endif
#ifdef __alpha__
#define ADDR_CACHE_TBL_SZ1 16
#define ADDR_CACHE_TBL_SZ2 512
#endif
#ifdef __ia64__
#define ADDR_CACHE_TBL_SZ1 8
#define ADDR_CACHE_TBL_SZ2 256
#endif

void addrCache_tblInit(int ppid);
void addrCache_tblClear(int ppid);
int  addrCache_tblUnlink(int cache_id);
void addrCache_tblLink(void *addrkey, int ppid);
void addrCache_display(int ppid);
void addrCache_invalidate(void);
extern void addrCache_reg_fn( int (*sender)(addr_summary_t* addrSum));

int memcpy3_to_user( void* vaddr, void* from, int nbytes, void *addrkey,
                                                struct task_struct *task );
int memcpy3_from_user( void* to, void* vaddr, int nbytes, void *addrkey,
                                                struct task_struct *task );
int addrCache_populate( unsigned long vaddr, int nbytes, int ppid, 
                                                          void **addrkey);
int addrCache_revalidate( addr_tbl_t *addr_tbl, unsigned long vaddr, 
                                  int nbytes, struct task_struct *task );

/* this is where the internal version of the memory descriptor 
   keeps an index into the kernel address cache for its region 
                                                     of memory */
struct addr_key_t {
    int cache_id;
    int table_id;
};

/* will have a set of these for each region size */
struct addr_tbl_t {
  unsigned long start;
  int length;
  int chunk0;
  int chunkn;
  int nchunks;
  int valid;
  int slot;             /* beginning of my set of slots in addrSlot[see cache.c] */
  addr_key_t addr_key;
  addr_tbl_t *next;
  addr_tbl_t *prev;
} ;

typedef enum { TBL_LOOKUP, TBL_REDO } addr_cache_strategy_t; 

/* one instance for each region size -- it points into the
   above set of tables */
struct addr_cache_t {
  int reg_sz;

  int num_reg;

  int num_reg_used;

  int freeTables;

  int tbl_sz;

  addr_cache_strategy_t addr_cache_strategy;

  addr_tbl_t *addr_tbl;       /* the set of available address tables in
                                 this cache */
  addr_tbl_t *taskTbl[NUM_PTL_TASKS];
                              /* the current validations wrt this subcache
                                 for each active portals process */
} ;


//extern addr_cache_t addrCache[];
//extern unsigned long addrSlot[]; 
#endif
