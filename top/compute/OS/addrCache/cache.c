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
#define EXPORT_SYMTAB

#define USE_PROC 0

#include <asm/uaccess.h>

#include <linux/module.h>
#include <linux/kernel.h>

#if USE_PROC
#include <linux/proc_fs.h>
#endif

#include <asm/page.h>
#include <portals/base/calcPhysAddress.h>
#include <portals/ppid.h>
#include <cTask/cTask.h>

#include <compute/OS/addrCache/cache.h>

#if USE_PROC
extern struct proc_dir_entry* proc_cplant;
#endif

EXPORT_SYMBOL(memcpy3_to_user);
EXPORT_SYMBOL(memcpy3_from_user);
EXPORT_SYMBOL(addrCache_populate);
EXPORT_SYMBOL(addrCache_tblInit);
EXPORT_SYMBOL(addrCache_tblClear);
EXPORT_SYMBOL(addrCache_tblLink);
EXPORT_SYMBOL(addrCache_tblUnlink);
EXPORT_SYMBOL(addrCache_reg_fn);

static int (*send_fn)(addr_summary_t* addrSummary);

#define USE_ADDR_LIST 1
#define CACHE_MAJOR 64

#if USE_ADDR_LIST
#define SHORT_STRATEGY TBL_LOOKUP
static addr_entry_t addrlist[ADDR_LIMIT];
static int num_addresses =0;
static addr_summary_t addrSummary;
static int rank=-1;
#else
#define SHORT_STRATEGY TBL_REDO
#endif

#if USE_PROC
int addrCache_read_proc_indirect(char* buf, char** start, off_t offset,
                        int len, int unused );
int addrCache_read_proc(char* buf, char** start, off_t offset,
                        int count, int *eof, void *data );
#endif
int addrCache_ioctl( struct inode *inodeP, struct file *fileP,
                        unsigned int cmd, unsigned long arg);

/* an array of address tables for each region size */
addr_tbl_t addrTbl0[ADDR_CACHE_NUM_REG0];
addr_tbl_t addrTbl1[ADDR_CACHE_NUM_REG1];
addr_tbl_t addrTbl2[ADDR_CACHE_NUM_REG2];

/* an array of pointers to the address tables (useful
   for performing global table operations) */
addr_tbl_t *addrTblPtrs[NUM_SUBCACHES] = { addrTbl0, 
                                           addrTbl1,
                                           addrTbl2 };

/* an array of table slots for all regions/tables */
unsigned long addrSlot[ ADDR_CACHE_TBL_SZ0 * ADDR_CACHE_NUM_REG0
                       +ADDR_CACHE_TBL_SZ1 * ADDR_CACHE_NUM_REG1 
                       +ADDR_CACHE_TBL_SZ2 * ADDR_CACHE_NUM_REG2 ];

addr_cache_t addrCache[NUM_SUBCACHES] = 
{ 
  {ADDR_CACHE_REG_SZ0, ADDR_CACHE_NUM_REG0, 0, ADDR_CACHE_NUM_REG0, ADDR_CACHE_TBL_SZ0, TBL_REDO, NULL}, 
  {ADDR_CACHE_REG_SZ1, ADDR_CACHE_NUM_REG1, 0, ADDR_CACHE_NUM_REG1, ADDR_CACHE_TBL_SZ1, TBL_REDO, NULL}, 
  {ADDR_CACHE_REG_SZ2, ADDR_CACHE_NUM_REG2, 0, ADDR_CACHE_NUM_REG2, ADDR_CACHE_TBL_SZ2, TBL_LOOKUP, NULL} 
};

#if USE_PROC
struct proc_dir_entry addrCache_proc_entry = {
    0,                        /* low_ino: the inode -- dynamic */
    9,   "addrCache",         /* len of name and name */
    S_IFREG | S_IRUGO,        /* mode */
    1, 0, 0,                  /* nlinks, owner, group */
    0,                        /* size -- unused */
    NULL,                     /* operations -- use default */ 
    &addrCache_read_proc_indirect  /* fn used to read data */
};
#endif

