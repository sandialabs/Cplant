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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include "puma.h"
#include "sys_limits.h"
#include "bebopd.h"
#include "bebopd_private.h"

#include <time.h>

extern int Dbglevel;
#ifdef REMAP
extern nid_type carray[];
extern int marray[];
extern int minMap;
extern int maxMap;
extern int Remap;
extern int minPct, maxPct;
extern char RemapFile[];
#endif

typedef enum{
  DEFAULT_ALLOCATOR,
  NOT_TOO_DUMB_ALLOCATOR,
  FIRST_FIT_ALLOCATOR,   
  BEST_FIT_ALLOCATOR,
  BEST_FIT_ALLOCATOR_MOD,
  SUM_OF_SQUARES_ALLOCATOR,
  LAST_ALLOCATOR
} bebopd_allocation_type;

/* typedef struct {
  nid_type loc;
  int range;
  void * next;
} _llist;
*/

typedef struct {
  nid_type loc;
  int range;
} _rangetype;

static char debugmsg[MAX_NODES*6];
bebopd_allocation_type  allocType = BEST_FIT_ALLOCATOR_MOD;
bebopd_allocation_type testing = LAST_ALLOCATOR;
/*
** $Id: bebopd_smart_alloc.c,v 1.9.2.9 2002/10/04 16:28:26 jrstear Exp $
**
**    Plug in your own smart allocator here.  The bebopd will
**    call this with the list of free nodes and the number of
**    nodes required by the job.  Write the allocated nodes
**    to the caller's "go" buffer.
*/

