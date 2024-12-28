#include <stdint.h>

#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t ready_list;

static uint32_t tcb_id = 0;

TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args) {
    TCB_t *tcb = (TCB_t *) page->raw;
    tcb->Page = page;

    uint32_t *stack_top = &(page->raw[page->size - 16]);
    stack_top[15] = (uint32_t) (1U << 24);// PSR
    stack_top[14] = (uint32_t) func;      // PC
    stack_top[13] = 0;                    // LR
    stack_top[12] = 0;                    // R12
    stack_top[11] = 0;                    // R3
    stack_top[10] = 0;                    // R2
    stack_top[9] = 0;                     // R1
    stack_top[8] = (uint32_t) args;       // R0
    tcb->StackTop = stack_top;

    tcb->TCBNumber = tcb_id++;

    list_item_init(&(tcb->StateListItem));
    tcb->StateListItem.Owner = tcb;

    return tcb;
}

void tcb_release(TCB_t *tcb) {
    page_free(tcb->Page);
}
