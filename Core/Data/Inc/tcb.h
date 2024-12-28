#ifndef __TCB_H__
#define __TCB_H__

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

TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args);
void tcb_release(TCB_t *tcb);
ThreadState_t tcb_status(TCB_t *tcb);

#endif
