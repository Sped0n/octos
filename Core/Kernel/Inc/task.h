#ifndef __TASK_H__
#define __TASK_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "page.h"

typedef enum ThreadState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ThreadState_t;

typedef struct TCB {
    uint32_t *SP;
    uint8_t ID;
    ThreadState_t State;
    ListItem_t StateListItem;
    Page_t Page;
} TCB_t;

typedef void (*TaskFunc_t)(void *arg);

uint8_t task_create(TaskFunc_t func, void *arg, PagePolicy_t page_policy,
                    size_t page_size);
void task_delete(TCB_t *tcb);
void task_terminate(void);
void task_yield(void);

#endif
