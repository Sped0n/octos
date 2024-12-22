#include "scheduler.h"

void scheduler_launch(void) {
  /* R0 contains the address of current_tcb_pointer */
  __asm("LDR     R0, =current_tcb_pointer");
  /* R2 contains the value of current_tcb_pointer */
  __asm("LDR     R2, [R0]");
  /* Load the SP reg with the stacked SP value */
  __asm("LDR     R4, [R2]");
  __asm("MOV     SP, R4");
  /* Pop registers R8-R11(user saved context) */
  __asm("POP     {R4-R7}");
  __asm("MOV     R8, R4");
  __asm("MOV     R9, R5");
  __asm("MOV     R10, R6");
  __asm("MOV     R11, R7");
  /* Pop registers R4-R7(user saved context) */
  __asm("POP     {R4-R7}");
  /* Start poping the stacked exception frame */
  __asm("POP     {R0-R3}");
  __asm("POP     {R4}");
  __asm("MOV     R12, R4");
  /* Skip the saved LR */
  __asm("ADD     SP,SP,#4");
  /* POP the saved PC into LR via R4, We do this to jump into the
   * first task when we execute the branch instruction to exit this routine.
   */
  __asm("POP     {R4}");
  __asm("MOV     LR, R4");
  __asm("ADD     SP,SP,#4");
  /* Enable interrupt */
  __asm("CPSIE   I");
  __asm("BX      LR");
}
