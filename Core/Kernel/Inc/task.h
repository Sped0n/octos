#ifndef __TASK_H__
#define __TASK_H__

#include <stddef.h>
#include <stdint.h>

#include "page.h"
#include "tcb.h"


typedef void (*TaskFunc_t)(void *arg);

uint8_t task_create(TaskFunc_t func, void *args, PagePolicy_t page_policy,
                    size_t page_size);
void task_delete(TCB_t *tcb);
void task_terminate(void);
void task_yield(void);

#endif
