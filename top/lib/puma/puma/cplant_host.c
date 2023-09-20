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
/* $Id: cplant_host.c,v 1.23.4.1 2002/06/10 16:24:08 jrstear Exp $ cplant_host.c - Reads /cplant/cplant-host file */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "puma.h"
#include "cplant_host.h"
#include "fyod_map.h"
#include "config.h"

#define HOSTNAME "/proc/sys/kernel/hostname"

#define MAX_STRING 256

#define ASCII_DASH '-'
#define ASCII_DOT '.'
#define ASCII_SPACE 0x20
#define ASCII_TAB 0x09
#define ASCII_OCTOTHORPE '#'
#define ASCII_LF 0x0a
#define ASCII_NULL 0x0

#define EOS 0
#define COMMENT ASCII_OCTOTHORPE

/*
 * Hack an error to avoid having to link with sfyod.
 */
static void
error(const char *fname, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", fname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ".\n");
    exit(1);
}
static void
warning(const char *fname, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", fname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ".\n");
    return;
}

/*
 * Go to next field.
 * Return begining of next field or NULL if EOS is reached.
 */
static char *cplantHost_nextField(char *line)
{
/* Find first white space */
    while ( *line != ASCII_SPACE && *line != ASCII_TAB )
	{
/* Look for End-of-String */
		if ( *line == ASCII_LF )
			return NULL;

        line++;
	}

/* At this point, we KNOW line is pointing to white space (space or tab) */

/* Find first non-white space */
    while ( *line == ASCII_SPACE || *line == ASCII_TAB )
        line++;

/* Make sure we are not at a comment or EOS */
	if ( *line == COMMENT || *line == ASCII_LF )
		return NULL;

    return line;
}

/*
 * Look for specified character within current item.
 * Item is defined as group of non-white characters with white space or EOS
 * at the end.
 * If specified character is found, a pointer to that character is returned.
 * Otherwise, a NULL pointer is returned.
 */
char *cplantHost_itemChr(char *cp, char target)
{
	while ( *cp != target )
	{
/* Look for white space or EOS */
		if ( *cp == ASCII_SPACE || *cp == ASCII_TAB || *cp == ASCII_LF )
			return NULL;

		cp++;
	}

	return cp;
}

/* Return the hostname of the current node */
int cplantHost_hostname(char *hostname, int length)
{
	static char fname[] = "cplantHost_hostname";
	struct stat statbuf;
	int fd, status, rdlen;

	status = OK;
	rdlen = 0;

	fd = open(HOSTNAME, O_RDONLY);

	if ( fd < 0 ){
		warning(fname, "Cannot open %s", HOSTNAME);
		status = -1;
        }
	else if (fstat( fd, &statbuf) != 0){
		warning(fname, "Cannot stat %s", HOSTNAME);
		status = -1;
	}
	else{

	    rdlen = statbuf.st_size;
	    if (rdlen >= length) rdlen = length - 1;

	    if ( read(fd, hostname, rdlen) < 0 ){
	        warning(fname, "Cannot read % bytes of %s", 
		                rdlen, HOSTNAME);
		status = -1;
		rdlen = 0;
            }
	}

	if (fd >= 0) close(fd);

	hostname[rdlen] = 0;

	return status;
}

#if 0
/*
 * Return the SU of the current node.
 * Assumes the node name is c-n.SU-nn
 */
static int cplantHost_SU(void)
{
	static char fname[] = "cplantHost_SU";
	char hostname[MAX_STRING];
	static int my_su = -1;
	char *cp;
	char *eptr;

/* Get su of this node once */
	if ( my_su >= 0 )
		return my_su;

	if ( cplantHost_hostname(hostname, MAX_STRING) != OK )
		error(fname, "cplantHost_hostname() failed");

/* Find first dash */
	cp = cplantHost_itemChr(hostname, ASCII_DASH);
	if ( cp == NULL )
		error(fname, "Invalid hostname: %s", hostname);

/* Find second dash */
	cp++;
	cp = cplantHost_itemChr(cp, ASCII_DASH);
	if ( cp == NULL )
		error(fname, "Invalid hostname: %s", hostname);

/* Go to SU number */
	cp++;

/* Convert number */
	my_su = strtol(cp, &eptr, 10);
	if ( eptr == cp || *eptr != ASCII_DOT )
		error(fname, "Bad SU number: %s", cp);

	return my_su;
}
#endif

/*
 * Return the nid of the specified node.
 * Assumes the node name is c-n.SU-nn or c-n
 */
static int cplantHost_nameToNid(char *hostname)
{
  char *eptr;
  int nid;

/* Check for physical node id -- don't accept host names 
   anymore (5/10/00)
*/

  nid = strtol( hostname, &eptr, 10 );
  if( eptr != hostname ) {
    return nid;
  }
  else {
    return -1;
  }
}

