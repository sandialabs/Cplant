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
 *  NOTES:  in recursive data structures, i call the helper functions with
 *          ind + 1 on each occasion with the assumption that the evaluation
 *          of (ind + 1) will be retained in a register by the compiler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DANiEL

#include "nfs_debug.h"

#else

#include "cmn.h"
#if 0
#include "smp.h"
#endif
#include "mountsvc.h"
#include "nfssvc.h"

#endif /* #ifdef DANiEL */

IDENTIFY("$Id: nfs_debug.c,v 0.4 2001/07/18 18:57:31 rklundt Exp $");

#define SURPRESS_DATA

/*
 *  forward references for helpers.
 */

static void
mount_dbgsvc_mountbody_help( mountbody* s, char* leader, unsigned ind );

static void
mount_dbgsvc_groupnode_help( groupnode* s, char* leader, unsigned ind );

static void
mount_dbgsvc_exportnode_help( exportnode* s, char* leader, unsigned ind );

static char*
print_nfsstat( nfsstat s );


/*
 *  private formating functions.
 */

static char*
getLeader( char* leader, int i, char* tag ){

  char* indentStr;
  char* ldr;

  indentStr = (char*) m_alloc( i + 1 );
  memset( indentStr, ' ', i );
  indentStr[i] = '\0';
  ldr = (char*) m_alloc( strlen( indentStr ) + strlen( tag ) +
			strlen( leader ) + 1 );
  (void) sprintf( ldr, "%s%s%s", leader, indentStr, tag );
  free( indentStr );
  return( ldr );
}

static char*
printOpaque( void* datain, size_t dataLen ){

  unsigned char* data;
  char* ptr;
  char* str;
  unsigned i;

  data = (unsigned char*) datain;
  str = (char*) m_alloc( (dataLen << 1) + 3 );
  ptr = &str[0];
  (void) sprintf( ptr, "0x" );
  ptr += strlen( ptr );
  for( i = 0; i < dataLen; i++ ){
    (void) sprintf( ptr, "%02X", data[i] );
    ptr += strlen( ptr );
  }
  return( str );
}
 
/***********************************************************************
 *                                                                     *
 *                            from mount.h                             *
 *                                                                     *
 ***********************************************************************/

/*
 *  typedef char *dirpath;
 */

static void
mount_dbgsvc_dirpath_help( dirpath s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\"\n", ldr, s );
  free( ldr );
}

void
mount_dbgsvc_dirpath( dirpath* s, char* leader ){
  mount_dbgsvc_dirpath_help( *s, leader, 0 );
}

/*
 *  typedef struct mountbody *mountlist;
 */

static void
mount_dbgsvc_mountlist_help( mountlist s, char* leader, unsigned ind ){

  mountbody* current;
  
  for( current = s; current != NULL; current = current->ml_next ){

    /* INDENTATION SURPRESSED due to the fact that this data
       structure is used only to represent a collection or list. */
  
    mount_dbgsvc_mountbody_help( current, leader, ind );
  }
}

void
mount_dbgsvc_mountlist( mountlist* s, char* leader ){
  mount_dbgsvc_mountlist_help( *s, leader, 0 );
}

/*
 *  struct mountbody {
 *    name ml_hostname;
 *    dirpath ml_directory;
 *    mountlist ml_next;
 *  }
 */

static void
mount_dbgsvc_mountbody_help( mountbody* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\" \"%s\"\n",
	  ldr, s->ml_hostname, s->ml_directory );
  free( ldr );
}

void
mount_dbgsvc_mountbody( mountbody* s, char* leader ){
  mount_dbgsvc_mountbody_help( s, leader, 0 );
}

/*
 *  typedef struct groupnode *groups;
 */

static void
mount_dbgsvc_groups_help( groups s, char* leader, unsigned ind ){

  groupnode* current;
  
  for( current = s; current != NULL; current = current->gr_next ){

    /* INDENTATION SURPRESSED due to the fact that this data
       structure is used only to represent a collection or list. */
  
    mount_dbgsvc_groupnode_help( current, leader, ind );
  }
}

void
mount_dbgsvc_groups( groups* s, char* leader ){
  mount_dbgsvc_groups_help( *s, leader, 0 );
}

/*
 *  struct groupnode {
 *    name gr_name;
 *    groups gr_next;
 *  };
 */

static void
mount_dbgsvc_groupnode_help( groupnode* s, char* leader, unsigned ind ){
  
  char* ldr;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\"\n", ldr, s->gr_name );
  free( ldr );
}

