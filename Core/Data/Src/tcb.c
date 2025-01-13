#include <stdint.h>

#include "list.h"
#include "page.h"
#include "tcb.h"

static uint32_t tcb_id = 0;

/**
  * @brief Create and initialize a new Thread Control Block
  * @param page Memory page to use for the thread
  * @param func Function pointer to the thread's task
  * @param args Arguments to pass to the thread's task
  * @param priority Initial priority for the thread
  * @retval TCB_t* Pointer to the created Thread Control Block
  */
TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args, uint8_t priority) {
    TCB_t *tcb = (TCB_t *) page->raw;

    tcb->Page.raw = page->raw;
    tcb->Page.size = page->size;
    tcb->Page.policy = page->policy;

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
    tcb->RootPriority = priority;
    tcb->Priority = priority;
    list_item_init(&(tcb->StateListItem));
    list_item_init(&(tcb->EventListItem));
    list_item_set_value(&(tcb->StateListItem), priority);
    list_item_set_value(&(tcb->StateListItem), priority);
    tcb->StateListItem.Owner = tcb;
    tcb->EventListItem.Owner = tcb;

    return tcb;
}
