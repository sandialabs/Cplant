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
** $Id: jobArgs.c,v 1.1 2001/11/17 01:23:08 lafisk Exp $
**
** For pnames:
**   Here you list the executable name for each command line.
**   If pname[0] is a NULL pointer, yod will spawn the same executable
**   as the parent's executable for each command line.  If nlines is
**   greater than 1, and pname[0] is not a NULL pointer, and the second
**   char pointer in the pnames array is a NULL pointer, we'll use
**   the pname[0] name for the program name for each command line.
**
** For nnodes and nprocs:
**   If either of these are NULL pointers, we'll assume 1 node
**   and/or 1 process per node for each command line.  If they are
**   not NULL, but the second value is -1, we'll repeat
**   nnodes[0] and/or nprocs[0] for each command line.
**
** For argvs:
**   If this is a NULL pointer, we assume no arguments for any
**   command line.  If argv[i] is NULL, we assume no arguments for
**   that command line.  As is typical, the last pointer in the
**   argv[i] array should be a NULL pointer.
*/

#include <stdlib.h>
#include <string.h>
#include "puma.h"
#include "sys_limits.h"
#include "cplant.h"

typedef struct _bufType{
  char *start;
  char *end;
  char *currptr;
  int  len;
} bufType;

static int increment   = 2*1024;

static int
initBuf(bufType *buf, int len)
{
    buf->start = calloc(1, len);

    if (!buf->start) return -1;

    buf->end = buf->start + len;
    buf->len = len;
    buf->currptr = buf->start;

    return 0;
}
static int
verifyBuf(bufType *buf, int need)
{
int remaining, add;

    remaining = (int)(buf->end - buf->currptr);

    if (remaining >= need) return 0;

    add = MAX(need, increment);

    buf->len += add;

    buf->start = realloc(buf->start, buf->len);

    if (!buf->start) return -1;

    buf->end = buf->start + buf->len;

    buf->currptr = buf->end - remaining;

    return 0;
}
static int
charStarStarLen(char **a, int maxa)
{
int len, i;

    /*
    ** char** lists are packed up with each string null
    ** terminated, and extra null byte at the end of
    ** the list of strings
    */

    if (!a) return 2;

    len = 0;

    for (i=0; ((i < maxa) && a[i]) ;i++){

       len += strlen(a[i]) + 1;
    }

    len += 1;

    return len;
}
static int
charStarStarUnroll(char ***buf, char **list)
{
char *c, **mybuf;
int i, nitems;

    c = *list;
    nitems = 0;
    
    while (*c){
        while (*(++c));
        nitems++;
        c++;
    }

    mybuf = (char **)calloc(nitems+1, sizeof(char *));

    if (!mybuf) return -1;

    c=*list;
    
    for (i=0; i<nitems; i++){

        mybuf[i] = strdup(c);

        if (!mybuf[i]) goto FAILURE2;
    
        while(*c++);
    }
    mybuf[nitems] = NULL;

    *buf = mybuf;

    *list = ++c;   /* char after last NULL byte */

     return 0;

FAILURE2:

    if (mybuf){
        for (i=0; i<nitems; i++){
           if (mybuf[i]) free(mybuf[i]);
        }
        free(mybuf);
    }
    return -1;
}
void
freeNodeRequest(ndrequest *nd)
{
int i, ii;

    if (nd->globalList)   free(nd->globalList);
    if (nd->localSizes)   free(nd->localSizes);
    if (nd->procsPerNode) free(nd->procsPerNode);

    if (nd->localLists){
        for (i=0; i<nd->nmembers; i++){
            if (nd->localLists[i]) free(nd->localLists[i]);
        }
        free(nd->localLists);
    }
    if (nd->pnames){
        for (i=0; i<nd->nmembers; i++){
            if (nd->pnames[i]) free(nd->pnames[i]);
        }
        free(nd->pnames);
    }
    if (nd->pargs){
        for (i=0; i<nd->nmembers; i++){
            if (nd->pargs[i]){

                ii = 0;

                while (nd->pargs[i][ii]){
                    free(nd->pargs[i][ii]);
                    ii++;
                }

                free(nd->pargs[i]);
            }
        }
        free(nd->pargs);
    }
    free(nd);
}

