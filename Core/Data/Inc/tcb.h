#ifndef __TCB_H__
#define __TCB_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "page.h"

typedef void (*TaskFunc_t)(void *args);

typedef enum ThreadState {
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    TERMINATED
} ThreadState_t;

typedef struct TCB {
    uint32_t *StackTop;
    Page_t *Page;
    ListItem_t StateListItem;
    uint32_t TCBNumber;
} TCB_t;

extern TCB_t *current_tcb;
extern List_t *ready_list;
extern List_t *pending_ready_list;
extern List_t *delayed_list;
extern List_t *delayed_list_overflow;
extern List_t *suspended_list;
extern List_t *terminated_list;

static uint32_t tcb_id;

MICROS_INLINE static inline TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args) {
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

MICROS_INLINE inline void tcb_release(TCB_t *tcb) {
    page_free(tcb->Page);
}

MICROS_INLINE static inline ThreadState_t tcb_status(TCB_t *tcb) {
    const List_t *parent = tcb->StateListItem.Parent;
    if (parent == NULL) return RUNNING;
    else if (parent == terminated_list)
        return TERMINATED;
    else if (parent == delayed_list || parent == delayed_list_overflow)
        return BLOCKED;// sleeping
    else if (parent == ready_list || parent == pending_ready_list)
        return READY;
    else if (parent == suspended_list)
        return SUSPENDED;
    else
        return BLOCKED;// mutex/semaphore
}

#endif
