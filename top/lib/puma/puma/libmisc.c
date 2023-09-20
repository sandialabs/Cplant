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
** $Id: libmisc.c,v 1.14 2001/11/17 01:25:01 lafisk Exp $
*/

#include "puma.h"
#include <string.h>
#include <stdio.h>
#include <sys/param.h>

#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif


/******************************************************************************/
/* Replace every occurence of at least 3 #s within name with an equally long string      */
/* representing number. If number has more digits than there are #s, replace the #s with */
/* the number as long as max_len is not exceeded. Return a pointer to the new            */
/* string.                                                                               */
/* IMPORTANT !!! The function returns a pointer to a static array containing the new     */
/* name. The next time it gets called the array content will be modified.                */

CHAR *
convert_pound(const CHAR *name, INT32 node_no)
{
CHAR *mark;
static CHAR file_name[MAXPATHLEN];
INT32 i;

extern INT32 replace_string(CHAR *source, CHAR **mark, long number);

    if ( (name == NULL) || (node_no < 0) || (strlen(name) >= MAXPATHLEN)) {
        return NULL;
    }

    for (i = 0; i < MAXPATHLEN; i++) {
    file_name[i] = '\0';
    }
    sprintf(file_name, "%s", name);

    for (mark = file_name; *mark != '\0';) {
        if ((mark = strstr(mark, "###")) == NULL) {
            break;
        }
        if (! replace_string(file_name, &mark, node_no)) {
            /*printf("Can not convert file name\n");*/
            return NULL;
        }
    }

    return file_name;

}

/******************************************************************************/
/* *mark is pointing to the beginning of a string of #s within source. Find out how */
/* long this string is and replace it with a string representing number.            */
/* Advance *mark to point to the end of the number substring within source.         */
/* Return 1 if successful, 0 otherwise.                                             */

INT32 
replace_string(CHAR *source, CHAR **mark, long number)
{
INT32 i, number_len;
CHAR *cursor, work_buf[MAXPATHLEN];

    sprintf(work_buf, "%ld", number);
    number_len = strlen(work_buf);

    for (i = 0, cursor = *mark; *cursor == '#'; cursor++, i++);

    if (i >= number_len) {
        strcpy(work_buf, *mark + i);
        sprintf(*mark, "%*.*ld%s", i, i, number, work_buf);
        *mark = *mark + i;
    } else {
        if ((strlen(source) - i + number_len) > MAXPATHLEN - 1) {
            /*printf("New file name would exceed MAXPATHLEN\n");*/
            return 0;
        }
        strcpy(work_buf, *mark + i);
        sprintf(*mark, "%*.*ld%s", number_len, number_len, number, work_buf);
        *mark = *mark + number_len;
    }
    return 1;
}

#include <sys/time.h>
#include <unistd.h>
 
double 
dclock(void)
{
struct timeval tv;
double tm;
 
    gettimeofday(&tv, NULL);
 
    tm = (double)(((double)tv.tv_usec / 1000000.0) + tv.tv_sec);

    return tm;
}
 
/* dclock() FORTRAN equivalent */
double
dclock_(void)
{
    return dclock();
}
/*
** For packing up a list of strings (like an argv list).
** Each string is NULL terminated, and the whole thing
** is NULL terminated.  So the last string is followed
** by two NULL bytes.
*/
int
pack_string(char **ustr, int maxu, char *pstr, int maxp)
{
int i;
char *p2;
int lp, lnew;

    if (!ustr){
        if (maxp < 2) return -1;

        pstr[0] = pstr[1] = 0;
        return 2;
    }

    p2 = pstr;
    lp = 0;

    i = 0;

    while ((i < maxu) && ustr[i]){
    
        lnew = strlen(ustr[i]) + 1;

        if  (lp + lnew >= maxp){
           return -1;
        }

        strcpy(p2, (const char *)ustr[i]);
    
        lp += lnew;

        p2 += lnew;

        i++;
    }

    *(pstr + lp) = '\0';

    return(lp + 1);
}