void
mount_dbgsvc_groupnode( groupnode* s, char* leader ){
  mount_dbgsvc_groupnode_help( s, leader, 0 );
}

/*
 *  typedef struct exportnode *exports;
 */

static void
mount_dbgsvc_exports_help( exports s, char* leader, unsigned ind ){

  exportnode* current;

  for( current = s; current != NULL; current = current->ex_next ){

    /* INDENTATION SURPRESSED due to the fact that this data
       structure is used only to represent a collection or list. */
  
    mount_dbgsvc_exportnode_help( current, leader, ind );
  }
}

void
mount_dbgsvc_exports( exports* s, char* leader ){
  mount_dbgsvc_exports_help( *s, leader, 0 );
}

/*
 *  struct exportnode {
 *    dirpath ex_dir;
 *    groups ex_groups;
 *    exports ex_next;
 *  };
 */

static void
mount_dbgsvc_exportnode_help( exportnode* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\"\n", ldr, s->ex_dir );
  mount_dbgsvc_groups_help( s->ex_groups, ldr, ind + 1 );
  free( ldr );
}

void
mount_dbgsvc_exportnode( exportnode* s, char* leader ){
  mount_dbgsvc_exportnode_help( s, leader, 0 );
}

/*
 *  typedef char fhandle[FHSIZE];
 */

static void
mount_dbgsvc_fhandle_help( fhandle s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s, FHSIZE );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, opq );
  free( ldr );
  free( opq );
}

void
mount_dbgsvc_fhandle( fhandle* s, char* leader ){
  mount_dbgsvc_fhandle_help( *s, leader, 0 );
}

/*
 *  struct fhstatus {
 *    u_int fhs_status;
 *    union {
 *      fhandle fhs_fhandle;
 *    } fhstatus_u;
 *  };
 *  typedef struct fhstatus fhstatus;
 */

static void
mount_dbgsvc_fhstatus_help( fhstatus* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s->fhstatus_u.fhs_fhandle, FHSIZE );
  ns = print_nfsstat( s->fhs_status );
  if( s->fhs_status == NFS_OK ){
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s %s\n", ldr, ns, opq );
  } else {
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  }
  free( ldr );
  free( opq );
}

void
mount_dbgsvc_fhstatus( fhstatus* s, char* leader ){
  mount_dbgsvc_fhstatus_help( s, leader, 0 );
}

/***********************************************************************
 *                                                                     *
 *                          from nfs_prot.h                            *
 *                                                                     *
 ***********************************************************************/

/*
 *  enum ftype {
 *      NFNON = 0,
 *      NFREG = 1,
 *      NFDIR = 2,
 *      NFBLK = 3,
 *      NFCHR = 4,
 *      NFLNK = 5,
 *      NFSOCK = 6,
 *      NFBAD = 7,
 *      NFFIFO = 8,
 *  };
 *
 *  typedef enum ftype ftype;
 */

/* NOTE: this function returns a pointer to a const static string,
         do not attempt to free it.                                */

static char*
print_ftype( ftype s ){

  static char* str;

  switch( s ){
  case NFNON:
    str = "NFNON";
    break;
  case NFREG:
    str = "NFREG";
    break;
  case NFDIR:
    str = "NFDIR";
    break;
  case NFBLK:
    str = "NFBLK";
    break;
  case NFCHR:
    str = "NFCHR";
    break;
  case NFLNK:
    str = "NFLNK";
    break;
  case NFSOCK:
    str = "NFSOCK";
    break;
  case NFBAD:
    str = "NFBAD";
    break;
  case NFFIFO:
    str = "NFFIFO";
    break;
  default:
    str = "foo!  daniel screwed up the ftypes.";
    break;
  }
  return( str );
}

/*
 *  enum nfsstat {
 *       NFS_OK = 0,
 *       NFSERR_PERM = 1,
 *       NFSERR_NOENT = 2,
 *       NFSERR_IO = 5,
 *       NFSERR_NXIO = 6,
 *       NFSERR_ACCES = 13,
 *       NFSERR_EXIST = 17,
 *       NFSERR_NODEV = 19,
 *       NFSERR_NOTDIR = 20,
 *       NFSERR_ISDIR = 21,
 *       NFSERR_FBIG = 27,
 *       NFSERR_NOSPC = 28,
 *       NFSERR_ROFS = 30,
 *       NFSERR_NAMETOOLONG = 63,
 *       NFSERR_NOTEMPTY = 66,
 *       NFSERR_DQUOT = 69,
 *       NFSERR_STALE = 70,
 *       NFSERR_WFLUSH = 99,
 *  }
 *
 *  typedef enum nfsstat nfsstat;
 */

