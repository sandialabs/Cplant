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
#include <asm/uaccess.h>

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

#include <portals/puma.h>
#include <portals/mlock.h>
#include <load/sys_limits.h>

#include <cTask/cTask.h>

#include <linux/unistd.h>

#if 0
#define PRINTK(args...) printk("portals: " args)
#else
#define PRINTK(args...) 
#endif

/* stuff used to do mlock syscalls thru ioctl */
typedef int (*FP)(unsigned long, ... );
extern FP *sys_call_table;
FP        *syscalls = (FP *)&sys_call_table;

#define CPLANT_MAJOR 63
#define SEC  1
#define NSEC 0

static int cTask_physNid = -1;
static int cTask_numPhysNodes = -1;
static unsigned int cTask_network;
static unsigned int cTask_netmask;

int cTask_ioctl(struct inode *inodeP, struct file *fileP,
                unsigned int cmd, unsigned long arg);
int cTask_read_proc_indirect(char* buf, char** start, off_t offset,
                      int len, int unused);
int cTask_read_proc(char* buf, char** start, off_t offset,
                      int count, int *eof, void *data);
static int cTask_open(struct inode *inodeP, struct file *fileP);
static int cTask_release(struct inode *inodeP, struct file *fileP);
static inline int sys_apcb( unsigned long param );

int nidpid_map_spid = -1;
NIDPID nidpid_map[MAX_NODES];

/*
 * PPIDs are 1 based because rtscts_send() doesn't like NULL send_tasks.
 * Instead of constantly adjusting from internal representation (0 based)
 * to external representation (1 based), I will allocate an extra element
 * and never use it.  Therefore, internal and external representation will
 * be 1 based.
 *
 */
static ppid_type Global_ppid = PPID_FLOATING-1;

taskInfo_t taskInfo[NUM_PTL_TASKS+1];

#if 1
struct proc_dir_entry *proc_cplant;
#else
/* Install /proc/cplant */
struct proc_dir_entry proc_cplant = {
   0,                  /* low_ino: the inode -- dynamic */
   6, "cplant",        /* len of name and name */
   S_IFDIR | S_IRUGO | S_IXUGO, 2, 0, 0, 0
};
#endif

#ifndef LINUX24
struct proc_dir_entry cTask_proc_entry = {
     0,                    /* low_ino: the inode -- dynamic */
     5,   "tasks",         /* len of name and name */       
     S_IFREG | S_IRUGO,    /* mode */
     1, 0, 0,              /* nlinks, owner, group */
     0,                    /* size -- unused */
     NULL,                 /* operations -- use default */
     &cTask_read_proc_indirect /* function used to read data */
};
#endif

static inline int taskInKernel( spid_type spid ); 

    EXPORT_SYMBOL(rank2pnid);
    EXPORT_SYMBOL(rank2ppid);
    EXPORT_SYMBOL(nidpid_map);
    EXPORT_SYMBOL(cTaskGetPhysNid);
    EXPORT_SYMBOL(cTaskGetNetwork);
    EXPORT_SYMBOL(cTaskGetNetmask);
    EXPORT_SYMBOL(cTaskGetNumPhysNodes);
    EXPORT_SYMBOL(taskInfo);
    EXPORT_SYMBOL(cTaskInit);
    EXPORT_SYMBOL(cTaskClear);
    EXPORT_SYMBOL(clearTheDead);
    EXPORT_SYMBOL(cTaskGetCurrentPPID);
    EXPORT_SYMBOL(cTaskGetEntry);
    EXPORT_SYMBOL(cTaskGetName);
    EXPORT_SYMBOL(cTaskGetRank);
    EXPORT_SYMBOL(cTaskSet_static);
    EXPORT_SYMBOL(cTaskSet_dynamic);
    EXPORT_SYMBOL(cTaskGetCurrentEntry);
    EXPORT_SYMBOL(getSpidIndex);
    EXPORT_SYMBOL(cTaskGetTask);
    EXPORT_SYMBOL(ppid2spid);
    EXPORT_SYMBOL(proc_cplant);