struct file_operations addrCache_fops =
{
    ioctl: addrCache_ioctl, 
};

/* memcpy to/from user (3): use an address cache instead of 
   calling calcPhysAddress()
*/

int memcpy3_to_user( void* vaddr, void* from, int nbytes, void *addrkey,
                                                  struct task_struct *task)
{
  int index, offset, rc;
  unsigned long block, virt;
  addr_tbl_t *table = NULL;
  addr_key_t *addr_key = (addr_key_t*) addrkey;


  //table = &addrCache[addr_key->cache_id].addr_tbl[addr_key->table_id];
  table = &addrTblPtrs[addr_key->cache_id][addr_key->table_id];

  if ( table->length < nbytes ) {
    printk("memcpy3_to_user: ERROR -- insufficient validation...\n");
    return -1;
  }

  if ( ! table->valid ) {
//    printk("m3tu: calling revalidate: table->start= 0x%lx\n", table->start);
//    rc = addrCache_revalidate( table, (unsigned long) vaddr, nbytes, task ); 
    rc = addrCache_revalidate( table, table->start, table->length, task ); 
    if ( rc < 0 ) {
      printk("memcpy3_to_user: revalidate() BAD: vaddr= %p, nbytes= %d\n",
                     vaddr, nbytes);
      return -1;
    }
  }

  virt = (unsigned long) vaddr;

  offset = virt - table->start;

  if ( offset < 0 ) {
    printk("memcpy3_to_user: FATAL -- requesting memory underrun\n");
    printk("memcpy3_to_user: virt= 0x%lx, start= 0x%lx, len= %d\n",
         virt, table->start, nbytes);
    printk("memcpy3_to_user: cache_id= %d, table_id= %d\n",
         addr_key->cache_id, addr_key->table_id);

    /* i guess caller should generate a SIGPIPE... */
    return -1;
  }
 
  if ( offset + nbytes > table->length ) {
    printk("memcpy3_to_user: FATAL -- requesting memory overrun for\n");
    printk("memcpy3_to_user: vaddr= 0x%lx, nbytes= %d\n", virt, nbytes);
    printk("memcpy3_to_user: cache_id= %d, table_id= %d\n", addr_key->cache_id, addr_key->table_id);
    printk("memcpy3_to_user: cache of length %d\n", table->length);
    printk("memcpy3_to_user: cache start 0x%lx\n", table->start);

    /* i guess caller should generate a SIGPIPE... */
    return -1;
  }

  index = 0;

  if ( (offset - table->chunk0) >= 0 ) { /* not in the 0th slot */

    offset -= table->chunk0;
    index = offset / PAGE_SIZE + 1;

    /* offset into the page represented by this slot */
    offset = offset & ~PAGE_MASK;

    /* for the initial slot, we might not be on a page
       boundary; subsequently we are (i.e., offset==0), 
       automagically...
    */

    /* technically, would need "block = table->chunkn - offset"
       here if this is the chunkn slot, but should not be a problem
       since we already did a specific check for overrun of the 
       validated region... we have 

          block  = PAGE_SIZE     - offset 
                >= table->chunkn - offset
                >= nbytes   (if last slot, by the overrun check and 
                             the way the cache was populated)
          => block -> nbytes in the code below
    */
    block = PAGE_SIZE - offset;
  }
  else {
    block = table->chunk0 - offset;
  }

  if ( nbytes < block ) {
    block = nbytes;
  }

  index += table->slot;

  //printk("memcpy3_to_user: memcpy(*), %d bytes\n", block);
  memcpy( (void*) addrSlot[index++]+offset, from, block );
  nbytes -= block;

  while ( nbytes ) {
    from += block;

    if ( nbytes < PAGE_SIZE ) {
      /* should be done... */
      block = nbytes;
    }    
    else {
      block = PAGE_SIZE;
    }

    //printk("memcpy3_to_user: memcpy(), %d bytes\n", block);
    memcpy( (void*) addrSlot[index++], from, block );
    nbytes -= block;
  }
  return 0;
}

