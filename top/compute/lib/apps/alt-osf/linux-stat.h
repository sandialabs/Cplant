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

/* from Linux /usr/include/statbuf.h 
           or /usr/include/bits/stat.h (RedHat 6.x) */
 
struct linux_stat {
    unsigned long st_dev;
    unsigned int  st_ino;
#ifdef REDHAT_6
    int           __pad1;
#endif
    unsigned int  st_mode;
    unsigned int  st_nlink;
    unsigned int  st_uid;
    unsigned int  st_gid;
    unsigned long st_rdev;
    long          st_size;
    long          st_atime;
    long          st_mtime;
    long          st_ctime;
#ifdef REDHAT_6
    int           st_blocks;
    int           __pad2;
    unsigned int  st_blksize;
#else
    unsigned int  st_blksize;
    int           st_blocks;
#endif
    unsigned int  st_flags;
    unsigned int  st_gen;
#ifdef REDHAT_6
    int           __pad3;
    long          __unused[4];
#endif
};

