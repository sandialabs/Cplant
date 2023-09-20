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
** $Id: proc_init.c,v 1.7 2001/08/16 16:10:24 pumatst Exp $
** Portals 3.0 module file that manages the files in /proc/cplant
*/

#include <linux/proc_fs.h>

#include <sys/defines.h>		/* For FALSE */
#include "proc.h"
#include "proc_init.h"


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

static int we_installed_proc_cplant;

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** The portals 2.0 module already installed this. All we have to
** do is find it. (If it is not there, we'll install it.)
*/
struct proc_dir_entry *proc_cplant;

/* Install /proc/cplant */
#if 1
struct proc_dir_entry *cplant_proc_dir;
struct proc_dir_entry *p3_proc_dir;
#else
struct proc_dir_entry cplant_proc_dir =   {
    0,			/* low_ino: the inode -- dynamic */
    6, "cplant",	/* len of name and name */         
    S_IFDIR | S_IRUGO | S_IXUGO, 2, 0, 0, 0       
};

/* Install /proc/cplant/p3 */
struct proc_dir_entry p3_proc_dir =   {
    0,			/* low_ino: the inode -- dynamic */
    2, "p3",		/* len of name and name */         
    S_IFDIR | S_IRUGO | S_IXUGO, 2, 0, 0, 0       
};
#endif

#ifdef LINUX24
struct proc_dir_entry *p3debug_proc_entry;
struct proc_dir_entry *p3dev_proc_entry;
struct proc_dir_entry *p3nal_proc_entry;
struct proc_dir_entry *p3versions_proc_entry;
#else
/* Install /proc/cplant/p3/debug */
struct proc_dir_entry p3debug_proc_entry =   {
     0,				/* low_ino: the inode -- dynamic */
     5, "debug",		/* len of name and name */       
     S_IFREG | S_IRUGO,		/* mode */
     1, 0, 0,			/* nlinks, owner, group */
     5 * 8 * 1024,		/* size */
     NULL,			/* operations -- use default */
     &p3_read_debug_proc_indirect /* function used to read data */
};

/* Install /proc/cplant/p3/devices */
struct proc_dir_entry p3dev_proc_entry =   {
     0,				/* low_ino: the inode -- dynamic */
     7, "devices",		/* len of name and name */       
     S_IFREG | S_IRUGO,		/* mode */
     1, 0, 0,			/* nlinks, owner, group */
     128,			/* size */
     NULL,			/* operations -- use default */
     &p3_read_dev_proc_indirect	/* function used to read data */
};

/* Install /proc/cplant/p3/nal */
struct proc_dir_entry p3nal_proc_entry =   {
     0,				/* low_ino: the inode -- dynamic */
     3, "nal",			/* len of name and name */       
     S_IFREG | S_IRUGO,		/* mode */
     1, 0, 0,			/* nlinks, owner, group */
     2434,			/* size */
     NULL,			/* operations -- use default */
     &p3_read_nal_proc_indirect	/* function used to read data */
};

/* Install /proc/cplant/versions.rtscts */
struct proc_dir_entry p3versions_proc_entry = {
    0,			/* low_ino: the inode -- dynamic */
    11, "versions.p3",	/* len of name and name */
    S_IFREG | S_IRUGO,	/* mode: regular, read by anyone */
    1, 0, 0,		/* nlinks, owner (root), group (root) */
    2048, NULL,		/* "file" size is 1024 B; inode ops unused */
    &p3_read_versions_proc_indirect /* Function that lists the versions */
};
#endif


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** Find /proc/cplant. If it is not there, create it.
** Code duplicated in other Cplant modules.
*/

static struct proc_dir_entry *
find_proc_cplant(void)
{

struct proc_dir_entry *dir;


    we_installed_proc_cplant= FALSE;
    dir= &proc_root;
    dir= dir->subdir;
    do   {
	if (strncmp("cplant", dir->name, 6) == 0)   {
	    printk("Found /proc/cplant; we'll attach to that.\n");
	    break;
	}
	dir= dir->next;
    } while (dir);

    if (dir == NULL)   {
	printk("Cannot find /proc/cplant; we'll create it\n");
	/* Install /proc/cplant */
	we_installed_proc_cplant= TRUE;
#if 1
        cplant_proc_dir = create_proc_entry("cplant", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
#else
	proc_register(&proc_root, &cplant_proc_dir);
#endif
	dir= cplant_proc_dir;
    }
    return dir;

}  /* end of find_proc_cplant() */

void
remove_proc_cplant(struct proc_dir_entry *cplant_dir)
{

    /*
    ** If and only if, /proc/cplant is empty will we remove it
    */
    if (!cplant_dir->subdir && we_installed_proc_cplant)   {
        printk("Removing /proc/cplant\n");
#if 1
        remove_proc_entry("cplant", &proc_root);
#else
        proc_unregister(&proc_root, cplant_dir->low_ino);
#endif
    }

}  /* end of remove_proc_cplant() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

/*
** Install directories and files we need in /proc
*/
void
register_proc(void)
{
    proc_cplant= find_proc_cplant();
#if 1
    p3_proc_dir = create_proc_entry("p3", S_IFDIR | S_IRUGO | S_IXUGO, proc_cplant);
#else
    /* Install /proc/cplant/p3 */   
    proc_register(proc_cplant, p3_proc_dir);
#endif

#ifdef LINUX24
    p3debug_proc_entry = create_proc_read_entry("debug", S_IFREG | S_IRUGO, p3_proc_dir,
      p3_read_debug_proc, NULL);
    p3dev_proc_entry = create_proc_read_entry("devices", S_IFREG | S_IRUGO, p3_proc_dir,
      p3_read_dev_proc, NULL);
    p3nal_proc_entry = create_proc_read_entry("nal", S_IFREG | S_IRUGO, p3_proc_dir,
      p3_read_nal_proc, NULL);
    p3versions_proc_entry = create_proc_read_entry("versions.p3", S_IFREG | S_IRUGO, p3_proc_dir,
      p3_read_versions_proc, NULL);
#else
    /* Install /proc/cplant/p3/debug */
    proc_register(p3_proc_dir, &p3debug_proc_entry);

    /* Install /proc/cplant/p3/devices */
    proc_register(p3_proc_dir, &p3dev_proc_entry);

    /* Install /proc/cplant/p3/nal */
    proc_register(p3_proc_dir, &p3nal_proc_entry);

    /* Install /proc/cplant/p3/versions.p3 */
    proc_register(p3_proc_dir, &p3versions_proc_entry);
#endif

}  /* end of register_proc() */


/*
** Remove the directories and files we created in /proc
*/
void
unregister_proc(void)
{
#ifdef LINUX24
    remove_proc_entry("versions.p3", p3_proc_dir);
    remove_proc_entry("nal", p3_proc_dir);
    remove_proc_entry("devices", p3_proc_dir);
    remove_proc_entry("debug", p3_proc_dir);
#else
    /* Remove /proc/cplant/p3/versions.p3 */
    proc_unregister(p3_proc_dir, p3versions_proc_entry.low_ino);

    /* Remove /proc/cplant/p3/nal */
    proc_unregister(p3_proc_dir, p3nal_proc_entry.low_ino);

    /* Remove /proc/cplant/p3/devices */
    proc_unregister(p3_proc_dir, p3dev_proc_entry.low_ino);

    /* Remove /proc/cplant/p3/debug */
    proc_unregister(p3_proc_dir, p3debug_proc_entry.low_ino);
#endif

#if 1
    remove_proc_entry("p3", proc_cplant);
#else
    /* Remove /proc/cplant/p3 */
    proc_unregister(proc_cplant, p3_proc_dir.low_ino);
#endif

    /* Remove /proc/cplant if we were the ones who installed it */
    remove_proc_cplant(proc_cplant);

}  /* end of unregister_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