/* getFyodUnits() reads the Fyod map and records unit numbers whose
                 associated nid field match _my_pnid 
   getFyodMap() reads in and stores the entire Fyod map 
 
   the format of lines in the Fyod map file is:

   fyod XY nid

   where X is a 2-digit number in the range [00,99]
         Y is a 1-digit number in the range [0,1]
   and 
         nid is a Cplant node id

   the idea is that there are 100 possible nodes running Fyod, each 
   having 2 possible disks, although the scheme can be easily
   extended to more Fyods w/ more disks.

   the unit number XY is used in file paths and in the Fyod map file;
   Cplant applications always translate this string to in integer
   (X*2+Y) and work with the integer value.
*/
int getFyodUnits( int *map ) 
{
   /* on successful return map[0] or map[1] may
      be set equal ind(X0) and map[1] may be set
      equal to ind(X1), where ind(XY) = 2*X+Y. this
      is the mapping f: chars->ints of the unit
      number.

      map[0] and map[1] should be initialized to -1
      before calling getFyodUnit()
   */

   FILE *fp;
   char line[MAX_STRING];
   char *cp;
   int unit, raid, subraid, nid;
   const char *hostFile;

   hostFile = cplant_host_file();

   if (!hostFile){
       error("getFyodUnits","no cplant-host file");
       return -1;
   }

   fp = fopen(hostFile, "r");
   if ( fp == NULL ) {
     return -1;
   }

   while ( TRUE ) {
     while( TRUE ) {
       if ( fgets(line, MAX_STRING, fp) == NULL ) {
         fclose(fp);
         return 0;
       }
       if ( strncmp(line, "fyod", 4) == 0 ) {
         break;
       }
     }
     cp = line;
     cp = cplantHost_nextField(cp);
     unit = atoi(cp);

     cp = cplantHost_nextField(cp);
     nid = atoi(cp);

   /* CHECK FOR BAD raid, subraid, nid !!!!!!!!!!!!!
      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   */

     raid = unit / FYOD_FACTOR; 
     subraid = unit % MAX_DISKS_PER_FYOD_NODE;

     /* bad map file */
     if (raid < 0 || raid > MAX_FYOD_NODES-1) {
       fclose(fp);
       return -1;
     }

     /* bad map file */
     if (subraid < 0 || subraid > MAX_DISKS_PER_FYOD_NODE) {
       fclose(fp);
       return -1;
     }

     if (nid == _my_pnid) {
       map[subraid] = MAX_DISKS_PER_FYOD_NODE*raid+subraid;
     }
   }
   fclose(fp);
}

int getFyodMap(int* map) 
{

   /* on entry the array map[FYOD_MAP_SZ] should be initialized
      to -1. the nid ass. w/ the unit number XY in the map
      file gets stored in map[ind(XY)], ind: chars->ints =
      MAX_DISKS_PER_FYOD_NODE*X+Y.
   */

   FILE *fp;
   char line[MAX_STRING];
   char *cp;
   int unit, raid, subraid, nid;
   const char *hostFile;

   hostFile = cplant_host_file();

   if (!hostFile){
       error("getFyodMap", "Can't determine cplant-host location");
       return -1;
   }

   fp = fopen(hostFile, "r");
   if ( fp == NULL ) {
     return -1;
   }

   while ( TRUE ) {
     while ( TRUE ) {
       if ( fgets(line, MAX_STRING, fp) == NULL ) {
         fclose(fp);
         return 0;
       }
       if ( strncmp(line, "fyod", 4) == 0 ) {
         break;
       }
     }
     cp = line;
     cp = cplantHost_nextField(cp);
     unit = atoi(cp);
     cp = cplantHost_nextField(cp);
     nid = atoi(cp);

     raid = unit / FYOD_FACTOR;
     subraid = unit % MAX_DISKS_PER_FYOD_NODE;

   /* CHECK FOR BAD unit, nid !!!!!!!!!!!!!
      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   */

     if (raid < 0 || raid > MAX_FYOD_NODES-1) {
       fclose(fp);
       return -1;
     }

     if (subraid < 0 || subraid > MAX_DISKS_PER_FYOD_NODE) {
       fclose(fp);
       return -1;
     }
/*   map[unit]           = nid; */
     map[raid*MAX_DISKS_PER_FYOD_NODE+subraid] = nid;

     printf("getFyodMap: map[%d] = %d\n", raid*MAX_DISKS_PER_FYOD_NODE+subraid, nid);
   }
   fclose(fp);
}


/*
 * Read specified entry in cplant-host file and return the nids of the node
 * names in the file.
 * 'length' is the size of the given 'list' array.
 * 'list' is a given array in which the nids are placed.
 * 'count' returns the actual number of nodes.
 * Only 'length' nids are returned.  All others are thrown away.
 */
INT32 cplantHost_getNid(const char *entry, int length, int *list, int *count)
{
	static char fname[] = "cplantHost_getNid";
	FILE *fp;
	int nodes;
	int entryLen;
	char line[MAX_STRING];
	char *cp;
	int nid;
        const char *hostFile;

        hostFile = cplant_host_file();

	if (!hostFile){
	    warning(fname, "Can't file cplant-host file");
	    return ERROR;
	}

/* Open cplant-host file */
	fp = fopen(hostFile, "r");
	if ( fp == NULL ){
		error(fname, "%s missing", hostFile);
        }

	entryLen = strlen(entry);

	while ( TRUE )
	{
		if ( fgets(line, MAX_STRING, fp) == NULL ){
                    warning(fname, "Missing entry: %s", entry); 
		    fclose(fp);
		    return ERROR;
                }

		if ( strncmp(line, entry, entryLen) == 0 )
			break;
	}

	fclose(fp);

	cp = line;
	nodes = 0;

/* Get the next field */
	while (( cp = cplantHost_nextField(cp) ))
	{
#if 0
		vdebug_level(8, s, cp);
#endif
		nid = cplantHost_nameToNid(cp);
		if ( nid < 0 ){
		    warning(fname, "Invalid node name: %s", cp);
		    return ERROR;
                }

/* Save nid in array if long enough */
		if ( nodes < length )
			*list++ = nid;

		nodes++;
	}

	*count = nodes;

	return OK;
}
