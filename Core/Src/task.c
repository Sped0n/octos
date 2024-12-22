#include "task.h"
#include "stm32f4xx.h" // IWYU pragma: keep

#define NUM_OF_THREADS 3
#define STACKSIZE 200

TCB tcbs[NUM_OF_THREADS];
TCB *current_tcb_pointer;

uint32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

static void task_stack_init(int i) {
  /* Stack Pointer */
  tcbs[i].stack_pointer = &TCB_STACK[i][STACKSIZE - 16];
  /* PSR thumb mode */
  TCB_STACK[i][STACKSIZE - 1] = (1U << 24);
}

uint8_t create_task(void (*task0)(void), void (*task1)(void),
                    void (*task2)(void)) {
  __disable_irq();

  tcbs[0].next = &tcbs[1];
  tcbs[1].next = &tcbs[2];
  tcbs[2].next = &tcbs[0];

  task_stack_init(0);
  TCB_STACK[0][STACKSIZE - 2] = (uint32_t)(task0); // Program Counter
  task_stack_init(1);
  TCB_STACK[1][STACKSIZE - 2] = (uint32_t)(task1); // Program Counter
  task_stack_init(2);
  TCB_STACK[2][STACKSIZE - 2] = (uint32_t)(task2); // Program Counter

  current_tcb_pointer = &tcbs[0];

  __enable_irq();

  return 1;
}

void task_yield(void) {
  /* Clear systick current value register */
  SysTick->VAL = 0;
  /* Trigger systick */
  SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
}