int memcpy3_from_user(void* to, void* vaddr, int nbytes, void *addrkey,
                                                  struct task_struct *task)
{
  int index, offset, rc;
  unsigned long block, virt;
  addr_tbl_t *table = NULL;
  addr_key_t *addr_key = (addr_key_t*) addrkey;

  //printk("memcpy_3_from_user: nbytes= %d\n", nbytes);

  //table = &addrCache[addr_key->cache_id].addr_tbl[addr_key->table_id];
  table = &addrTblPtrs[addr_key->cache_id][addr_key->table_id];

  if ( table->length < nbytes ) {
    printk("memcpy3_from_user: ERROR -- insufficient validation...\n");
    printk("memcpy3_from_user: cache_id= %d, table_id= %d\n", addr_key->cache_id, addr_key->table_id);
    return -1;
  }

  if ( ! table->valid ) {
//    printk("m3fu: calling revalidate: table->start= 0x%lx\n", table->start);
//    rc = addrCache_revalidate( table, (unsigned long) vaddr, nbytes, task ); 
    rc = addrCache_revalidate( table, table->start, table->length, task ); 
    if ( rc < 0 ) {
      printk("memcpy3_from_user: revalidate() BAD: vaddr= %p, nbytes= %d\n",
                     vaddr, nbytes);
      printk("table->start= 0x%lx\n", table->start);
      return -1;
    }
  }

  virt = (unsigned long) vaddr;

  offset = virt - table->start;

  if ( offset < 0 ) {
    printk("memcpy3_from_user: FATAL -- requesting memory underrun\n");
    printk("memcpy3_from_user: virt= 0x%lx, start= 0x%lx, len= %d\n",
      virt, table->start, nbytes);
    printk("memcpy3_from_user: cache_id= %d, table_id= %d\n",
         addr_key->cache_id, addr_key->table_id);

    /* i guess caller should generate a SIGPIPE... */
    return -1;
  }
 
  if ( offset + nbytes > table->length ) {
    printk("memcpy3_from_user: FATAL -- requesting memory overrun for\n");
    printk("memcpy3_from_user: cache of length %d\n", table->length);

    /* i guess caller should generate a SIGPIPE... */
    return -1;
  }

  index = 0;

  if ( (offset - table->chunk0) >= 0 ) { /* not in the 0th slot */

    offset -= table->chunk0;
    index = offset / PAGE_SIZE + 1;

    /* offset into the page represented by this slot */
    offset = offset & ~PAGE_MASK;

    /* for the initial slot, we might not be on a page
       boundary; subsequently we are (i.e., offset==0), 
       automagically...
    */

    /* technically, would need "block = table->chunkn - offset"
       here if this is the chunkn slot, but should not be a problem
       since we already did a specific check for overrun of the 
       validated region... we have 

          block  = PAGE_SIZE     - offset 
                >= table->chunkn - offset
                >= nbytes   (if last slot, by the overrun check and 
                             the way the cache was populated)
          => block -> nbytes in the code below
    */
    block = PAGE_SIZE - offset;
  }
  else {
    block = table->chunk0 - offset;
  }

  if ( nbytes < block ) {
    block = nbytes;
  }

  index += table->slot;

  //printk("memcpy3_from_user: memcpy(*), %d bytes\n", block);
  memcpy( to, (void*) addrSlot[index++]+offset, block );
  nbytes -= block;

  while ( nbytes ) {
    to += block;

    if ( nbytes < PAGE_SIZE ) {
      /* should be done... */
      block = nbytes;
    }    
    else {
      block = PAGE_SIZE;
    }

    //printk("memcpy3_from_user: memcpy(), %d bytes\n", block);
    memcpy( to, (void*) addrSlot[index++], block );
    nbytes -= block;
  }
  return 0;
}