int
packNodeRequest(int nlines, char **pnames, char ***argvs,
            int *nnodes, int *nprocs, char **req, int *reqlen)
{
int i, len, twoInts, allInts;
int *nums;
bufType reqbuf;

    if (nlines <= 0) return -1;

    if (initBuf(&reqbuf, increment)) goto FAILURE;

    allInts = nlines * sizeof(int);
    twoInts =      2 * sizeof(int);

    nums = (int *)(reqbuf.start);

    nums[0] = nlines;    /* number of command lines */
    reqbuf.currptr += sizeof(int);

    nums = (int *)(reqbuf.currptr);

    /*
    ** number of nodes requested by each command line
    */
    if (!nnodes){
        nums[0] = 1;
        nums[1] = -1;
        reqbuf.currptr += twoInts;
    }
    else if (nnodes[1] == -1){
        nums[0] = nnodes[0];
        nums[1] = -1;
        reqbuf.currptr += twoInts;
    }
    else{
        if (verifyBuf(&reqbuf, allInts)) goto FAILURE;
        memcpy((void *)nums, (void *)nnodes, allInts);
        reqbuf.currptr += allInts;
    }

    /*
    ** number of processes per node for each command line
    */
    nums = (int *)reqbuf.currptr;

    if (!nprocs){
        if (verifyBuf(&reqbuf, twoInts)) goto FAILURE;
        nums[0] = 1;
        nums[1] = -1;
        reqbuf.currptr += twoInts;
    }
    else if (nprocs[1] == -1){
        if (verifyBuf(&reqbuf, twoInts)) goto FAILURE;
        nums[0] = nprocs[0];
        nums[1] = -1;
        reqbuf.currptr += twoInts;
    }
    else{
        if (verifyBuf(&reqbuf, allInts)) goto FAILURE;
        memcpy((void *)nums, (void *)nprocs, allInts);
        reqbuf.currptr += allInts;
    }

    /*
    ** program names
    */

    if ((pnames == NULL) || (pnames[0] == NULL)){
        if (verifyBuf(&reqbuf, 2)) goto FAILURE;
        *reqbuf.currptr++ = 0;
        *reqbuf.currptr++ = 0;
    }
    else if ((nlines == 1) || !pnames[1]){
        len = strlen(pnames[0]);
        if (verifyBuf(&reqbuf, len + 2)) goto FAILURE;
        strcpy(reqbuf.currptr, pnames[0]);
        reqbuf.currptr +=  (len + 1);
        *reqbuf.currptr++ = 0;
    }
    else{
        len = charStarStarLen(pnames, nlines);

        if (len < 0){
            goto FAILURE;
        }

        if (verifyBuf(&reqbuf, len)) goto FAILURE;

        pack_string(pnames, nlines, reqbuf.currptr, len);

        reqbuf.currptr += len;
    }

    /*
    ** program arguments
    */
    if (!argvs){
        if (verifyBuf(&reqbuf, 2)) goto FAILURE;
        *reqbuf.currptr++ = 0;
        *reqbuf.currptr++ = 0;
    }
    else{
        for (i=0; i<nlines; i++){

            if ((argvs[i] == NULL) || (argvs[i][0] == NULL)){
                if (verifyBuf(&reqbuf, 2)) goto FAILURE;
                *reqbuf.currptr++ = 0;
                *reqbuf.currptr++ = 0;
            }
            else{
                len = charStarStarLen(argvs[i], MAX_ARGV);

                if (len < 0){
                    goto FAILURE;
                }
        
                if (verifyBuf(&reqbuf, len)) goto FAILURE;
        
                pack_string(argvs[i], MAX_ARGV, reqbuf.currptr, len);

                reqbuf.currptr += len;
            }
        }
    }
    if (verifyBuf(&reqbuf, 1)) goto FAILURE;

    *reqbuf.currptr++ = 0;

    *req    = reqbuf.start;
    *reqlen = (int)(reqbuf.currptr - reqbuf.start);

    return 0;

FAILURE:
    if (reqbuf.start) free(reqbuf.start);
    return -1;
}

