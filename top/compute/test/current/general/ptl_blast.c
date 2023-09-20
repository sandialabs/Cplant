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
#include "stdio.h"
#include "stdlib.h"
#include "puma.h"
#include "myrnal.h"
#include "p30.h"

/* quick and dirty test that PtlPut()s to a target node as fast as possible */

int main(int argc, char **argv)
{
    int rc, i=0;
    int target_nid;
    char *test_msg = "this is a test of the emergency broadcasting system";

    ptl_handle_ni_t ni_h;
    ptl_process_id_t id;
    ptl_id_t group_size;
    ptl_handle_md_t md_h; 
    ptl_md_t md;
    ptl_process_id_t tid;

    if (argc != 2) {
        printf("You must specify a nid to blast on the command line\n");
        exit(EXIT_FAILURE);
    }
    target_nid = atoi(argv[1]);

    rc = PtlNIInit(PTL_IFACE_MYR, MYRNAL_MAX_PTL_SIZE, MYRNAL_MAX_ACL_SIZE, &ni_h); 
    if (rc != PTL_OK) {
        printf("Error calling PtlNIInit()\n");
        exit(EXIT_FAILURE);
    }

    rc = PtlGetId(ni_h, &id, &group_size);
    if (rc != PTL_OK) {
        printf("Error calling PtlGetId()\n");
        exit(EXIT_FAILURE);
    }

    md.start = test_msg;
    md.length = strlen(test_msg);
    md.threshold = PTL_MD_THRESH_INF;
    md.options = 0;
    md.user_ptr = NULL;
    md.eventq = PTL_EQ_NONE;
    rc = PtlMDBind(ni_h, md, &md_h);
    if (rc != PTL_OK) {
        printf("Error calling PtlMDBind()\n");
        exit(EXIT_FAILURE);
    }

    tid.nid = target_nid;
    tid.pid = 100; 
    tid.addr_kind = PTL_ADDR_NID;
    
    printf("%d: Blasting %d...\n", id.rid, target_nid);
    while (1) {
        i++;
        if (i%1000 == 0) printf("%d: %d\n", id.rid, i);

        rc = PtlPut(md_h, PTL_NOACK_REQ, tid, 0, 1, 0, 0, 0);
        if (rc != PTL_OK) {
            if (rc == PTL_FAIL) {
                printf("Error calling PtlPut(): send queue is full.\n");
            }
            else {
                printf("Error calling PtlPut()\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return EXIT_SUCCESS;
}