/* addrCache_populate: to be called in the context of
   a running process, NOT at interrupt time...
   fills in an address cache structure for the specified 
   region (vaddr,nbytes)... mainly, does virt-to-phys
   translation of this region

   would be nice if physically contiguous pages were
   coalesced...
*/
int addrCache_populate( unsigned long vaddr, int nbytes, 
                                      int ppid, void **addr_key )
{
  /* addr_key is a reference to an MD's addrkey pointer
     this fn has considerable responsibility wrt the pointer:
     it must be given a value, even if NULL, upon returning. 
     this ensures (since this is the only routine that updates 
     the MD's pointer) that an out-of-date pointer value (one
     corresponding to a previous process, for example) does not
     get passed along to the memory access routines... the latter
     will not try to dereference a NULL ptr, but could try to 
     use a stale nonNULL ptr! 
  */

  int i=0, index=0;
  unsigned long phys_addr;
  unsigned long chunk;
  addr_tbl_t *table = NULL;
  addr_cache_t* cache;
  int cache_id, table_id;
  unsigned long flags;

  if ( vaddr == 0 || nbytes == 0 ) {
   *addr_key = NULL;
    return 0;
  }

  /* use the cache that corresponds to the
     size of the region...
  */
  if ( nbytes <= addrCache[0].reg_sz ) {
    cache = &addrCache[0];
    cache_id = 0;
  }
  else {
    if ( nbytes <= addrCache[1].reg_sz ) {
      cache = &addrCache[1];
      cache_id = 1;
    } 
    else {
      cache = &addrCache[2];
      cache_id = 2;
    }
  }


  switch( addrCache[cache_id].addr_cache_strategy ) {

    case TBL_REDO:
    /* get the next free table from this subcache */
  
    i = addrCache_tblUnlink(cache_id);

    if ( i < 0 ) {
      /* ran out of validation space (Oops!) */
      printk("addrCache_pop: ran out of validations in cache %d\n", 
                                          cache_id);
     *addr_key = NULL;
      return -1;
    }
    else {
      table_id = i;
      table = &addrTblPtrs[cache_id][table_id];
     *addr_key = (void *) &(table->addr_key);

      /* add the new table to the task's list of validations */
      table->next = addrCache[cache_id].taskTbl[ppid];
      if (table->next) {
        table->next->prev = table;
      }
      addrCache[cache_id].taskTbl[ppid] = table;
    }
    break;

    case TBL_LOOKUP:
    /* see if we have the region in this cache -- search most
       recent validations first */

#if 1
    for ( table=addrCache[cache_id].taskTbl[ppid]; table; table=table->next) {
      if ( table->start == vaddr ) {
       *addr_key = (void *) &(table->addr_key);
        table_id = table->addr_key.table_id;
        //printk("addrCache_pop: SUCCEEDED on key,len 0x%x,%d cache,table=(%d,%d)\n", (unsigned int) vaddr, nbytes, cache_id, table_id);

        /* if we're already sufficiently validated, nothing to do */
        if ( table->length >= nbytes && table->valid ) {
          //printk("addrCache_pop: cache sufficiently validated...0x%x,%d\n", (unsigned int) vaddr, nbytes);
          return 0;
        }
        break;
      }
    } 
#endif

    if ( table == NULL ) {
      /* did not find the region in the process list;
         try to get a free table from this subcache */

      i = addrCache_tblUnlink(cache_id);

      if ( i < 0 ) {
        /* ran out of validation space (Oops!) */
        printk("addrCache_pop: ran out of validations in cache %d\n", 
                                          cache_id);
       *addr_key = NULL;
        return -1;
      }

      table_id = i;
      table = &addrTblPtrs[cache_id][table_id];
     *addr_key = (void *) &(table->addr_key);
       
      /* add the new table to the task's list of validations */
      table->next = addrCache[cache_id].taskTbl[ppid];
      addrCache[cache_id].taskTbl[ppid] = table;

#if USE_ADDR_LIST
#if 0
      if (num_addresses < ADDR_LIMIT && nbytes != 8064 
                                     && ppid >= PPID_FLOATING) {
        addrlist[num_addresses  ].len  = nbytes;
        addrlist[num_addresses++].addr = vaddr;
      } 
#endif
      if ( ppid > MAX_FIXED_PPID ) {
        addrSummary.numvals[cache_id]++;
      }
#endif
    }
    else { 
      /* table is not NULL but table->length < nbytes or
                              !table->valid or both
         anyways, continue w/o adding the table to this
         task's list (redundant)...
      */
    }
    break;

    default:
    printk("addrCache_populate: no caching strategy for region,\n");
    printk("addrCache_populate: vaddr= 0x%lx, nbytes= %d, cache_id= %d\n", 
                                                  vaddr, nbytes, cache_id);
    *addr_key = (void *) &(table->addr_key);
    return -1;
    break;
  }
  /*----------------------------------------------------------------------*/

#if 1
  save_flags(flags);
  cli();
#endif

  table->start  = vaddr;
  table->length = nbytes;

  phys_addr = (unsigned long) calcPhysAddress( current, vaddr );

  if ( ! phys_addr )   {
    /* couldn't translate virtual address; don't use addr */
    printk("addrCache_pop: calcPhysAddress failed: 0x%lx\n", vaddr);
    table->length = 0;
   *addr_key = NULL;
    restore_flags(flags);
    return -1;
  }

  index = table->slot;

  addrSlot[index++] = phys_addr;
  //printk("validation 0: 0x%lx\n", phys_addr);
    
  /* if we are not on a page boundary */
  if ( phys_addr & ~PAGE_MASK ) {
    chunk = PAGE_SIZE - ( phys_addr & ~PAGE_MASK );
    if ( nbytes < chunk ) {
      chunk = nbytes;
    } 
  } 
  else {
    if ( nbytes >= PAGE_SIZE ) {
      chunk = PAGE_SIZE;
    } 
    else {
      chunk = nbytes;
    }
  }  
  nbytes -= chunk;
  table->chunk0 = chunk;

  i=0;
  while ( nbytes ) {
    i++;

    vaddr += chunk;

    phys_addr = (unsigned long) calcPhysAddress( current, vaddr );

    if ( !phys_addr )   {
      /* couldn't translate virtual address; don't use addr */
      printk("addrCache_pop: calcPhysAddress failed: 0x%lx\n", vaddr);

      /* the following should throw up a red flag when this translation 
         is referred to later...  */
      table->length = 0;
     *addr_key = NULL;
      restore_flags(flags);
      return -1;
    }

    addrSlot[index++] = phys_addr;
    //printk("validation %d: 0x%lx\n", i, phys_addr);
    
    if ( nbytes >= PAGE_SIZE ) {
      chunk = PAGE_SIZE;
    } 
    else {
      chunk = nbytes;
    }

    nbytes -= chunk;
  }

  table->chunkn  = chunk;
  table->nchunks = i;

  table->valid = 1;

  restore_flags(flags);
  return 0;
}

