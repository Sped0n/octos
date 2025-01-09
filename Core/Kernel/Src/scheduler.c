#include <stdint.h>

#include "list.h"
#include "scheduler.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t *ready_list;
extern List_t *pending_ready_list;
extern List_t *terminated_list;

void scheduler_rr(void) {
    if (tcb_status(current_tcb) == RUNNING) {
        uint8_t priority = list_item_get_value(&(current_tcb->StateListItem));
        list_item_set_value(&(current_tcb->StateListItem), priority == 0 ? current_tcb->BasePriority : (priority - 1));
        list_insert(ready_list, &(current_tcb->StateListItem));
    }

    if (list_valid(terminated_list) && terminated_list->Length > 0) {
        ListItem_t *tail = list_tail(terminated_list);
        list_remove(tail);
        tcb_release(tail->Owner);
    }

    /* if we have any task in pending ready list, make it current_tcb instantly */
    if (pending_ready_list->Length > 0) {
        ListItem_t *head = list_head(pending_ready_list);
        current_tcb = head->Owner;
        list_remove(head);
    } else {
        current_tcb = list_tail(ready_list)->Owner;
        list_remove(&(current_tcb->StateListItem));
    }
}