struct file_operations cTask_fops =
{
    ioctl  : cTask_ioctl,
    open   : cTask_open,
    release: cTask_release,
};

static inline int cTaskSetPhysNid( int pnid )
{
  if ( !suser() ) {
    return -EPERM;
  }
  if ( pnid < cTask_numPhysNodes && pnid >= 0 ) {
    cTask_physNid = pnid;
    return 0;
  }
  else {
    return -1;
  }
}

static inline int cTaskSetNetwork( unsigned int network )
{
  if ( !suser() ) {
    return -EPERM;
  }
  cTask_network = (unsigned int) network;
  return 0;
}

unsigned int cTaskGetNetwork(void)
{
  return cTask_network;
}

static inline int cTaskCopyNetwork( void *network )
{
  int rc;
  rc = copy_to_user(network, &cTask_network, sizeof(unsigned int));
  return rc;
}

static inline int cTaskSetNetmask( unsigned int netmask )
{
  if ( !suser() ) {
    return -EPERM;
  }
  cTask_netmask = netmask;
  return 0;
}

unsigned int cTaskGetNetmask(void)
{
  return cTask_netmask;
}

static inline int cTaskCopyNetmask( void *netmask )
{
  int rc;
  rc = copy_to_user(netmask, &cTask_netmask, sizeof(unsigned int));
  return rc;
}

int cTaskGetPhysNid( void )
{
  return cTask_physNid;
}

int cTaskGetNumPhysNodes( void )
{
  return cTask_numPhysNodes;
}

static inline int cTaskSetNumPhysNodes( int numNodes )
{
   if ( !suser() ) {
     return -EPERM;
   }

   cTask_numPhysNodes = numNodes;
   return 0;
}

static inline int set_nid_map( unsigned long param )
{
    taskInfo_t* entry;
    int nb_elements;
    int c;
    nid_type *vals;
    nid_type val;

    PRINTK("in PTL_SET_NID_MAP\n");
    vals = (nid_type *)param;

    if ( (entry = cTaskGetCurrentEntry()) == 0 ) {
      printk("set_nid_map: call to cTaskGetCurrentEntry() failed...\n");
      return -1;
    }
    nb_elements = entry->nprocs;

    PRINTK("nb_elements=%d\n", nb_elements);

    for (c = 0; c < nb_elements; c++) {
	copy_from_user(&val, &(vals[c]), sizeof(int));
	PRINTK("%d: %d\n", c, val);
	nidpid_map[c].nid = (nid_type) val;
    }
    PRINTK("out PTL_SET_NID_MAP\n");
    return 0;
}

inline static int set_pid_map( unsigned long param )
{
    taskInfo_t* entry;
    int nb_elements;
    int c;
    ppid_type *vals;
    ppid_type val;

    PRINTK("in PTL_SET_PID_MAP\n");
    vals = (ppid_type *)param;

    if ( (entry = cTaskGetCurrentEntry()) == 0 ) {
      printk("set_pid_map: call to cTaskGetCurrentEntry() failed...\n");
      return -1;
    }
    nb_elements = entry->nprocs;

    PRINTK("nb_elements=%d\n", nb_elements);

    for (c = 0; c < nb_elements; c++) {
	copy_from_user(&val, &(vals[c]), sizeof(int));
	PRINTK("%d: %d\n", c, val);
	nidpid_map[c].pid = (ppid_type) val;
    }
    PRINTK("out PTL_SET_PID_MAP\n");

    /* Who owns the latest nidpid map */
    nidpid_map_spid= current->pid;

    return 0;
}

