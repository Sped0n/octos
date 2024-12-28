#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "kernel.h"
#include "list.h"
#include "page.h"
#include "scheduler.h"
#include "task.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t ready_list;
extern List_t pending_ready_list;
extern List_t delayed_list;
extern List_t delayed_list_overflow;
extern List_t suspended_list;
extern List_t terminated_list;

uint8_t task_create(TaskFunc_t func, void *args, PagePolicy_t page_policy,
                    size_t page_size) {
    ENTER_CRITICAL();

    Page_t page;
    page_alloc(&page, page_policy, page_size);
    if (!page.raw) {
        EXIT_CRITICAL();
        return 1;
    }

    TCB_t *tcb = tcb_build(&page, func, args);

    if (tcb->TCBNumber == 0) {
        current_tcb = tcb;
        list_init(&ready_list);
    } else {
        list_insert(&ready_list, &(tcb->StateListItem));
    }

    EXIT_CRITICAL();
    return 0;
}

void task_delete(TCB_t *tcb) {
    if (!tcb || tcb_status(tcb) == TERMINATED)
        return;

    ENTER_CRITICAL();

    if (tcb_status(tcb) != RUNNING) {
        list_remove(&(tcb->StateListItem));
    }
    list_insert(&terminated_list, &(tcb->StateListItem));

    scheduler_trigger();
    EXIT_CRITICAL();
}

void task_terminate(void) { task_delete(current_tcb); }

void task_yield(void) { scheduler_trigger(); }

void task_delay(uint32_t ticks_to_delay) {
    const uint32_t current_tick = kernel_get_tick();
    const uint32_t tick_to_wake = current_tick + ticks_to_delay;

    list_item_set_value(&(current_tcb->StateListItem), tick_to_wake);

    if (tick_to_wake < current_tick) {
        // Overflow will occur, add to overflow list
        list_insert(&delayed_list_overflow, &(current_tcb->StateListItem));
    } else {
        // Add to normal delayed list
        list_insert(&delayed_list, &(current_tcb->StateListItem));
    }
    // Switch task
    task_yield();
}