/* NOTE: this function returns a pointer to a const static string,
         do not attempt to free it.                                */

static char*
print_nfsstat( nfsstat s ){

  static char* str;

  switch( s ){
  case NFS_OK:
    str = "NFS_OK";
    break;
  case NFSERR_PERM:
    str = "NFSERR_PERM";
    break;
  case NFSERR_NOENT:
    str = "NFSERR_NOENT";
    break;
  case NFSERR_IO:
    str = "NFSERR_IO";
    break;
  case NFSERR_NXIO:
    str = "NFSERR_NXIO";
    break;
  case NFSERR_ACCES:
    str = "NFSERR_ACCES";
    break;
  case NFSERR_EXIST:
    str = "NFSERR_EXIST";
    break;
  case NFSERR_NODEV:
    str = "NFSERR_NODEV";
    break;
  case NFSERR_NOTDIR:
    str = "NFSERR_NOTDIR";
    break;
  case NFSERR_ISDIR:
    str = "NFSERR_ISDIR";
    break;
  case NFSERR_FBIG:
    str = "NFSERR_FBIG";
    break;
  case NFSERR_NOSPC:
    str = "NFSERR_NOSPC";
    break;
  case NFSERR_ROFS:
    str = "NFSERR_ROFS";
    break;
  case NFSERR_NAMETOOLONG:
    str = "NFSERR_NAMETOOLONG";
    break;
  case NFSERR_NOTEMPTY:
    str = "NFSERR_NOTEMPTY";
    break;
  case NFSERR_DQUOT:
    str = "NFSERR_DQUOT";
    break;
  case NFSERR_STALE:
    str = "NFSERR_STALE";
    break;
  case NFSERR_WFLUSH:
    str = "NFSERR_WFLUSH";
    break;
  default:
    str = "foo!  daniel screwed up the nftstats.";
    break;
  }
  return( str );
}

static void
nfs_dbgsvc_nfsstat_help( nfsstat s, char* leader, int ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s );
  free( ldr );
}

void
nfs_dbgsvc_nfsstat( nfsstat* s, char* leader ){
  nfs_dbgsvc_nfsstat_help( *s, leader, 0 );
} 

/*
 *  struct nfs_fh {
 *    opaque data[NFS_FHSIZE];
 *  };
 */

static void
nfs_dbgsvc_nfs_fh_help( nfs_fh* s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s->data, NFS_FHSIZE );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, opq );
  free( ldr );
  free( opq );
}

void
nfs_dbgsvc_nfs_fh( nfs_fh* s, char* leader ){
  nfs_dbgsvc_nfs_fh_help( s, leader, 0 );
}

/*
 *  struct nfstime {
 *    unsigned seconds;
 *    unsigned useconds;
 *  };
 */

/* NOTE: you must free the string returned by this function. */

static char*
print_nfstime( nfstime* s ){

  char* str;

  str = m_alloc( 128 );
  (void) sprintf( str, "%u.%u", s->seconds, s->useconds );
  return( str );
}

static void
nfs_dbgsvc_nfstime_help( nfstime* s, char* leader, unsigned ind ){

  char* ldr;
  char* nt;

  ldr = getLeader( leader, ind, "" );
  nt = print_nfstime( s );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, nt );
  free( ldr );
  free( nt );
}

void
nfs_dbgsvc_nfstime( nfstime* s, char* leader ){
  nfs_dbgsvc_nfstime_help( s, leader, 0 );
}

/*
 *  struct fattr {
 *    ftype type;
 *    unsigned mode;
 *    unsigned nlink;
 *    unsigned uid;
 *    unsigned gid;
 *    unsigned size;
 *    unsigned blocksize;
 *    unsigned rdev;
 *    unsigned blocks;
 *    unsigned fsid;
 *    unsigned fileid;
 *    nfstime	atime;
 *    nfstime	mtime;
 *    nfstime	ctime;
 *  }
 */

