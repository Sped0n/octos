#include "scheduler.h"
#include "data.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include "task.h"

extern TCB_t *current_tcb;
extern List_t ready_list;

void scheduler_launch(void) {
  /* Load the SP reg with the stacked SP value */
  __asm("LDR     SP, %0" ::"m"(current_tcb->SP));
  /* Pop registers R4-R11(user saved context) */
  __asm("POP     {R4-R11}");

  /* Start poping the stacked exception frame */
  __asm("POP     {R0-R3}");
  __asm("POP     {R12}");
  /* Skip the saved LR */
  __asm("ADD     SP,SP,#4");
  /* POP the saved PC into LR, We do this to jump into the
   * first task when we execute the branch instruction to exit this routine.
   */
  __asm("POP     {LR}");
  __asm("ADD     SP,SP,#4");
  /* Enable interrupt */
  __asm("CPSIE   I");
  /* Return from exception */
  __asm("BX      LR");
}

void scheduler_rr(void) {
  if (current_tcb->state == RUNNING) {
    list_insert(&ready_list, &(current_tcb->StateListItem));
    current_tcb->state = READY;
  }

  current_tcb = ready_list.End.Next->Owner;
  list_remove(&(current_tcb->StateListItem));
  current_tcb->state = RUNNING;
}
