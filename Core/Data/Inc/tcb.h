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
    ListItem_t EventListItem;
    uint8_t Priority;
    uint8_t BasePriority; /* Priority last assigned to the task */
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

TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args, uint8_t priority);

OCTOS_INLINE inline void tcb_release(TCB_t *tcb) {
    page_free(tcb->Page);
}

OCTOS_INLINE static inline ThreadState_t tcb_status(TCB_t *tcb) {
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

OCTOS_INLINE static inline void tcb_recover_priority(TCB_t *tcb) {
    if (tcb->BasePriority != tcb->Priority) {
        tcb->BasePriority = tcb->Priority;
        tcb->StateListItem.Value = tcb->Priority;
    }
}

#endif