int
cTask_ioctl(struct inode *inodeP, struct file *fileP,
            unsigned int cmd, unsigned long arg)
{
  int retval = 0;
  struct mlock_crud baby;

  switch(cmd) {
    case CPL_SET_PHYS_NID:
      retval = cTaskSetPhysNid( (int) arg );
      break;

    case CPL_GET_PHYS_NID:
      retval = cTaskGetPhysNid();
      break;

    case CPL_SET_NETWORK:
      retval = cTaskSetNetwork( (unsigned int) arg );
      break;

    case CPL_COPY_NETWORK:
      retval = cTaskCopyNetwork( (void*) arg );
      break;

    case CPL_SET_NETMASK:
      retval = cTaskSetNetmask( (unsigned int) arg );
      break;

    case CPL_COPY_NETMASK:
      retval = cTaskCopyNetmask( (void*) arg );
      break;

    case CPL_SYS_APCB:
      retval = sys_apcb(arg);

      if (!retval) {
        fileP->private_data = (void *)1;
      }
      break;

    case CPL_MLOCKALL:
#ifdef __ia64__
      retval = __ia64_syscall((int)arg, 0,0,0,0, __NR_mlockall);
#else
      retval = syscalls[__NR_mlockall](arg);
#endif
      break;

    case CPL_MLOCK:
      copy_from_user( &baby, (void*) arg, sizeof(struct mlock_crud));
#ifdef __ia64__
      retval = __ia64_syscall(baby.start, baby.len, 0,0,0, __NR_mlock);
#else
      retval = syscalls[__NR_mlock]( baby.start, baby.len );
#endif
      break;

    case CPL_MUNLOCK:
      copy_from_user( &baby, (void*) arg, sizeof(struct mlock_crud));
#ifdef __ia64__
      retval = __ia64_syscall(baby.start, baby.len, 0,0,0, __NR_munlock);
#else
      retval = syscalls[__NR_munlock]( baby.start, baby.len );
#endif
      break;

    case CPL_SET_NID_MAP:
      retval = set_nid_map( arg );
      break;

    case CPL_SET_PID_MAP:
      retval = set_pid_map( arg );
      break;

    case CPL_SET_NUM_PHYS_NODES:
      retval = cTaskSetNumPhysNodes((int)arg);
      break;

    case CPL_CLEAR_DEAD:
      if ( !suser() ) {
        return -EPERM;
      }
      clearTheDead();
      break;

    case CPL_GET_NUM_PHYS_NODES:
      retval = cTaskGetNumPhysNodes();
      break;

    default:
      printk("cTask_ioctl: unknown action %d from pid %d\n", cmd, current->pid);
      retval = -EINVAL;
  }
  return retval;
}

/* return task table index corr. to given portal pid
 
   the routine getSpidIndex() returns the task 
   table index corr. to a system pid. 
*/
static inline int findIndex( ppid_type ppid ) 
{
  int i;

  if ( (1 <= ppid) && (ppid <= MAX_FIXED_PPID) ) {
    PRINTK("findIndex: found index (%d) for ppid (%d)\n", ppid, ppid);
    return ppid;
  }

  if ( (PPID_FLOATING <= ppid) && (ppid <= MAX_PPID) ) {

    for (i=PPID_FLOATING; i<=NUM_PTL_TASKS; i++) {
      if ( taskInfo[i].ppid == ppid ) {
        PRINTK("findIndex: found index (%d) for ppid (%d)\n", i, ppid);
        return i;
      }
    }
    printk("findIndex: entry for ppid (%d) not found\n", ppid);
    return -1;

  }
  printk("findIndex: received ppid (%d) out of range\n", ppid);
  return -1;
}

/* 
  takes a system pid and returns the index in the taskInfo
  table of the task with the matching (system) pid
*/
int getSpidIndex( spid_type spid ) 
{
  int i;

  PRINTK("getSpidIndex(): system pid = %d\n", spid);

  /* search the system pids */
  for ( i=1; i<=NUM_PTL_TASKS; i++ ) {
    if ( taskInfo[i].spid == spid ) {
      PRINTK("getSpidIndex(): found index (%d) for sys pid (%d)\n", i, spid);
      return i;
    }
  }
  printk("getSpidIndex(): could not find entry for sys pid (%d)\n", spid);
  return -1;
}

/* clear out all the entries in the portals
   task table
*/
void cTaskInit( )
{
  int i;

  PRINTK("cTaskInit()\n");

  for ( i=1; i<=NUM_PTL_TASKS; i++ ) {
     taskInfo[i].free = 1;
     taskInfo[i].ppid = 0;
     taskInfo[i].spid = 0;
     taskInfo[i].gid  = 0;
     taskInfo[i].task = NULL;
  }
}

