#ifndef __TASK_H__
#define __TASK_H__

#include "data.h"
#include <stdint.h>

typedef enum thread_state {
  READY,
  RUNNING,
  BLOCKED,
} thread_state_t;

typedef struct TCB {
  uint32_t *SP;
  ListItem_t StateListItem;
  uint8_t id;
  char *name;
  thread_state_t state;
} TCB_t;

uint8_t create_task(void (*task0)(void), void (*task1)(void),
                    void (*task2)(void));
void task_yield(void);
void print_task_status(void);

#endif
