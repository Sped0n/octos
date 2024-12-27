#ifndef __TASK_H__
#define __TASK_H__

#include "list.h"
#include "page.h"
#include <stdint.h>

typedef enum thread_state {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} thread_state_t;

typedef struct TCB {
    uint32_t *SP;
    uint8_t id;
    thread_state_t state;
    ListItem_t StateListItem;
    page_t page;
} TCB_t;

typedef void (*task_func_t)(void *arg);

uint8_t task_create(task_func_t func, void *arg, page_policy_t page_policy,
                    uint16_t page_size);
void task_delete(TCB_t *tcb);
void task_terminate(void);
void task_yield(void);

#endif