int cTaskReset(int i) {
  if (i < 1 || i > NUM_PTL_TASKS) {
    printk("cTaskReset: called with bad table index %d\n", i);
    return -1;
  }
  taskInfo[i].free = 1;
  taskInfo[i].ppid = 0;
  taskInfo[i].spid = 0;
  taskInfo[i].gid  = 0;
  taskInfo[i].task = NULL;
  return 0;
}


/* return the kernel task struct corresponding to
   a given portals pid -- used by routines that
   want to do vm area verification...
*/
struct task_struct *cTaskGetTask( ppid_type ppid )
{
  int index;

  PRINTK("in cTaskGetTask: just in\n");

  index = findIndex( ppid );

  /* assumes ppids are > 0 */
  if ( index <= 0 ) {
    printk("cTaskGetTask: failed w/ index %d on ppid %d\n", index, ppid);
    return 0;
  }

  PRINTK("out cTaskGetTask(): ppid=%d index=%d\n", ppid, index);
  return taskInfo[index].task;
}

/* return the process rank corresponding
   to a given portals pid 
*/
int cTaskGetRank( ppid_type ppid )
{
  int index;

  PRINTK("in cTaskGetRank: just in\n");

  index = findIndex( ppid );

  /* assumes ppids are > 0 */
  if ( index <= 0 ) {
    printk("cTaskGetRank: failed w/ index %d on ppid %d\n", index, ppid);
    return -1;
  }

  PRINTK("out cTaskGetRank(): ppid=%d index=%d\n", ppid, index);
  return (int) taskInfo[index].rid;
}

/* return the process name corresponding
   to a given portals pid 
*/
char *cTaskGetName( ppid_type ppid )
{
  int index;

  PRINTK("in cTaskGetName: just in\n");

  index = findIndex( ppid );

  /* assumes ppids are > 0 */
  if ( index <= 0 ) {
    printk("cTaskGetName: failed w/ index %d on ppid %d\n", index, ppid);
    return NULL;
  }

  PRINTK("out cTaskGetName(): ppid=%d index=%d\n", ppid, index);
  return taskInfo[index].name;
}


/* return ptr to the ith task table entry */
taskInfo_t* cTaskGetEntry(int index) 
{
  PRINTK("cTaskGetEntry(): top...\n");

  if (index < 1 || index > NUM_PTL_TASKS) {
    printk("pltTaskGetEntry: index %d out of range\n", index);
    return 0;
  }
  return &taskInfo[index];
}


/* return ptr to the task table entry for the synchronously
   running portals task
*/
taskInfo_t* cTaskGetCurrentEntry() 
{
  int i;

  PRINTK("cTaskGetCurrentEntry(): top...\n");

  /* search the system pids */
  for ( i=1; i<=NUM_PTL_TASKS; i++ ) {
    if ( taskInfo[i].spid == current->pid ) {
      PRINTK("cTaskGetCurrentEntry(): found entry (%d) for sys pid (%d)\n", i, spid);
      return &taskInfo[i];
    }
  }
  printk("cTaskGetCurrentEntry(): could not find entry for sys pid (%d)\n", current->pid);
  return 0;
}

/* return the ppid of the 
   synchronously running portals task 
*/
ppid_type cTaskGetCurrentPPID( void )
{
  int index;

  PRINTK("cTaskGetCurrentPPID()\n");

  index = getSpidIndex( current->pid );

  /* assumes ppids are > 0 */
  if ( index <= 0 ) {
    printk("cTaskGetCurrentPPID(): bad index (%d) for curr spid (%d)\n", index, current->pid);
    return 0;
  }

  PRINTK("cTaskGetCurrentPPID(): spid=%d, index=%d\n", current->pid, index);
  return taskInfo[index].ppid;
}
    