int addrCache_revalidate( addr_tbl_t *table, unsigned long vaddr, 
                                   int nbytes, struct task_struct *task)
{
  int i=0, index=0;
  unsigned long phys_addr;
  unsigned long chunk;
  unsigned long flags;

//printk("revalidate: revalidating 0x%lx, %d\n", vaddr, nbytes);

  if ( vaddr == 0 || nbytes == 0 ) {
    table->valid = 1; 
    return 0;
  }

  /*----------------------------------------------------------------------*/

#if 1
  save_flags(flags);
  cli();
#endif

  table->start  = vaddr;
  table->length = nbytes;

  phys_addr = (unsigned long) calcPhysAddress( task, vaddr );

  if ( ! phys_addr )   {
    /* couldn't translate virtual address; don't use addr */
    printk("addrCache_reval: calcPhysAddress failed: 0x%lx\n", vaddr);
    table->length = 0;
    restore_flags(flags);
    return -1;
  }

  index = table->slot;

  addrSlot[index++] = phys_addr;
  //printk("validation 0: 0x%lx\n", phys_addr);
    
  /* if we are not on a page boundary */
  if ( phys_addr & ~PAGE_MASK ) {
    chunk = PAGE_SIZE - ( phys_addr & ~PAGE_MASK );
    if ( nbytes < chunk ) {
      chunk = nbytes;
    } 
  } 
  else {
    if ( nbytes >= PAGE_SIZE ) {
      chunk = PAGE_SIZE;
    } 
    else {
      chunk = nbytes;
    }
  }  
  nbytes -= chunk;
  table->chunk0 = chunk;

  i=0;
  while ( nbytes ) {
    i++;

    vaddr += chunk;

    phys_addr = (unsigned long) calcPhysAddress( task, vaddr );

    if ( !phys_addr )   {
      /* couldn't translate virtual address; don't use addr */
      printk("addrCache_reval: calcPhysAddress failed: 0x%lx\n", vaddr);

      /* the following should throw up a red flag when this translation 
         is referred to later...  */
      table->length = 0;
      restore_flags(flags);
      return -1;
    }

    addrSlot[index++] = phys_addr;
  //printk("validation %d: 0x%lx\n", i, phys_addr);
    
    if ( nbytes >= PAGE_SIZE ) {
      chunk = PAGE_SIZE;
    } 
    else {
      chunk = nbytes;
    }

    nbytes -= chunk;
  }

  table->chunkn  = chunk;
  table->nchunks = i;

  table->valid = 1;

  restore_flags(flags);
  return 0;
}

