#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "scheduler.h"
#include "task.h"

#include "stm32f4xx.h" // IWYU pragma: keep

#define STACK_SIZE 512

TCB_t *current_tcb;
List_t ready_list;

static uint8_t next_id = 0;

static void task_stack_init(TCB_t *tcb, task_func_t func, void *arg) {
  uint32_t *stack_top = &tcb->page.raw[tcb->page.size - 16];

  // Initialize stack
  stack_top[15] = (uint32_t)(1U << 24); // PSR
  stack_top[14] = (uint32_t)func;       // PC
  stack_top[13] = 0;                    // LR
  stack_top[12] = 0;                    // R12
  stack_top[11] = 0;                    // R3
  stack_top[10] = 0;                    // R2
  stack_top[9] = 0;                     // R1
  stack_top[8] = (uint32_t)arg;         // R0

  tcb->SP = stack_top;
}

uint8_t task_create(task_func_t func, void *arg, page_policy_t page_policy) {

  __disable_irq();

  TCB_t *tcb = malloc(sizeof(TCB_t));
  if (!tcb) {
    __enable_irq();
    return 1;
  }

  // Allocate stack
  page_alloc(&(tcb->page), page_policy, STACK_SIZE);
  if (!tcb->page.raw) {
    free(tcb);
    __enable_irq();
    return 1;
  }

  tcb->id = next_id++;

  task_stack_init(tcb, func, arg);
  list_item_init(&(tcb->StateListItem));
  tcb->StateListItem.Owner = tcb;

  if (!ready_list.Current) {
    list_init(&ready_list);
  }

  if (tcb->id == 0) {
    tcb->state = RUNNING;
    current_tcb = tcb;
  } else {
    tcb->state = READY;
    list_insert(&ready_list, &(tcb->StateListItem));
  }

  __enable_irq();
  return 0;
}

void task_yield(void) { scheduler_trigger(); }