/* clear out the task table entry corresponding
   to the given ppid
*/
int cTaskClear( ppid_type ppid )
{
  int index;

  PRINTK("cTaskClear()\n");

  index = findIndex(ppid); 

  /* assumes ppids are > 0 */
  if ( index <= 0 ) {
    printk("cTaskClear: bad index (%d) for ppid %d\n", index, ppid);
    return -1;
  }

  taskInfo[index].free = 1;
  taskInfo[index].ppid = 0;
  taskInfo[index].pnid = 0;
  taskInfo[index].rid  = 0;
  taskInfo[index].gid  = 0;
  taskInfo[index].spid = 0;
  taskInfo[index].nprocs  = 0;
  taskInfo[index].task = NULL;
  return 0;
}


/* Find first free slot in taskInfo array */
static int cTask_findFree(void)
{
  int i;

  for ( i=PPID_FLOATING; i<=NUM_PTL_TASKS; i++ ) {
    if ( taskInfo[i].free ) {
      PRINTK("cTask_findFree: found free slot (%d)\n", i);
      return i;
    }
  }
  printk("cTask_findFree: could not find free slot\n");
  return -1;
}


ppid_type cTaskSet_dynamic( taskInfo_t *task_in )
{
  int slot;

  /* assumes free ppids are > PPID_FLOATING */
  if ( (slot = cTask_findFree()) < PPID_FLOATING ) {
    clearTheDead();
    if ( (slot = cTask_findFree()) < PPID_FLOATING ) {
      printk("cTaskSet_dynamic: bad slot (%d) after clearTheDead()\n", slot);
      return 0;
    }
  }

  /* see if counter needs turning over */
  if (Global_ppid == MAX_PPID) {
    Global_ppid = PPID_FLOATING-1;
  }
  taskInfo[slot].free = 0;

  taskInfo[slot].ppid = ++Global_ppid;

  taskInfo[slot].pnid   = task_in->pnid;
  taskInfo[slot].rid    = task_in->rid;
  taskInfo[slot].nprocs = task_in->nprocs;
  taskInfo[slot].gid    = task_in->gid;
  taskInfo[slot].spid   = current->pid;
  taskInfo[slot].task   = current;

  strcpy(taskInfo[slot].name, task_in->name);
  //printk("cTask(dynamic): registering %s\n", taskInfo[slot].name);

  //initPortalInfo(Global_ppid);
  return Global_ppid;
}


/* register a portals task with the requested ppid
*/
ppid_type cTaskSet_static( taskInfo_t *task_in )
{
  ppid_type ppid;

  if (!suser()) {
    printk("cTaskSet_static: user is not superuser\n");
    return 0;
  }

  ppid = task_in->ppid;

  if ( (1 <= ppid) && (ppid <= MAX_FIXED_PPID) ) {
    if ( !taskInfo[ppid].free && taskInKernel(taskInfo[ppid].spid) ) {
      printk("cTaskSet_static: could not allocate ppid %d: slot taken and spid (%d) in kernel\n", ppid, taskInfo[ppid].spid);
        return 0;
    }
    else {
      PRINTK("cTaskSet_static: allocating slot=ppid %d \n", ppid);
      taskInfo[ppid].free = 0;
      taskInfo[ppid].ppid = task_in->ppid;
      taskInfo[ppid].pnid = task_in->pnid;
      taskInfo[ppid].gid  = task_in->gid;
      taskInfo[ppid].spid = current->pid;
      taskInfo[ppid].task = current;

      strcpy(taskInfo[ppid].name, task_in->name);
      //printk("cTask(static): registering %s\n", taskInfo[ppid].name);

      //initPortalInfo(ppid);

      return ppid;
    }
  }
  printk("cTaskSet_static: ppid %d out of range\n", ppid);
  return 0;
}


