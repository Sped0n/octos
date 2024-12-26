#include "task.h"
#include "data.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NUM_OF_THREADS 3
#define STACKSIZE 2048

TCB_t tcbs[NUM_OF_THREADS];
TCB_t *current_tcb;
List_t ready_list;

uint32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

static void task_stack_init(int i) {
  /* Stack Pointer */
  tcbs[i].SP = &TCB_STACK[i][STACKSIZE - 16];
  /* PSR thumb mode */
  TCB_STACK[i][STACKSIZE - 1] = (1U << 24);
}

uint8_t create_task(void (*task0)(void), void (*task1)(void),
                    void (*task2)(void)) {
  __disable_irq();

  list_init(&ready_list);

  task_stack_init(0);
  TCB_STACK[0][STACKSIZE - 2] = (uint32_t)(task0); // Program Counter
  list_item_init(&(tcbs[0].StateListItem));
  tcbs[0].StateListItem.Owner = &tcbs[0];
  list_insert(&ready_list, &(tcbs[0].StateListItem));

  task_stack_init(1);
  TCB_STACK[1][STACKSIZE - 2] = (uint32_t)(task1); // Program Counter
  list_item_init(&(tcbs[1].StateListItem));
  tcbs[1].StateListItem.Owner = &tcbs[1];
  list_insert(&ready_list, &(tcbs[1].StateListItem));

  task_stack_init(2);
  TCB_STACK[2][STACKSIZE - 2] = (uint32_t)(task2); // Program Counter
  list_item_init(&(tcbs[2].StateListItem));
  tcbs[2].StateListItem.Owner = &tcbs[2];
  list_insert(&ready_list, &(tcbs[2].StateListItem));

  current_tcb = &tcbs[0];
  current_tcb->state = RUNNING;
  list_remove(&(current_tcb->StateListItem));

  __enable_irq();

  return 1;
}

void print_task_status(void) {
  printf("Task Status:\n\r");
  for (int i = 0; i < NUM_OF_THREADS; i++) {
    printf("Task %d: State=%d\n\r", i, tcbs[i].state);
  }
}

void task_yield(void) {
  /* Trigger PendSV */
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