int
smart_alloc(nid_type *avail, /* list of free nodes (physical node IDs) */
        int len,        /* number of free nodes on the list */
	    int want,       /* number of free nodes wanted      */
	    nid_type *go)   /* list of allocated nodes          */
{
  int found;
  int count, where, begin, beginReal, binsize, bestbinsize, numranges;
  int numbinofsize[len+1], wherelocated[len+1];
  int i, j, sum, min, temp, beststart, bestsum, overflow;
  _rangetype * rangelist;
  //log_msg("i want %d nodes but there are %d nodes", want, len);


  LOCATION("smart_alloc","top");
  if (want == len) {
   memcpy((void *)go, (void*)avail, sizeof(nid_type) * want);
   return 0;
  }

  if (want > len){
    log_msg("Insufficient nodes available. Requested : %d Available : %d", 
		want, len);
    return -1;
  } // End if (want > len)

  if ((allocType < 0) ||(allocType >= LAST_ALLOCATOR)){
  log_msg("Invalid allocation type : %d",allocType);
  return -1;
  } // End if ((allocType < 0) ||(allocType >= LAST_ALLOCATOR))

  if (allocType == DEFAULT_ALLOCATOR){
    memcpy((void *)go, (void *)avail, sizeof(nid_type) * want);    
  } // End if (allocType == DEFAULT_ALLOCATOR)



  if (allocType == FIRST_FIT_ALLOCATOR){
    LOCATION("smart_alloc","first fit allocator");
    found = 0;
    where =0;
    count=1;
    begin=0;

    log_msg("in first fit");

    if(want==1)
      found = 1;
    
    while ((found ==0)&&(where<len)){ //see if there is a fit
      where++;
      if(avail[where-1]==(avail[where]-1)){
	count++;
      } // End if(avail[where-1]==(avail[where]-1))
      else{
	    count=1;
	    begin = where;
      } // End else for if(avail[where-1]==(avail[where]-1))

      if(count == want)
	    found=1;
    } // End while ((found ==0)&&(where<len)){ //see if there is a fit

    //    log_msg("found is %d begin is %d", found, begin);

    if(found==1){ //if there is a location big enough
      memcpy((void *)go, (void *)(avail + begin), sizeof(nid_type)*want); 
                 //need to look at how to tell it where to start
    } // End if(found==1)
   
    else{ //allocate randomly
      memcpy((void *)go, (void *)avail, sizeof(nid_type) * want);   
    } // End else for if(found==1)

  } // if (allocType == FIRST_FIT_ALLOCATOR)

  //BEST_FIT

  if((allocType==BEST_FIT_ALLOCATOR) || (allocType==BEST_FIT_ALLOCATOR_MOD)
      || (allocType==FIRST_FIT_ALLOCATOR)){
    LOCATION("smart_alloc","best fit allocator");
    count=1;
    begin=0;
    beginReal=0;
    bestbinsize=len + 1;
    numranges=1;

    for(where = 1; where<len; where++){
#ifdef REMAP
      if ( (Remap && (marray[avail[where-1]]==(marray[avail[where]]-1))) || 
         ( !Remap && (avail[where-1]==(avail[where]-1)) ) ) {
#else
      if(avail[where-1]==(avail[where]-1)){
#endif
	    count++;
      } // end if(avail[where-1]==(avail[where]-1)){
      else{
        numranges++;
            if ((allocType==FIRST_FIT_ALLOCATOR) && (count >= want)
                 && (bestbinsize==len+1)) {
              beginReal=begin;
              bestbinsize = count-want;
              break;
            }
            else if (((count-want) >= 0) && ((count-want)< bestbinsize)){
	      beginReal=begin;
	      bestbinsize = count-want;
	    } // end if((((count-want) >= 0) && ((count-want)< bestbinsize)))
	    count=1;
	    begin = where;
	  } // end of else for if(avail[where-1]==(avail[where]-1))
    } // end for(where = 1; where<len; where++)
    /* Special case if the last block is contiguous to make sure it doesn't get missed */
    if (((count - want) >= 0) && ((count - want) < bestbinsize)) {
      beginReal=begin;
      bestbinsize = count-want;
    }

    if (bestbinsize  == len+1) { 
      if (allocType==BEST_FIT_ALLOCATOR) {
        memcpy((void *)go, (void *)avail, sizeof(nid_type) * want);
      } // End if (allocType==BEST_FIT_ALLOCATOR)
      else {
/* Create sorted list of node ranges, something that has both the start */
/*  of the node range, and the size. Then do multiple memcopies. */


        rangelist = malloc(sizeof(_rangetype)*numranges);
        where=1;
        overflow=len+1;
/* This is the first pass, for making the list */
        for ( i = 0; i < numranges; i++) {
          count=1;
          rangelist[i].loc=where-1;
#ifdef REMAP
          while (( Remap && (where < len) && (marray[avail[where-1]]==(marray[avail[where]]-1))) || ( !Remap  && ((where < len) && (avail[where-1]==(avail[where]-1)))) ) {
#else
          while ((where < len) && (avail[where-1]==(avail[where]-1))) {
#endif
            count++;
            where++;
          } // while ((where < len) && (avail[where-1]==(avail[where]-1))
          rangelist[i].range=count;
          where++;
        } // End for ( i = 0; i < numranges; i++)
/* This is the second pass for finding the optimal allocation */
        beststart=0;
        bestsum=MAX_NODES+1;
        for (i = 0; i < numranges; i++) {
          count=0;
          sum=0;
          for ( j = i; j < numranges; j++ ) {
            count+=rangelist[j].range;
              /* Once we've found as many as we  need, stop */
              if (count >= want) break; 
            } // for ( j = i; j < numranges; j++ )
            /* If we didn't find enough, going any further won't turn up more nodes */
            if ( count < want ) break; 
#ifdef REMAP
            if (Remap){                     
              sum=(marray[avail[rangelist[j].loc]]-marray[avail[rangelist[i].loc]])+rangelist[j].range-(count-want);
            }
#endif
            else {
              sum=(avail[rangelist[j].loc]-avail[rangelist[i].loc])+rangelist[j].range-(count-want);
#ifdef REMAP
            }
#endif
          if ((sum < bestsum) || ((sum==bestsum) && (count-want)<overflow )) {
            bestsum=sum;
            beststart=i;
            overflow=count-want;
          } // End if ( sum < bestsum )
        } // End for (i = 0; i < numranges; i++) 
        binsize = want;
        count = 0;
#if 0
#ifdef REMAP
        if (Remap) {
          for ( i = beststart; i < numranges+beststart; i++) {
            memcpy((void *) (go + count), (void *)(avail + rangelist[i%numranges].loc),
              sizeof(nid_type)*(binsize > rangelist[i%numranges].range ?
              rangelist[i%numranges].range : binsize));
            count += rangelist[i%numranges].range;
            binsize-= rangelist[i%numranges].range;
            if (count >= want) break;
          } // End for ( i = beststart; i < numranges+beststart; i++ )
        }
        else {
#endif
          for ( i = beststart; i < numranges; i++ ) {
            memcpy((void *) (go + count), (void *)(avail + rangelist[i].loc),
              sizeof(nid_type)*(binsize > rangelist[i].range ? 
              rangelist[i].range : binsize));
            count += rangelist[i].range;
            binsize-= rangelist[i].range;
            if (count >= want) break;
          } // End for ( i = beststart; i < numranges; i++ )
#ifdef REMAP
        }
#endif
#endif
        memcpy((void *) go,(void *)(avail + rangelist[beststart].loc),
          sizeof(nid_type)*want);
        free(rangelist);
          
      } /* End : else for allocType==BEST_FIT_ALLOCATOR */
    } /* End : if (bestbinsize  == len+1) */
    else {
      memcpy((void *)go, (void *)(avail + beginReal), sizeof(nid_type)*want);
    }
    
    if (Dbglevel > 0) {
      j=0;
      for (i = 0; i < len; i++) {
        temp=sprintf(&debugmsg[j]," %d",avail[i]);
        j+=temp;
      }
      log_msg("Available Nodes :%s\n",debugmsg);
      j=0;
      for (i = 0; i < want; i++) {
        temp=sprintf(&debugmsg[j]," %d",go[i]);
        j+=temp;
      }
      log_msg("Selected Nodes:%s\n",debugmsg);
#ifdef REMAP
      if (Remap) {
        j=0;
        for (i = 0; i < len; i++) {
          temp=sprintf(&debugmsg[j]," %d",marray[avail[i]]);
          j+=temp;
        }
        log_msg("Available Nodes(remap) :%s\n",debugmsg);
        j=0;
        for (i = 0; i < want; i++) {
          temp=sprintf(&debugmsg[j]," %d",marray[go[i]]);
          j+=temp;
        }
        log_msg("Selected Nodes(remap):%s\n",debugmsg);
      }
#endif
    }
    return 0;

  } // End else for if((allocType==BEST_FIT_ALLOCATOR) || (allocType==BEST_FIT_ALLOCATOR_MOD))

  /*SUM OF SQUARES*/

  if(allocType==SUM_OF_SQUARES_ALLOCATOR){
    
    count=1;
    begin=0;
    beginReal = 0;
    found = 0;
    sum =0;
    
    log_msg("in sos");
    
    for(i = 0; i<len+1; i++){
      numbinofsize[i]=0; 
      wherelocated[i]=0;
    }
    

    for(where = 1; where<len; where++){
#ifdef REMAP
      if(( Remap &&marray[avail[where-1]]==(marray[avail[where]]-1))|| ( !Remap  && (avail[where-1]==(avail[where]-1)))){
#else
      if(avail[where-1]==(avail[where]-1)){
#endif
	count++;
	//log_msg(" supposedly %d %d at %d", avail[where-1], avail[where], where);
      }

      else{
	
	numbinofsize[count]++;
	wherelocated[count]=begin;
	
	count=1;
	begin = where;
      }
      
    }
    
    numbinofsize[count]++;
    wherelocated[count]=begin;
    
    log_msg("out of for loop");	
    //    log_msg("the best bin size is %d and it starts at %d", bestbinsize, beginReal);
    

    /*calculate the sum of squares*/
    for(i=1; i<len +1; i++)
      sum = sum + numbinofsize[i]*numbinofsize[i];


    /*FIND THE CORRECT BIN */

    min=sum*sum;

    log_msg("here1");
    
    for(i=want; i< len+1; i++){
      if(numbinofsize[i] !=0){
	temp = sum- numbinofsize[i]*numbinofsize[i] + (numbinofsize[i]-1)*(numbinofsize[i]-1);
	if ((i-want) != 0)
	  temp = temp + numbinofsize[i-want]*numbinofsize[i-want];
	if (temp<min){
	  min = temp;
	  beginReal = wherelocated[i];
	  found = 1;
	}
      }
    }

    log_msg("here?");
    
    if(found  == 0){ //allocate randomly-no bin found
      log_msg("random allocation");
     
      memcpy((void *)go, (void *)avail, sizeof(nid_type) * want);
    }
    else{   
   
      log_msg("allocating from node %d", beginReal); 
      memcpy((void *)go, (void *)(avail + beginReal), sizeof(nid_type)*want);
    }
  }



  if (allocType == NOT_TOO_DUMB_ALLOCATOR){
    if (want >= 64){
      memcpy((void *) go, (void *)(avail + len - want +1), sizeof(int)*want);
    }
    else {
      memcpy((void *)go, (void *)avail, sizeof(int) * want);
    }
  } 
  return 0;
}

#ifdef REMAP
int remap() {

  FILE * fp;
  int i,nid,loc,rc,fsize;
  struct stat statbuf;

  for (i=0; i < MAX_NODES; i++) {
    carray[i]=-1;
    marray[i]=-1;
  }
 
  rc = stat(RemapFile,&statbuf);
  if (rc==0) {
    fsize = statbuf.st_size;
  }
  
  if (rc < 0) {
    printf("Could not find file %s. Remap disabled.\n",RemapFile);
    log_msg("Could not find file %s. Remap disabled.\n",RemapFile);
    Remap=FALSE;
    return -1;
  }

  if (fsize == 0) {
    printf("Remap File %s has no entries. Remap disabled.\n",RemapFile);
    log_msg("Remap File %s has no entries. Remap disabled.\n",RemapFile);
    Remap=FALSE;
    return -1;
  }
 
  fp=fopen(RemapFile,"r");
  rc=1;
  maxMap=0;
  minMap=MAX_NODES;
  while (rc != EOF) {
    rc=fscanf(fp,"%d %d",&nid,&loc);
    if (rc >=0) {
      if ((loc >= MAX_NODES) || ( nid >= MAX_NODES)) {
        printf("Invalid Remap file, %s. Remap disabled.\n",RemapFile);
        log_msg("Invalid Remap file, %s. Remap disabled.\n",RemapFile);
        Remap=FALSE;
        break;
      }
      if (loc < minMap) minMap=loc;
      if (loc > maxMap) maxMap=loc;
      carray[loc]=nid;
      marray[nid]=loc;
    }
    else {
      break;
    }
    if (maxMap < minMap) {
      printf("Invalid Remap file, %s. Remap disabled.\n",RemapFile);
      log_msg("Invalid Remap file, %s. Remap disabled.\n",RemapFile);
      Remap=FALSE;
    }
  }
  fclose(fp);
  return rc;
}

#endif
