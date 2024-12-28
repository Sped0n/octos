#include <stddef.h>
#include <stdint.h>

#include "cmsis_gcc.h"
#include "list.h"
#include "page.h"
#include "scheduler.h"
#include "task.h"

#include "stm32f4xx.h"// IWYU pragma: keep

#define STACK_SIZE 512

TCB_t *current_tcb;
List_t ready_list;
List_t terminated_list;

static uint8_t next_id = 0;

static void task_stack_init(TCB_t *tcb, TaskFunc_t func, void *arg) {
    uint32_t *stack_top = &tcb->Page.raw[tcb->Page.size - 16];

    // Initialize stack
    stack_top[15] = (uint32_t) (1U << 24);// PSR
    stack_top[14] = (uint32_t) func;      // PC
    stack_top[13] = 0;                    // LR
    stack_top[12] = 0;                    // R12
    stack_top[11] = 0;                    // R3
    stack_top[10] = 0;                    // R2
    stack_top[9] = 0;                     // R1
    stack_top[8] = (uint32_t) arg;        // R0

    tcb->SP = stack_top;
}

uint8_t task_create(TaskFunc_t func, void *arg, PagePolicy_t page_policy,
                    size_t page_size) {

    __disable_irq();

    Page_t page;
    page_alloc(&page, page_policy, page_size);
    if (!page.raw) {
        __enable_irq();
        return 1;
    }

    TCB_t *tcb = (TCB_t *) page.raw;
    tcb->Page = page;
    tcb->ID = next_id++;
    task_stack_init(tcb, func, arg);
    list_item_init(&(tcb->StateListItem));
    tcb->StateListItem.Owner = tcb;

    if (!list_valid(&ready_list))
        list_init(&ready_list);

    if (tcb->ID == 0) {
        tcb->State = RUNNING;
        current_tcb = tcb;
    } else {
        tcb->State = READY;
        list_insert(&ready_list, &(tcb->StateListItem));
    }

    __enable_irq();
    return 0;
}

void task_delete(TCB_t *tcb) {
    if (!tcb || tcb->State == TERMINATED)
        return;

    __disable_irq();

    if (!list_valid(&terminated_list))
        list_init(&terminated_list);

    if (tcb->State != RUNNING) {
        list_remove(&(tcb->StateListItem));
    }
    list_insert(&terminated_list, &(tcb->StateListItem));
    tcb->State = TERMINATED;

    scheduler_trigger();
    __enable_irq();
}

void task_terminate(void) { task_delete(current_tcb); }

void task_yield(void) { scheduler_trigger(); }
