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
**  $Id: yod_site_priv.c,v 1.6 2001/05/18 10:09:41 lafisk Exp $
**
** Arguments:
**      "put"                          "remove"
**      SU number        OR            SU number
**      file name                      file name
**      vmname                         vmname
**      uid
**      gid
**
**  yod requires the ability to do three operations as root:
**
** rsh SSS1 "rcp fixed-path1/file-name SSS0-SU-number:fixed-path2/file-name"
**
** rsh SSS1 "rsh SSS0-SU-number chown uid:gid fixed-path2/file-name"
**
** rsh SSS1 "rsh SSS0-SU-number rm fixed-path2/file-name"
**
** If you modify this file remember it runs with root privilege.  Don't
** make it's interface too general.
*/

#ifdef TWO_STAGE_COPY

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include "yod_site.h"
#include "config.h"

static char cmd[500];

#define PUT    1
#define REMOVE 2

int
main(int argc, char *argv[])
{
char *fname, *vmname;
int op,  rc, sunum;
char suname[80];
const char *mname, *suformat, *vmstorage, *sustorage;
uid_t uid, myuid;
gid_t gid, mygid;

    if (argc < 5){
	printf("%s: Invalid argument list\n",argv[0]);
	exit(-1);
    }

    if ((argv[1][0] == 'p') || (argv[1][0] == 'P')){
	op = PUT;
        if (argc < 7){
	    printf("%s: Invalid argument list\n",argv[0]);
	    exit(-1);
        }
        uid     = atoi(argv[5]);
        gid     = atoi(argv[6]);
    }
    else if ((argv[1][0] == 'r') || (argv[1][0] == 'R')){
	op = REMOVE;
    }
    else{
	printf("%s: Invalid operation %s\n",argv[0],argv[1]);
	exit(-1);
    }
    sunum = atoi(argv[2]);
    fname  = argv[3];
    vmname  = argv[4];

    if ((sunum < 0) || (sunum >= MAX_SU)){
	printf("%s: Invalid SU number %d\n",argv[0],sunum);
	exit(-1);
    }
    suformat = su_name_format();

    sprintf(suname,suformat,sunum);

    /*
    ** Check that the file name is legitimate
    */
    if (strncmp(fname, NAME_PREFIX, strlen(NAME_PREFIX))){ 
	printf("%s: Invalid file name\n",argv[0]);
	exit(-1);
    }
    /*
    ** OK
    */
    myuid = getuid();
    mygid = getgid();

    rc = setuid(0);
    if (!rc) rc = setgid(0);

    if (rc){
	printf("%s: This was supposed to be an SUID program\n",argv[0]);
	exit(-1);
    }
    mname = vm_global_machine();
    vmstorage = vm_global_storage();
    sustorage = su_global_storage();

    if (op == PUT){
	sprintf(cmd,"rsh %s \"rcp %s/%s %s:%s/%s\" ",
		  mname,
		  make_name(vmstorage, vmname), fname, 
		  suname,
	          make_name(sustorage, vmname), fname);

        rc = system(cmd);

	sprintf(cmd,"rsh %s \"rsh %s chown %d:%d %s/%s\" ",
		  mname,
		  suname,
		  uid, gid,
	          make_name(sustorage, vmname), fname);

        rc = system(cmd);
    }
    else{
	sprintf(cmd,"rsh %s \"rsh %s rm %s/%s\" ",
		   mname,
		   suname,
		   make_name(sustorage, vmname), fname);

        rc = system(cmd);

    }
    free_names();

    setuid(myuid);
    setgid(mygid);

    exit(rc);
    return 0;  /*NOTREACHED*/
}
#else
main(){
    return 0;
}
#endif