static void
nfs_dbgsvc_fattr_help( fattr* s, char* leader, unsigned ind ){

  char* ldr;
  char* ft;
  char* nt1;
  char* nt2;
  char* nt3;

  ldr = getLeader( leader, ind, "" );
  ft = print_ftype( s->type );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(type): %s (mode): 0%o (nlink): %u\n",
	  ldr, ft, s->mode, s->nlink );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(uid): %u (gid): %u (size): %uB  (blocksize): %uKB\n",
	  ldr, s->uid, s->gid, s->size, s->blocksize );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(rdev): %u (blocks): %u (fsid): 0x%X (fileid): 0x%X\n",
	  ldr, s->rdev, s->blocks, s->fsid, s->fileid );
  nt1 = print_nfstime( &s->atime );
  nt2 = print_nfstime( &s->mtime );
  nt3 = print_nfstime( &s->ctime );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(atime): %s\n", ldr, nt1 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(mtime): %s\n", ldr, nt2 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(ctime): %s\n", ldr, nt3 );
  free( nt1 );
  free( nt2 );
  free( nt3 );
  free( ldr );
}

void
nfs_dbgsvc_fattr( fattr* s, char* leader ){
  nfs_dbgsvc_fattr_help( s, leader, 0 );
}

/*
 *  struct sattr {
 *    unsigned mode;
 *    unsigned uid;
 *    unsigned gid;
 *    unsigned size;
 *    nfstime	atime;
 *    nfstime	mtime;
 *  }
 */

static void
nfs_dbgsvc_sattr_help( sattr* s, char* leader, unsigned ind ){

  char* ldr;
  char* nt1;
  char* nt2;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ),
	   "%s(mode): %u (uid): %u (gid): %u (size): %u\n",
	   ldr, s->mode, s->uid, s->gid, s->size );
  nt1 = print_nfstime( &s->atime );
  nt2 = print_nfstime( &s->mtime );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(atime): %s (mtime): %s\n",
	  ldr, nt1, nt2 );
  free( nt1 );
  free( nt2 );
  free( ldr );
}

void
nfs_dbgsvc_sattr( sattr* s, char* leader ){
  nfs_dbgsvc_sattr_help( s, leader, 0 );
}

/*
 *  union attrstat switch (nfsstat status) {
 *  case NFS_OK:
 *    fattr attributes;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_attrstat_help( attrstat* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  if( s->status == NFS_OK ){
    nfs_dbgsvc_fattr_help( &(s->attrstat_u.attributes), leader, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_attrstat( attrstat* s, char* leader ){
  nfs_dbgsvc_attrstat_help( s, leader, 0 );
}

/*
 *  struct sattrargs {
 *    nfs_fh file;
 *    sattr attributes;
 *  };
 */

static void
nfs_dbgsvc_sattrargs_help( sattrargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->file), ldr, ind + 1 );
  nfs_dbgsvc_sattr_help( &(s->attributes), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_sattrargs( sattrargs* s, char* leader ){
  nfs_dbgsvc_sattrargs_help( s, leader, 0 );
}

/*
 *  struct diropargs {
 *    nfs_fh	dir;
 *    filename name;
 *  };
 */

static void
nfs_dbgsvc_diropargs_help( diropargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->dir), ldr, ind + 1 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\"\n", ldr, s->name );
  free( ldr );
}

void
nfs_dbgsvc_diropargs( diropargs* s, char* leader ){
  nfs_dbgsvc_diropargs_help( s, leader, 0 );
}

/*
 *  struct diropokres {
 *    nfs_fh file;
 *    fattr attributes;
 *  };
 */

static void
nfs_dbgsvc_diropokres_help( diropokres* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->file), ldr, ind + 1 );
  nfs_dbgsvc_fattr_help( &(s->attributes), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_diropokres( diropokres* s, char* leader ){
  nfs_dbgsvc_diropokres_help( s, leader, 0 );
}

/*
 *  union diropres switch (nfsstat status) {
 *  case NFS_OK:
 *    diropokres diropres;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_diropres_help( diropres* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  if( s->status == NFS_OK ){
    nfs_dbgsvc_diropokres_help( &(s->diropres_u.diropres), ldr, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_diropres( diropres* s, char* leader ){
  nfs_dbgsvc_diropres_help( s, leader, 0 );
}

/*
 *  union readlinkres switch (nfsstat status) {
 *  case NFS_OK:
 *    nfspath data;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_readlinkres_help( readlinkres* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  if( s->status == NFS_OK ){
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s \"%s\"\n",
	    ldr, ns, s->readlinkres_u.data );
  } else {
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  }
  free( ldr );
}

void
nfs_dbgsvc_readlinkres( readlinkres* s, char* leader ){
  nfs_dbgsvc_readlinkres_help( s, leader, 0 );
}

/*
 *  struct readargs {
 *    nfs_fh file;
 *    unsigned offset;
 *    unsigned count;
 *    unsigned totalcount;
 *  };
 */

static void
nfs_dbgsvc_readargs_help( readargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->file), ldr, ind + 1 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ),
	   "%s(offset): %u (count): %u (totalcount): %u\n",
	   ldr, s->offset, s->count, s->totalcount);
  free( ldr );
}

