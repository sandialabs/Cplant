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
/* $Id: protocol_switch.c,v 1.30 2001/02/16 05:29:51 lafisk Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "protocol_switch.h"

const char *io_uris[LAST_IO_PROTO];
io_ops_t *io_ops[LAST_IO_PROTO];

char newname[MAXPATHLEN];
char newname2[MAXPATHLEN];
char *GL_fname;
char *GL_fname2;

#define PATH_IS_FYOD( S )  strncmp( (S) , "/raid_", 6) == 0 && \
	isdigit( S[6] ) && isdigit( S[7] ) && isdigit( S[8] ) && \
	S[9] == '/'

int uri_check(const char *path, char* newpath);

int path2io_proto(const char* full_path ) 
{
    int proto;

    /* uri check */
    proto = uri_check(full_path,newname);
    if ( proto >= 0 ) {
      GL_fname = newname;
      return proto;
    }

    if ( PATH_IS_FYOD(full_path) ) {
      GL_fname = (char*) full_path;
      return FYOD_IO_PROTO;
    }

    /* default behavior */
    GL_fname = (char*) full_path;
    return YOD_IO_PROTO;
}

int path2io_proto2(const char* path1, const char* path2 ) 
{
    int proto1=-1, proto2=-1;

    proto1 = uri_check(path1,newname);
    if ( proto1 >= 0 ) {
      proto2 = uri_check(path2,newname2);
      if ( proto1 == proto2 ) {
        GL_fname = newname;
        GL_fname2 = newname2;
        return proto1;
      }
      else {
        GL_fname = (char*) path1;
        GL_fname2 = (char*) path2;
        return YOD_IO_PROTO;
      }
    }

    if ( PATH_IS_FYOD(path1) ) {
      if ( PATH_IS_FYOD(path2) ) {
        GL_fname  = (char*) path1;
        GL_fname2 = (char*) path2;
        return FYOD_IO_PROTO;
      }
      else {
        GL_fname = (char*) path1;
        GL_fname2 = (char*) path2;
        return YOD_IO_PROTO;
      }
    }

    GL_fname = (char*) path1;
    GL_fname2 = (char*) path2;
    return YOD_IO_PROTO;
}


int fd2io_proto ( int fd )
{
  if ( !validFd(fd)){
    /*
    ** if fd is bad, we want the yod func to return 
    ** the proper error value.
    */
    return YOD_IO_PROTO;
  }
  return FD_ENTRY_PROTOCOL(fd);
}

int register_io_proto(int protocol, io_ops_t *ioops, const char* uri)
{
  if ( protocol >= LAST_IO_PROTO ) {
    return -1;
  }
  io_ops[protocol] = ioops; 
  io_uris[protocol] = uri;

  return 0;
}

void io_proto_init(void) 
{
  io_uris[FYOD_IO_PROTO]   = "fyod:";
  io_uris[YOD_IO_PROTO]    = "yod:";
  io_uris[ENFS_IO_PROTO]   = "enfs:";
  io_uris[DUMMY_IO_PROTO]  = "dummy:";
  io_uris[DUMMY1_IO_PROTO] = "dummy1:";
  io_uris[DUMMY2_IO_PROTO] = "dummy2:";

  register_io_proto(FYOD_IO_PROTO,  &io_ops_yod,   io_uris[FYOD_IO_PROTO]);
  register_io_proto(YOD_IO_PROTO,   &io_ops_yod,   io_uris[YOD_IO_PROTO]);
  register_io_proto(DUMMY_IO_PROTO, &io_ops_dummy, io_uris[DUMMY_IO_PROTO]);
  register_io_proto(ENFS_IO_PROTO,  &io_ops_enfs,  io_uris[ENFS_IO_PROTO]);
}

int uri_check(const char *path, char* newpath) {
    int i, j, k, plen, slen, proto=-1;
    BOOLEAN colon;
    char* fp;

    plen = strlen(path);

    fp = (char*) path;
    for (i=0; i<plen; i++) {
       if ( fp[i] == ':' ) {
         colon = TRUE;
         break;
       }
    }
    if ( colon == TRUE ) { /* back off to the preceding slash */
      for (j=i-1; j>=0; j--) {
        if ( fp[j] == '/' ) {
          break;
        }
      }
      /* now j points "in front of" the protocol name;
         bump it up
      */
      j++;

      for (k=0; k<LAST_IO_PROTO; k++) {
        slen = strlen(io_uris[k]);
        if ( strncmp(fp+j, io_uris[k], slen) == 0 ) {
          memcpy(newpath, fp+j+slen, plen-j-slen+2);
          proto = k;
        }
      }
#if 0
      if (strncmp(fp+j, "enfs:", 5) == 0) {
        memcpy(newpath, fp+j+5, plen -j -4 +1);
        proto = ENFS_IO_PROTO;
      }
#endif
    }
    return proto;
}