void clearTheDead(void) 
{
  /* clear out portals task entries for tasks not
     represented in the kernel's list of tasks
  
     this is a recovery mechanism for the case that portals
     processes for some reason do not clear their slot

     the "for_each_task" macro below requires the export
     of init_task by a 2.0 kernel and init_task_union
     by a 2.1 kernel -- see linux/sched.h 
  */
  int i, slot_free;
  struct task_struct *p;

  PRINTK("clearTheDead: we'll free any slots used by dead\n");
  PRINTK("clearTheDead: portals tasks\n");

  /* loop through taskinfo[] */
  for ( i=1; i<=NUM_PTL_TASKS; i++ ) {
    slot_free=1;
    /* loop through kernel's list of tasks -- see linux/sched.h */
    for_each_task(p) {
      if (taskInfo[i].spid == p->pid) {
        slot_free=0;
        break;
      }
    }
    if (slot_free) {
      PRINTK("clearTheDead: found free slot %d\n", i);
      /* mark the entry as free */
      taskInfo[i].free = 1; 
      taskInfo[i].ppid = 0; 
      taskInfo[i].pnid = 0; 
      taskInfo[i].rid  = 0; 
      taskInfo[i].gid  = 0; 
      taskInfo[i].spid = 0; 
      taskInfo[i].nprocs = 0; 
      taskInfo[i].task = NULL; 
    }
  }
}


/* check to see if the task represented by the
   given system pid is still in the kernel's
   list of tasks
*/
static inline int taskInKernel( spid_type spid ) 
{
  int rc = 0;
  struct task_struct *p;

  for_each_task(p) {
    if (spid == p->pid) {
      rc = 1;
      break;
    }
  }
  return rc;
}


/* return system pid corresponding to given portals pid
*/
spid_type ppid2spid( ppid_type ppid ) 
{
  int rc;

  /* assumes ppids are > 0 */
  if ( (rc = findIndex(ppid)) <= 0 ) {
    printk("ppid2spid: bad slot slot (%d) for ppid %d\n", rc, ppid);
    return -1;
  }
  else {
    PRINTK("ppid2spid: found spid %d for ppid %d\n", taskInfo[rc].spid, ppid);
    return taskInfo[rc].spid;
  }
}

#if 1
/*
** The following two functions should really consult a nidpid map
** based on a given process. It seems there is only one per node
** right now?!?
*/
int
rank2pnid(int nid, taskInfo_t *taskInfo)
{

    if (nidpid_map_spid == taskInfo->spid)   {
	/* The nidpid map is valid for this process */
	return nidpid_map[nid].nid;
    } else   {
	return cTaskGetPhysNid();
    }

}  /* end of rank2pnid() */


int
rank2ppid(int nid, taskInfo_t *taskInfo)
{

    if (nidpid_map_spid == taskInfo->spid)   {
	/* The nidpid map is valid for this process */
	return nidpid_map[nid].pid;
    } else   {
	return taskInfo->ppid;
    }

}  /* end of rank2ppid() */
#endif


int init_module(void)
{
    PRINTK("init_module\n");

    proc_cplant = create_proc_entry("cplant", S_IFDIR | S_IRUGO | S_IXUGO, NULL);

#ifdef LINUX24
    create_proc_read_entry("tasks", S_IFREG | S_IRUGO, proc_cplant, cTask_read_proc, NULL);
#else
    proc_register(proc_cplant, &cTask_proc_entry);
#endif

    if (register_chrdev(CPLANT_MAJOR, "cTask", &cTask_fops)) {
      printk("cplantTask's init_module: unable to get major_device_num= %d\n", CPLANT_MAJOR);
      return -EIO;
    }

    cTaskInit();

    return 0;
}

void cleanup_module(void)
{
    PRINTK("cleanup_module\n");

#ifdef LINUX24
    remove_proc_entry("tasks", proc_cplant);
#else
    proc_unregister(proc_cplant, cTask_proc_entry.low_ino);
#endif
#if 1
    remove_proc_entry("cplant", &proc_root);
#else
    proc_unregister(&proc_root, proc_cplant.low_ino);
#endif

    unregister_chrdev(CPLANT_MAJOR, "cTask");
}

#ifndef LINUX24
int cTask_read_proc_indirect(char* buf, char** start, off_t offset,
                      int len, int unused)
{
  int eof=0;
  return cTask_read_proc(buf, start, offset, len, &eof, NULL);
}
#endif