int addrCache_tblUnlink(cache_id)
{
  int table_id;
  addr_tbl_t* table;

  if ( addrCache[cache_id].freeTables <= 0 ) {
    return -1;
  }

  table = addrCache[cache_id].addr_tbl;

  table_id = table->addr_key.table_id;

  /* unlink */
  addrCache[cache_id].addr_tbl = table->next;

  table->next = NULL;

  addrCache[cache_id].freeTables--;

  return table_id;
}

void addrCache_tblLink(void *addrkey, int ppid)
{
  addr_tbl_t* table;
  addr_key_t *addr_key = (addr_key_t*) addrkey;
  int cache_id, table_id;

  if ( !addrkey ) {
    return;
  }

  cache_id = addr_key->cache_id;

  if ( addrCache[cache_id].addr_cache_strategy == TBL_LOOKUP ) {
    return;
  }

  table_id = addr_key->table_id;

  table = &addrTblPtrs[cache_id][table_id];

  /* take off of process's list */
  if (table->next) {
    table->next->prev = table->prev;
  }
  if (table->prev) {
    table->prev->next = table->next;
    table->prev = NULL; 
  }
  else { /* must been first on the list, so update the head */
    addrCache[cache_id].taskTbl[ppid] = table->next;
  }

  /* link into free pool -- put before the current head */
  table->next = addrCache[cache_id].addr_tbl;
  addrCache[cache_id].addr_tbl = table;

  addrCache[cache_id].freeTables++;

  return;
}
/*------------------------------------------------------------------------*/

