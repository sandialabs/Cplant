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
#include <stdlib.h>
#include <srvr_lib.h>
#include <sys_limits.h>
#include <srvr_async.h>

typedef struct _pending_buf {
    struct _pending_buf *next;
    char                *buf;
    ptl_handle_md_t     md;
    ptl_handle_eq_t     eventq;
    int                 num_targets;
    int                 *target_nids;
    int                 *wait_array;
    int                 num_left;
} pending_buf;

pending_buf *head;
int pending_nids[MAX_NODES];

void
free_pending_buf(pending_buf *pb)
{
    int rc;

    free(pb->buf);      
    free(pb->target_nids);
    free(pb->wait_array);

    rc = PtlMDUnlink(pb->md);
    if (rc != PTL_OK) 
        log_error("Error in free_pending_buf(): PtlMDUnlink returned error (rc=%d)", rc);

    rc = PtlEQFree(pb->eventq);
    if (rc != PTL_OK) 
        log_error("Error in free_pending_buf(): PtlEQFree returned error (rc=%d)", rc);

    free(pb);
}

void
init_pending_buf_list()
{
    int i;

    head = NULL;
    for (i = 0; i < MAX_NODES; i++) pending_nids[i] = 0;
}

void
add_buf(char *buf, ptl_handle_md_t md, ptl_handle_eq_t eventq, 
        int num_targets, int *target_nids, int *wait_array)
{
    int i, num_left = 0, target_nid;
    pending_buf *new;

    /* sanity check */
    if ((buf == NULL) || (eventq == PTL_EQ_NONE) || 
        (num_targets <= 0) || (num_targets > MAX_NODES) || 
        (target_nids == NULL) || (wait_array == NULL)) 
    {
        log_error("Error occured in add_buf(): invalid arguments");
    }

    new = malloc(sizeof(pending_buf));
    if (new == NULL) 
        log_error("Error occured in add_buf(): couldn't allocate new pending_buf");

    new->buf         = buf;
    new->md          = md;
    new->eventq      = eventq; 
    new->num_targets = num_targets;
    new->target_nids = target_nids;
    new->wait_array  = wait_array;

    /* calculate num_left and remember target nids we are waiting on */
    for (i = 0; i < num_targets; i++) {
        if (wait_array[i] == 1) {
            ++num_left;

            target_nid = target_nids[i];
  
            /* sanity check */
            if ((target_nid < 0) || (target_nid >= MAX_NODES)) 
                log_error("Error occured in add_buf(): invalid target nid (%d)", target_nid);

            pending_nids[target_nid] += 1;
        }
    }

    if (num_left == 0) 
        log_error("Error occured in add_buf(): num left calculated to be 0");

    new->num_left = num_left;

    /* insert at head of list */
    new->next = head;
    head = new;
}

static void
update_bufs()
{
    int rc, i, nid;
    pending_buf *curr = head;
    ptl_event_t ev;

    while (curr != NULL) {
        while (1) {
            rc = PtlEQGet(curr->eventq, &ev);
            if (rc == PTL_OK) {
                if (ev.type == PTL_EVENT_SENT) {
                    i = (ev.match_bits >> 32);

                    /* sanity checks */
                    if (i < 0) log_error("Error occured in update_bufs(): error in match bits");
                    if (curr->wait_array[i] == 0) {
                        log_error("Error occured in update_bufs(): wait_array[%d] (nid %d) is already zeroed", 
                                  i, curr->target_nids[i]);
                    }
                    curr->wait_array[i] = 0;
                    --(curr->num_left);

                    nid = curr->target_nids[i];

                    /* sanity checks */
                    if (pending_nids[nid] <= 0) {
                        log_error("Error occured in update_bufs(): pending_nids[%d] = %d", nid, pending_nids[nid]); 
                    }

                    pending_nids[nid] -= 1;
                }
            }
            else if (rc == PTL_EQ_EMPTY) break;
            else log_error("Error occured in update_bufs(): unexpected rc (%d) from PtlEQGet()", rc);
        }
	curr = curr->next;
    }
}

void
prune_bufs()
{
    int i;
    pending_buf *curr = head;
    pending_buf *prev = NULL;
    pending_buf *next;

    update_bufs();
    
    while (curr != NULL) {
        if (curr->num_left == 0) {

            /* sanity check */
            for (i = 0; i < curr->num_targets; i++) {
                if (curr->wait_array[i] == 1)
                    log_error("Error in prune_bufs(): curr->wait_aray[%d] (nid %d) = %d", i, curr->target_nids[i], curr->wait_array[i]);
            }

            /* unlink and free the pending buf */
            if (prev == NULL) {
                /* removing head of list */
                head = next = curr->next;
            }
            else {
                /* removing from middle or end of list */
                prev->next = next = curr->next;
            }
            free_pending_buf(curr);
            curr = next;
        }
        else if (curr->num_left < 0) {
            log_error("Error in prune_bufs(): curr->num_left = %d", curr->num_left);
        }
        else {
            prev = curr;
            curr = curr->next; 
        }
    }
}

int
check_nid(int nid)
{
    if (pending_nids[nid] > 2) return 1;
    return 0;
}

void
print_bufs()
{
    int i = 1;
    pending_buf *curr = head;

    log_msg("Pending buf list: ");
    while (curr != NULL) {
        log_msg("  %d: num_left=%d", i, curr->num_left); 
        ++i;
        curr = curr->next;
    }
}

void
print_target_nids()
{
    int i;

    printf("All outstanding target nids:\n");
    for (i = 0; i < MAX_NODES; i++) {
        if (pending_nids[i] > 0) printf("    %d: %d\n", i, pending_nids[i]);
    }
}
