#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "page.h"
#include "tcb.h"

bool task_create(TaskFunc_t func, void *args, uint8_t priority, PagePolicy_t page_policy,
                 size_t page_size);
void task_delete(TCB_t *tcb);
void task_terminate(void);
void task_yield(void);
void task_delay(uint32_t ticks_to_delay);
void task_suspend(void);

#endif
