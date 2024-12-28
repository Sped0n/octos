#include "scheduler.h"
#include "list.h"

#include "Arch/stm32f4xx/Inc/api.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t ready_list;
extern List_t terminated_list;

void scheduler_trigger(void) {
    MICROS_DISABLE_IRQ();
    MICROS_TRIGGER_PENDSV();
    MICROS_ENABLE_IRQ();
}

void scheduler_rr(void) {
    if (tcb_status(current_tcb) == RUNNING) {
        list_insert(&ready_list, &(current_tcb->StateListItem));
    }

    if (list_valid(&terminated_list) && terminated_list.Length > 0) {
        ListItem_t *head = list_head(&terminated_list);
        list_remove(head);
        tcb_release(head->Owner);
    }

    current_tcb = list_head(&ready_list)->Owner;
    list_remove(&(current_tcb->StateListItem));
}