void
nfs_dbgsvc_readargs( readargs* s, char* leader ){
  nfs_dbgsvc_readargs_help( s, leader, 0 );
}

/*
 *  struct readokres {
 *    fattr	attributes;
 *    opaque data<NFS_MAXDATA>;
 *  }
 */

static void
nfs_dbgsvc_readokres_help( readokres* s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s->data.data_val, s->data.data_len );
  nfs_dbgsvc_fattr_help( &(s->attributes), ldr, ind + 1 );
  #ifdef SURPRESS_DATA
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ),
	   "%s(%uB of data)\n", ldr, s->data.data_len );
  #else
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, opq );
  #endif
  free( ldr );
  free( opq );
}

void
nfs_dbgsvc_readokres( readokres* s, char* leader ){
  nfs_dbgsvc_readokres_help( s, leader, 0 );
}

/*
 *  union readres switch (nfsstat status) {
 *  case NFS_OK:
 *    readokres reply;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_readres_help( readres* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  if( s->status == NFS_OK ){
    nfs_dbgsvc_readokres_help( &(s->readres_u.reply), ldr, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_readres( readres* s, char* leader ){
  nfs_dbgsvc_readres_help( s, leader, 0 );
}

/*
 *  struct writeargs {
 *    nfs_fh   file;
 *    unsigned beginoffset;
 *    unsigned offset;
 *    unsigned totalcount;
 *    opaque data<NFS_MAXDATA>;
 *  };
 */

static void
nfs_dbgsvc_writeargs_help( writeargs* s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s->data.data_val, s->data.data_len );
  nfs_dbgsvc_nfs_fh_help( &(s->file), ldr, ind + 1 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ),
	   "%s(beginoffset): %u (offset): %u (totalcount): %u\n",
	   ldr, s->beginoffset, s->offset, s->totalcount );
  #ifdef SURPRESS_DATA
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(%uB of data)\n", ldr, s->data.data_len );
  #else
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, opq );
  #endif
  free( ldr );
  free( opq );
}

void
nfs_dbgsvc_writeargs( writeargs* s, char* leader ){
  nfs_dbgsvc_writeargs_help( s, leader, 0 );
}

/*
 *  struct createargs {
 *    diropargs where;
 *    sattr attributes;
 *  };
 */