int cTask_read_proc(char* buf, char** start, off_t offset,
                      int count, int *eof, void *data)
{
  int i=1, len=0;
  taskInfo_t* info = NULL;

  len += sprintf(buf+len, "----------------------------------------------\n");
  len += sprintf(buf+len, "PPid Key: bebopd = %d   pct = %d   fyod = %d\n", PPID_BEBOPD, PPID_PCT, PPID_FYOD);
  len += sprintf(buf+len, "----------------------------------------------\n");
  len += sprintf(buf+len, " Gid Key: bebopd = %d   pct = %d   fyod = %d\n", GID_BEBOPD,  GID_PCT,  GID_FYOD); 
  len += sprintf(buf+len, "          yod = %d   pingd = %d   bt = %d   ptltest = %d\n", GID_YOD,  GID_PINGD,  GID_BT,  GID_PTLTEST); 
  len += sprintf(buf+len, "          cgdb = %d\n", GID_CGDB); 
  len += sprintf(buf+len, "----------------------------------------------\n");

  len += sprintf(buf+len, "Fixed pids:\n\n");

  for (i=1; i<=MAX_FIXED_PPID; i++) {
    info = cTaskGetEntry(i);
    if (!info->free) {
      len += sprintf(buf+len, "entry %d: ppid %d, spid %d, gid %d (%s)\n", i, info->ppid, info->spid, info->gid, info->name);
    }
  }

  len += sprintf(buf+len, "\n");
  len += sprintf(buf+len, "Floating pids:\n\n");

  for (i=MAX_FIXED_PPID+1; i<=NUM_PTL_TASKS; i++) {
    info = cTaskGetEntry(i);
    if (!info->free) {
      len += sprintf(buf+len, "entry %d: ppid %d, spid %d, gid %d, nprocs %d (%s)\n",
	  i, info->ppid, info->spid, info->gid, info->nprocs, info->name);
    }
  }
  len += sprintf(buf+len, "----------------------------------------------\n");

  *eof = 1;
  return len;
}

static int
cTask_open(struct inode *inodeP, struct file *fileP)
{
    fileP->private_data = 0;  /* mark as never having registered in taskInfo */
    MOD_INC_USE_COUNT;
    return 0;
}

int
cTask_release(struct inode *inodeP, struct file *fileP)
{
  int index;
  int retval = 0;

  PRINTK("cTask_release: clearing task info for spid %i\n", current->pid);

  if (!fileP->private_data)  /* never did sys_apcb, don't search */
    goto out;

  /* retrieve the portals task index of the
     indicated process 
  */
  index = getSpidIndex( current->pid );

  if ( index < 0 ) {
    printk("cTask_release: could not get index of requested task...\n");
    retval = -1;
    goto out;
  }
  
  taskInfo[index].free = 1;
  taskInfo[index].ppid = 0;
  taskInfo[index].pnid = 0;
  taskInfo[index].rid  = 0;
  taskInfo[index].gid  = 0;
  taskInfo[index].spid = 0;
  taskInfo[index].nprocs = 0;
  taskInfo[index].task = NULL;

out:
  MOD_DEC_USE_COUNT;
  return retval;
}

static inline int sys_apcb( unsigned long param )
{ 
    ppid_type ppid;
    taskInfo_t taskI;
    taskInfo_t *task_in;

    /* 
    ** APCB location initialization.
    ** This call MUST be executed one time before any other
    ** operation on portals.
    */
    PRINTK("(PTL_SYS_APCB) initializing user PCB\n");

    task_in = (taskInfo_t*) param;

    copy_from_user( &taskI, task_in, sizeof(taskInfo_t) );

    ppid = taskI.ppid;

    if ( ppid == 0 ) {
      ppid = cTaskSet_dynamic( &taskI );
    }
    else {
      if ( (1 <= ppid) && (ppid <= MAX_FIXED_PPID) ) {
        ppid = cTaskSet_static( &taskI );
      } 
      else {
        printk("sys_apcb: ppid %d out of range\n", ppid);
        return -1;
      }
    }

    if ( ppid == 0 ) {
      printk("sys_apcb: did not get requested ppid (%d)\n", ppid);
      return -1;
    }

    copy_to_user( &task_in->ppid, &ppid, sizeof(ppid_type) );

    return 0;
}

