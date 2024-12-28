#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "page.h"
#include "scheduler.h"
#include "task.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t ready_list;
extern List_t terminated_list;

uint8_t task_create(TaskFunc_t func, void *args, PagePolicy_t page_policy,
                    size_t page_size) {
    MICROS_DISABLE_IRQ();

    Page_t page;
    page_alloc(&page, page_policy, page_size);
    if (!page.raw) {
        __enable_irq();
        return 1;
    }

    TCB_t *tcb = tcb_build(&page, func, args);

    if (tcb->TCBNumber == 0) {
        current_tcb = tcb;
    } else {
        list_insert(&ready_list, &(tcb->StateListItem));
    }

    MICROS_ENABLE_IRQ();
    return 0;
}

void task_delete(TCB_t *tcb) {
    if (!tcb || tcb_status(tcb) == TERMINATED)
        return;

    MICROS_DISABLE_IRQ();

    if (!list_valid(&terminated_list))
        list_init(&terminated_list);

    if (tcb_status(tcb) != RUNNING) {
        list_remove(&(tcb->StateListItem));
    }
    list_insert(&terminated_list, &(tcb->StateListItem));

    scheduler_trigger();
    MICROS_ENABLE_IRQ();
}

void task_terminate(void) { task_delete(current_tcb); }

void task_yield(void) { scheduler_trigger(); }
