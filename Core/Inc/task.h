#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>

typedef struct TCB {
  uint32_t *stack_pointer;
  struct TCB *next;
} TCB;

uint8_t create_task(void (*task0)(void), void (*task1)(void),
                    void (*task2)(void));
void task_yield(void);

#endif
