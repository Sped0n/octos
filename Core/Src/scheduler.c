#include "scheduler.h"

void scheduler_launch(void) {
  /* R0 contains the address of current_tcb_pointer */
  __asm("LDR     R0, =current_tcb_pointer");
  /* R2 contains the value of current_tcb_pointer */
  __asm("LDR     R2, [R0]");
  /* Load the SP reg with the stacked SP value */
  __asm("LDR     SP, [R2]");
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
  __asm("BX      LR");
}