ndrequest *
unpackNodeRequest(char *req, int reqlen, char *parentName)
{
int *num, i, sameVal, rc, len;
char *currptr, *parentString, *c;
ndrequest *ndr;

    ndr = (ndrequest *)calloc(1, sizeof(ndrequest));

    if (!ndr) return NULL;

    num = (int *)req;

    ndr->globalSize = 0;
    ndr->globalList = NULL;
    ndr->localLists = NULL;

    ndr->nmembers = *num;

    if (ndr->nmembers <= 0){

         goto FAILURE3;
    }

    ndr->localSizes   = (int *)malloc(sizeof(int)        * ndr->nmembers);
    ndr->procsPerNode = (int *)malloc(sizeof(int)        * ndr->nmembers);
    ndr->pargs        = (char ***)calloc(ndr->nmembers, sizeof(char **));

    if (!ndr->localSizes || !ndr->procsPerNode || !ndr->pargs){

        goto FAILURE3;
    }

    num++;                                       /* number of nodes */

    sameVal = ((num[1] == -1) ? num[0] : 0);   

    for (i=0; i<ndr->nmembers; i++){

        ndr->localSizes[i] = (sameVal ? sameVal : num[i]);
        
    }

    if (sameVal) num += 2;                     /* number of procs per node */
    else         num += ndr->nmembers;

    sameVal = ((num[1] == -1) ? num[0] : 0);

    for (i=0; i<ndr->nmembers; i++){

        ndr->procsPerNode[i] = (sameVal ? sameVal : num[i]);
        
    }

    if (sameVal) num += 2;                       /* program names */
    else         num += ndr->nmembers;

    currptr = (char *)num;

    if (! *currptr){
        if (!parentName){
            goto FAILURE3;
        }

        len = strlen(parentName);
        parentString = (char *)malloc(len + 2);

        if (!parentString) goto FAILURE3;

        strcpy(parentString, parentName);
        parentString[len+1] = 0;

        currptr += 2;

        c = parentString;
        
        rc = charStarStarUnroll(&(ndr->pnames), &c);

        free(parentString);
    }
    else{
        rc = charStarStarUnroll(&(ndr->pnames), &currptr);
    }

    if (rc){
        goto FAILURE3;
    }
  
    if (!currptr[0] && !currptr[1] && !currptr[2]){   /* program arguments */
        free(ndr->pargs);        
        ndr->pargs = NULL;
    }

    else{
        for (i=0; i<ndr->nmembers; i++){

            if (!currptr[0] && !currptr[1]){

                 currptr += 2;
                 ndr->pargs[i] = NULL;
                 continue;
            }

            rc = charStarStarUnroll(&(ndr->pargs[i]), &currptr);

            if (rc) goto FAILURE3;
        }
    }

    return ndr;

FAILURE3:

    freeNodeRequest(ndr);

    return NULL;
}
/**
*** debugging stuff
**/
#include <ctype.h>

void
displayPackedRequest(char *buf, int len)
{
int i;

    for (i=0; i<len; i++){

        if (i && (i%25==0)){
             printf("\n");
        }
        if (isalnum(buf[i]) || isspace(buf[i])) printf("%c",buf[i]);
        else                                    printf("<%d>",buf[i]);
    }
    printf("\n");
}
void
displayNodeRequest(ndrequest* nd)
{
int i, ii, sizes, nprocs, lists, args;

    sizes  = (nd->localSizes != NULL);
    nprocs = (nd->procsPerNode != NULL);
    lists  = (nd->localLists != NULL);
    args   = (nd->pargs!= NULL);

    if (nd->globalSize > 0){
        printf("Size: %d\n",nd->globalSize);
    }
    if (nd->globalProcsPerNode > 0){
        printf("Number procs/node: %d\n",nd->globalProcsPerNode);
    }
    if (nd->globalList){
        printf("List: %s\n",nd->globalList);
    }
    for (i=0; i<nd->nmembers; i++){
        printf("Member %d:\n",i+1);

        if (sizes && (nd->localSizes[i] > 0)) printf("    size: %d\n",nd->localSizes[i]);
        if (nprocs && (nd->procsPerNode[i] > 0)) printf("    procs/node: %d\n",nd->procsPerNode[i]);
        if (lists && nd->localLists[i]) printf("    list: %s\n",nd->localLists[i]);

        if (i == 0){
            printf("    program: %s\n",nd->pnames[0]);
        }
        else if (nd->pnames[1]){
            printf("    program: %s\n",nd->pnames[i]);
        }
        else{
            printf("    program: Same as first member\n");
        }

        if (args && nd->pargs[i]){
            printf("    arguments: ");
            for (ii=0; nd->pargs[i][ii]; ii++){
                printf("%s ", nd->pargs[i][ii]);
            }
            printf("\n");
        }
    }
    printf("\n");
}

