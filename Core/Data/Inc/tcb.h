#ifndef __TCB_H__
#define __TCB_H__

#include <stddef.h>
#include <stdint.h>

#include "attr.h"
#include "list.h"
#include "page.h"

/**
  * @brief Function pointer type for tasks that can be executed by the scheduler
  * @param args Pointer to the task's arguments
  */
typedef void (*TaskFunc_t)(void *args);

/**
  * @brief Thread states enumeration
  */
typedef enum ThreadState {
    READY,     /*!< Thread is ready to run */
    RUNNING,   /*!< Thread is currently running */
    BLOCKED,   /*!< Thread is blocked (sleeping or waiting for resource) */
    SUSPENDED, /*!< Thread is suspended */
    TERMINATED /*!< Thread has terminated */
} ThreadState_t;

/**
  * @brief Thread Control Block structure definition
  */
typedef struct TCB {
    uint32_t *StackTop;       /*!< Pointer to the top of thread's stack */
    Page_t *Page;             /*!< Memory page allocated for this thread */
    ListItem_t StateListItem; /*!< List item for thread state lists */
    ListItem_t EventListItem; /*!< List item for event waiting lists */
    uint8_t RootPriority;     /*!< Original priority of the thread */
    uint8_t Priority;         /*!< Current priority of the thread */
    uint32_t TCBNumber;       /*!< Unique identifier for the thread */
} TCB_t;

extern TCB_t *current_tcb;
extern List_t *ready_list;
extern List_t *pending_ready_list;
extern List_t *delayed_list;
extern List_t *delayed_list_overflow;
extern List_t *suspended_list;
extern List_t *terminated_list;

TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args, uint8_t priority);
void tcb_release(TCB_t *tcb);

/**
  * @brief Get the current state of a thread
  * @param tcb Pointer to the Thread Control Block
  * @retval ThreadState_t Current state of the thread
  */
OCTOS_INLINE static inline ThreadState_t tcb_status(TCB_t *tcb) {
    const List_t *parent = tcb->StateListItem.Parent;
    if (parent == NULL) return RUNNING;
    else if (parent == terminated_list)
        return TERMINATED;
    else if (parent == delayed_list || parent == delayed_list_overflow)
        return BLOCKED;
    else if (parent == ready_list || parent == pending_ready_list)
        return READY;
    else if (parent == suspended_list)
        return SUSPENDED;
    else
        return BLOCKED;
}

/**
  * @brief Restore thread's priority to its original value
  * @param tcb Pointer to the Thread Control Block
  * @retval None
  */
OCTOS_INLINE static inline void tcb_recover_priority(TCB_t *tcb) {
    if (tcb->Priority != tcb->RootPriority) {
        tcb->Priority = tcb->RootPriority;
        tcb->StateListItem.Value = tcb->RootPriority;
    }
}

#endif