int init_module(void)
{
  int i, j;
  int slotInd;

#if USE_PROC
#ifdef LINUX24  
  create_proc_read_entry("addrCache", S_IFREG | S_IRUGO, proc_cplant,
                          addrCache_read_proc, NULL);
#else
  proc_register(proc_cplant, &addrCache_proc_entry);
#endif
#endif

  if (register_chrdev(CACHE_MAJOR, "addrCache", &addrCache_fops)) {
    printk("addrCache init_module: unable to get major dev num= %d\n",
                                 CACHE_MAJOR);
    return -EIO;
  }

  /* init the address cache */

  /* have each table point to its set of slots */
  slotInd = 0;
  for (j=0; j<NUM_SUBCACHES; j++) {
    for (i=0; i<addrCache[j].num_reg; i++) {
    //addrCache[j].addr_tbl[i].table_id = i;
      addrTblPtrs[j][i].addr_key.table_id = i;
      addrTblPtrs[j][i].addr_key.cache_id = j;
    //addrCache[j].addr_tbl[i].tbl = slotPtr;
      addrTblPtrs[j][i].slot = slotInd;
      slotInd += addrCache[j].tbl_sz;
    }
  }

  /* make a linked list of the tables in each subcache */
  for (j=0; j<NUM_SUBCACHES; j++) {
    addrCache[j].num_reg_used = 0;
    addrCache[j].addr_tbl = addrTblPtrs[j]; 
    addrCache[j].freeTables = addrCache[j].num_reg;
    for (i=0; i<addrCache[j].num_reg-1; i++) {
      addrTblPtrs[j][i].next = &addrTblPtrs[j][i+1];
      addrTblPtrs[j][i].prev = NULL;
    }
    addrTblPtrs[j][addrCache[j].num_reg-1].next = NULL;
    addrTblPtrs[j][addrCache[j].num_reg-1].prev = NULL;
  }
  return 0;
}

/* this should be called "at the beginning" of each application run 
   (i.e., on opening the portals device) */
void addrCache_tblInit(int ppid)
{
  int j;
  char *name;

#if USE_ADDR_LIST
  if ( ppid > MAX_FIXED_PPID ) {

    rank = cTaskGetRank(ppid);
    if (rank == 1) {
      name = cTaskGetName(ppid);
      if ( name ) {
        strcpy( addrSummary.name, name);
      }
      else {
        strcpy( addrSummary.name, "unknown");
      }
      for (j=0; j<NUM_SUBCACHES; j++) {
        addrSummary.numvals[j]= 0;
      }
    }
  }

//  num_addresses = 0;
#endif

  for (j=0; j<NUM_SUBCACHES; j++) {
    if ( addrCache[j].taskTbl[ppid] ) {
      printk("addrCache_tblInit: addrCache[%d].taskTbl[%d] was not NULL!\n", 
                                                                  j, ppid);
      addrCache[j].taskTbl[ppid] = NULL;
    }
  }
}

/* this should be called "at the end" of each application
   run (i.e., on closing the portals device) */
void addrCache_tblClear(int ppid)
{
  addr_tbl_t *table, *next;
  int cache_id;

  /* put any remaining address tables from this
     process list back on the free store */

  /* the TBL_LOOKUP strategy, for example, should leave
     alls its validations for this code to clean up.
     the TBL_REDO strategy should, on the other hand,
     always return its validations as messaging operations
     complete... finding remaining validations here
     would indicate an error... */

#if USE_ADDR_LIST
  if (rank == 1) {
    send_fn(&addrSummary);
  }
  rank=-1;
#endif

  for (cache_id=0; cache_id<NUM_SUBCACHES; cache_id++) {

    table = addrCache[cache_id].taskTbl[ppid];

    while( table ) {
      next = table->next;

      /* link -- put this one before the current head */
      table->next = addrCache[cache_id].addr_tbl;
      addrCache[cache_id].addr_tbl = table;

      addrCache[cache_id].freeTables++;

      table->start = 0;
      table->length = 0;

      table = next;
    }
    addrCache[cache_id].taskTbl[ppid] = NULL;
  }
}


void cleanup_module(void)
{
#if USE_PROC
#ifdef LINUX24
  remove_proc_entry("addrCache", proc_cplant);
#else
  proc_unregister(proc_cplant, addrCache_proc_entry.low_ino);
#endif
#endif
  unregister_chrdev(CACHE_MAJOR, "addrCache");
}