static void
nfs_dbgsvc_createargs_help( createargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_diropargs_help( &(s->where), ldr, ind + 1 );
  nfs_dbgsvc_sattr_help( &(s->attributes), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_createargs( createargs* s, char* leader ){
  nfs_dbgsvc_createargs_help( s, leader, 0 );
}

/*
 *  struct renameargs {
 *    diropargs from;
 *    diropargs to;
 *  };
 */

static void
nfs_dbgsvc_renameargs_help( renameargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_diropargs_help( &(s->from), ldr, ind + 1 );
  nfs_dbgsvc_diropargs_help( &(s->to), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_renameargs( renameargs* s, char* leader ){
  nfs_dbgsvc_renameargs_help( s, leader, 0 );
}

/*
 *  struct linkargs {
 *    nfs_fh from;
 *    diropargs to;
 *   };
 */

static void
nfs_dbgsvc_linkargs_help( linkargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->from), ldr, ind + 1 );
  nfs_dbgsvc_diropargs_help( &(s->to), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_linkargs( linkargs* s, char* leader ){
  nfs_dbgsvc_linkargs_help( s, leader, 0 );
}

/*
 *  struct symlinkargs {
 *    diropargs from;
 *    nfspath to;
 *    sattr attributes;
 *  };
 */

static void
nfs_dbgsvc_symlinkargs_help( symlinkargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_diropargs_help( &(s->from), ldr, ind + 1 );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s\"%s\"\n", ldr, s->to );
  nfs_dbgsvc_sattr_help( &(s->attributes), ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_symlinkargs( symlinkargs* s, char* leader ){
  nfs_dbgsvc_symlinkargs_help( s, leader, 0 );
}

/*
 *  typedef opaque nfscookie[NFS_COOKIESIZE];
 */

static void
nfs_dbgsvc_nfscookie_help( nfscookie s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( s, NFS_COOKIESIZE );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, opq );
  free( ldr );
  free( opq );
}

void
nfs_dbgsvc_nfscookie( nfscookie* s, char* leader ){
  nfs_dbgsvc_nfscookie_help( *s, leader, 0 );
}


/*
 *  struct readdirargs {
 *    nfs_fh dir;
 *    nfscookie cookie;
 *    unsigned count;
 *  };
 */

static void
nfs_dbgsvc_readdirargs_help( readdirargs* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  nfs_dbgsvc_nfs_fh_help( &(s->dir), ldr, ind + 1 );
  nfs_dbgsvc_nfscookie_help( s->cookie, ldr, ind + 1 );
  free( ldr );
}

void
nfs_dbgsvc_readdirargs( readdirargs* s, char* leader ){
  nfs_dbgsvc_readdirargs_help( s, leader, 0 );
}


/*
 *  struct entry {
 *    unsigned fileid;
 *    filename name;
 *    nfscookie cookie;
 *    entry *nextentry;
 *  };
 */

static void
nfs_dbgsvc_entry_help( entry* s, char* leader, unsigned ind ){

  char* ldr;
  char* opq;

  ldr = getLeader( leader, ind, "" );
  opq = printOpaque( &(s->cookie), NFS_COOKIESIZE );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s0x%X %s \"%s\"\n",
	  ldr, s->fileid, opq, s->name );
  free( opq );
  free( ldr );
}

void
nfs_dbgsvc_entry( entry* s, char* leader ){
  nfs_dbgsvc_entry_help( s, leader, 0 );
}

/*
 *  struct dirlist {
 *    entry *entries;
 *    bool eof;
 *  };
 */

static void
nfs_dbgsvc_dirlist_help( dirlist* s, char* leader, unsigned ind ){

  entry* current;
  char* ldr;
  
  ldr = getLeader( leader, ind, "" );

  if( s->eof ){
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%sEOF\n", ldr );
  } else {
    LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s!EOF\n", ldr );
  }
  for( current  = s->entries;
       current != NULL;
       current  = current->nextentry ){
    nfs_dbgsvc_entry_help( current, leader, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_dirlist( dirlist* s, char* leader ){
  nfs_dbgsvc_dirlist_help( s, leader, 0 );
}


/*
 *  union readdirres switch (nfsstat status) {
 *  case NFS_OK:
 *    dirlist reply;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_readdirres_help( readdirres* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  if( s->status == NFS_OK ){
    nfs_dbgsvc_dirlist_help( &(s->readdirres_u.reply), leader, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_readdirres( readdirres* s, char* leader ){
  nfs_dbgsvc_readdirres_help( s, leader, 0 );
}

/*
 *  struct statfsokres {
 *    unsigned tsize;
 *    unsigned bsize;
 *    unsigned blocks;
 *    unsigned bfree;
 *    unsigned bavail;
 *  };
 */

static void
nfs_dbgsvc_statfsokres_help( statfsokres* s, char* leader, unsigned ind ){

  char* ldr;

  ldr = getLeader( leader, ind, "" );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(tsize): %u (bsize): %u (blocks): %u\n",
	  ldr, s->tsize, s->bsize, s->blocks );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s(bfree): %u (bavail): %u\n",
	  ldr, s->bfree, s->bavail );
  free( ldr );
}

void
nfs_dbgsvc_statfsokres( statfsokres* s, char* leader ){
  nfs_dbgsvc_statfsokres_help( s, leader, 0 );
}


/*
 *  union statfsres switch (nfsstat status) {
 *  case NFS_OK:
 *    statfsokres reply;
 *  default:
 *    void;
 *  };
 */

static void
nfs_dbgsvc_statfsres_help( statfsres* s, char* leader, unsigned ind ){

  char* ldr;
  char* ns;

  ldr = getLeader( leader, ind, "" );
  ns = print_nfsstat( s->status );
  LOG_DBG( NFS_SVCDBG_CHECK( 1 ), "%s%s\n", ldr, ns );
  if( s->status == NFS_OK ){
    nfs_dbgsvc_statfsokres_help( &(s->statfsres_u.reply), ldr, ind + 1 );
  }
  free( ldr );
}

void
nfs_dbgsvc_statfsres( statfsres* s, char* leader ){
  nfs_dbgsvc_statfsres_help( s, leader, 0 );
}



