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
** $Id: nodeList.c,v 1.4 2002/01/18 23:58:16 pumatst Exp $
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "sys_limits.h"
#include "idtypes.h"

/***********************************************************************
** parse a pingd/yod node list
************************************************************************
*
** Node list is separated by commas, node identifier can be a
** node number or a node number range, which is 2 node numbers
** separated by dots.  No white space allowed.
**
**   list:   nid[,nid]*
**
**   nid:    number              (physical node number)
**           number.[.]*number   (node number range, at least one dot)
**
**   example:   1,4,7,12..20,100...200
**
** Function returns number of nodes specified.  node_list contains
** node numbers listed in the order they appear in the list string.
** Order may be significant when we have heterogeneous compute nodes.
**
** Warning: If a node number appears more than once in the list, only
** the first occurence is kept in the node list.
**
** Return: Number of entries in list (may be zero if list is empty),
** or -1 (in case of error).
*/
static int tmpNodeList[MAX_NODES];

int
parse_node_list(char *list, int *node_list,
                int size,     /* number of ints allocated in node_list */
                int min,      /* minimum valid node number */
                int max)      /* maximum valid node number */
{
char *lptr, *nodenums;
int from, to, i, lcount, maxnodes;
 
    maxnodes = max - min + 1;

    /* empty list */
    if ( !list ) {
      return 0;
    }
 
    if (!node_list || (size==0)){
        size = MAX_NODES;
        node_list = tmpNodeList;
    }
 
    if ((maxnodes <= 0) || (size <= 0))
        return -1;

    for (i=0; list[i]; i++){
	if (!isdigit(list[i]) && (list[i] != '.') && (list[i] != ',')){
	    printf("Invalid node characters in node list\n");
	    return -1;
        }
    }
 
    nodenums = (char *)calloc(maxnodes, 1);
 
    if (!nodenums){
        printf("memory allocation error in parse_node_list\n");
        return -1;
    }
 
    lptr = list;
    lcount = 0;
 
    while (1){
 
        from = atoi(lptr);
 
        if ((from < min) || (from > max)){
            printf("Invalid node numbers in node list\n");
            return -1;
        }
 
        while (isdigit(*lptr)) lptr++;
 
        if ( *lptr == '.' ){
            while (*lptr == '.') lptr++;
            to = atoi(lptr);
            if ((to < min) || (to > max)){
                printf("Invalid node numbers in node list\n");
                return -1;
            }
            while (isdigit(*lptr)) lptr++;
        }
        else{
            to = from;
        }
        if (from <= to){
            for (i=from; i<=to; i++){
 
                if (nodenums[i-min] == 0){
		    if (lcount <= size){
			node_list[lcount++] = i;
		    }
		    nodenums[i-min] = 1;
		}
            }
        }
        else{
            for (i=from; i>=to; i--){
 
                if (nodenums[i-min] == 0){
		    if (lcount <= size){
			node_list[lcount++] = i;
		    }
		    nodenums[i-min] = 1;
		}
            }
        }
        while (*lptr == ',') lptr++;
 
        if (*lptr == 0) break;
    }
    free(nodenums);
 
    if (lcount > size){
	 printf("Ran out of space in node list array\n");
         return -1;
    }
 
    return lcount;
}
/*
** Return position of number in list, -1 if not in list
*/

static int findNodeList[MAX_NODES];

int
findInList(char *list, int nodenum)
{
int i, listlen;

    listlen = parse_node_list(list, findNodeList, MAX_NODES, 0, MAX_NODES);

    if (listlen <= 0){
        return -1;
    }

    for (i=0; i<listlen; i++){
        if (findNodeList[i] == nodenum){
            return i;
        }
    }

    return -1;
}

int
simpleNodeRange(int *list, int size)
{
int i, a, b, simple=1;
 
    if (size > 1){
 
        a = list[0];  b = list[size-1];
 
        if (a < b){
            if ( (b - a + 1) != size){
                simple = 0;
            }
            else
                for (i=1; i<size-1; i++){
                    if (list[i] != ++a){
                       simple = 0;
                       break;
                    }
                }
        }
        else{
            if ( (a - b + 1) != size){
                simple = 0;
            }
            else
                for (i=size-2; i>0; i++){
                    if (list[i] != --b){
                       simple = 0;
                       break;
                    }
                }
        }
    }
    return simple;
}
/*
** Write out every node in a node list of length "size", starting at 
** left margin "lmargin", and for a width of "width".
*/
int
print_node_list(FILE *fp, int *nids, int size, int width, int lmargin)
{
int i, n, buflen;
char *c, *cend, *buf;

    buflen = (size * 5) + 10;   /* should be enough */
    buf = malloc(buflen); 
    if (!buf){
	return -1;
    }
    buf[0] = 0;
    c = buf;
    n = 0;

    while (c <= (buf + buflen - 10)){

	c = buf + strlen(buf);
	sprintf(c, "%d", nids[n]);

	if (n == (size-1)) break;

	if (nids[n+1] == (nids[n] + 1)){
	 
	    while (nids[n+1] == (nids[n] + 1)){
		n++;
		if (n == (size-1)) break;
	    }
	    
	    c = buf + strlen(buf);
	    sprintf(c,"..%d",nids[n]);
	}
	else if (nids[n+1] == (nids[n] - 1)){

	    while (nids[n+1] == (nids[n] - 1)){
		n++;
		if (n == (size-1)) break;
	    }
	    
	    c = buf + strlen(buf);
	    sprintf(c,"..%d",nids[n]);
	}
	if (n == (size-1)) break;

	strcat(buf,",");
	n++;
    }

    c = buf;
    cend = buf+strlen(buf);

    while (c < cend){
	for (i=0; i<lmargin; i++){
	    fprintf(fp," ");
	}
	
	for (i=0; (i<width) && (c<cend); i++){
	    fprintf(fp,"%c",*c++);
	}
	if ( (c < cend) && isdigit(*(c-1)) && (isdigit(*c))){
	    while (isdigit(*c)){
		fprintf(fp,"%c",*c++);
	    }
	}
	if ( (c < cend) && (*c == ',')){
	    fprintf(fp,"%c",*c++);
	}
	fprintf(fp,"\n");
    }
    free(buf);

    return 0;
}
/*
** Write out every node in a node list of length "size", in the
** buffer given.
*/
int
write_node_list(nid_type *nids, int size, char *buf, int len)
{
int n;
char *c;

    buf[0] = 0;
    c = buf;
    n = 0;

    while (c <= (buf + len - 10)){

	c = buf + strlen(buf);
	sprintf(c, "%d", nids[n]);

	if (n == (size-1)) break;

	if (nids[n+1] == (nids[n] + 1)){
	 
	    while (nids[n+1] == (nids[n] + 1)){
		n++;
		if (n == (size-1)) break;
	    }
	    
	    c = buf + strlen(buf);
	    sprintf(c,"..%d",nids[n]);
	}
	else if (nids[n+1] == (nids[n] - 1)){

	    while (nids[n+1] == (nids[n] - 1)){
		n++;
		if (n == (size-1)) break;
	    }
	    
	    c = buf + strlen(buf);
	    sprintf(c,"..%d",nids[n]);
	}
	if (n == (size-1)) break;

	strcat(buf,",");
	n++;
    }
    return 0;
}