int
addrCache_ioctl(struct inode *inodeP, struct file *fileP,
                unsigned int cmd, unsigned long arg)
{
  int rc, retval = 0;
  struct vm_area_struct *vma=NULL;

  switch(cmd) {
    case CACHE_GET_ADDR_LIST:
      /* copy the addrlist into the given buffer */
      rc = copy_to_user((void*)arg, addrlist,
                                   num_addresses * sizeof(addr_entry_t));
      if (rc < 0) {
        printk("addrCache_ioctl: CACHE_GET_ADDR_LIST, copy_to_user FAILED\n");
        retval = -1;
      } 
      break;

#if 0
    case CACHE_GET_LEN_LIST:
      /* copy the lenlist into the given buffer */
      rc = copy_to_user((void*)arg, lenlist, num_addresses * sizeof(int));
      if (rc < 0) {
        printk("addrCache_ioctl: CACHE_GET_LEN_LIST, copy_to_user FAILED\n");
        retval = -1;
      } 
      break;
#endif

    case CACHE_GET_VAL:
      /* display the validation tables for specified ppid */
      addrCache_display( (int) arg );
      break;

    case CACHE_INVALIDATE:
      /* mark all the validation tables for the current process
         as invalid -- if memcpy3 attempts to use one of these
         tables it will have to revalidate -- populate() should
         consider this flag as well when it decides whether a
         region is sufficiently validated */
      addrCache_invalidate();
      break;

    case CACHE_USER_PAGE_IS_RDONLY:
      vma = find_vma(current->mm,arg);
      if ( !vma ) {
        printk("addrCache RDONLY?: no vma found for 0x%lx\n", arg);
        printk("addrCache RDONLY?: system process id= %d\n", current->pid);
        retval = -1;
        break;
      }
      if ( vma->vm_flags & VM_WRITE ) {
//        printk("addrCache RDONLY?: 0x%lx: vma->vm_flags == VM_WRITE (0x%x)\n",
//                                arg, vma->vm_flags);
      }
      else {
//        printk("addrCache RDONLY?: 0x%lx: vma->vm_flags != VM_WRITE (0x%x)\n",
//                                arg, vma->vm_flags);
        retval = 1;
      }
      break;

    default:
      printk("addrCache_ioctl: unknown action: %d\n", cmd);
      retval = -EINVAL;
  }
  return retval;
}


void addrCache_display(int ppid) 
{
  addr_tbl_t *table;
  int cache_id;

  for (cache_id=0; cache_id<NUM_SUBCACHES; cache_id++) {
    printk("display: subcache %d, ppid %d:\n", cache_id, ppid); 
    for (table=addrCache[cache_id].taskTbl[ppid]; table; table=table->next) {
      printk("display: (%d,%d) start=0x%lx, length=%d\n", 
                     table->addr_key.cache_id,
                     table->addr_key.table_id,
                     table->start, table->length);
    }
  }
}


void addrCache_invalidate(void)
{
  addr_tbl_t *table;
  int cache_id, ppid;

  ppid  = (int) cTaskGetCurrentPPID();
  for (cache_id=0; cache_id<NUM_SUBCACHES; cache_id++) {
    for (table=addrCache[cache_id].taskTbl[ppid]; table; table=table->next) {
       table->valid = 0;
    } 
  }
}


#if USE_PROC
int addrCache_read_proc_indirect(char* buf, char **start, off_t offset,
                        int len, int unused )
{
  int eof=0;
  return addrCache_read_proc(buf, start, offset, len, &eof, NULL);
}

int addrCache_read_proc(char* buf, char **start, off_t offset,
                        int count, int *eof, void *data)
{
#if USE_ADDR_LIST
  static int tog=0;
  int j;
#endif
  int len = 0;


#if USE_ADDR_LIST

  if (tog==0) {
    len += sprintf(buf+len, "num_addresses= %d\n", num_addresses);
  }
  else {
    for (j=0; j<num_addresses; j++) {
      if (addrlist[j].len != 8064) {
        len += sprintf(buf+len, "%d:  0x%lx   %d\n", j, addrlist[j].addr, 
                                                        addrlist[j].len);
      }
    }
  }
  tog = 1-tog;

#endif

  *eof = 1;
  return len;
}
#endif

void addrCache_reg_fn( int (*sender)(addr_summary_t* addrSumm))
{
    send_fn = sender;
}
