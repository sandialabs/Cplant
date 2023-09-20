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
**  $Id: yod_site.h,v 1.2 2001/11/02 17:25:30 lafisk Exp $
*/
#include <sys/param.h>
#include <string.h>
#include <malloc.h>

#ifdef TWO_STAGE_COPY
/***********************************************************************
** Maintain up to NNAMEBUFS strings for command building.  All can
** be freed, or they'll be freed as they are reused.
*/
#define NNAMEBUFS  5
static char *name_bufs[NNAMEBUFS];
static int lastbuf = -1;
 
static void
free_names(void)
{
int i;
 
    for (i=0; i<NNAMEBUFS; i++){
        if (name_bufs[i]){
            free(name_bufs[i]);
            name_bufs[i] = NULL;
        }
    }
    lastbuf = -1;
}
/*
** Path name may have %s format specifier, which is to be replaced with
** virtual machine name.  make_name returns pointer to format specifier
** if it has no %s, or new string with %s replaced by vm name otherwise.
*/
static const char *
make_name(const char *format, const char *vm)
{
char buf[MAXPATHLEN]; 
const char *sptr;
int bufnum;
 
    sptr = format;
 
    if (strstr(format, "%s")){
        snprintf(buf, MAXPATHLEN, format, vm);
        bufnum = lastbuf + 1;
        if (bufnum == NNAMEBUFS) bufnum = 0;
        if (name_bufs[bufnum]){
            free(name_bufs[bufnum]);
        }
        name_bufs[bufnum] = (char *)malloc(strlen(buf) + 1);
 
        strcpy(name_bufs[bufnum], buf);
 
        sptr = name_bufs[bufnum];
    }
 
    return sptr;
}
#else
int move_executable(int rank, int timing_data, int job_ID);

#endif

