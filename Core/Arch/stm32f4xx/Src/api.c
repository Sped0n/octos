#include "Arch/stm32f4xx/Inc/api.h"
#include "global.h"
#include "kernel.h"
#include "tcb.h"

#include "stm32f4xx.h"// IWYU pragma: keep

extern TCB_t *current_tcb;

MICROS_NAKED void scheduler_launch(void) {
    /* Load the SP reg with the stacked SP value */
    __asm("LDR     SP, %0" ::"m"(current_tcb->StackTop));
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

void setup_interrupt_priority(void) {
    /* Set priority for interrupts (Max 0, Min 15) */
    NVIC_SetPriority(SysTick_IRQn, MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(PendSV_IRQn, MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
}

void setup_systick(Quanta_t *quanta) {
    /* Reset systick */
    SysTick->CTRL = 0;
    /* Clear systick current value register */
    SysTick->VAL = 0;
    /* Load quanta */
    SysTick->LOAD = (quanta->Value * (SystemCoreClock / quanta->Unit)) - 1;
    /* Setup systick */
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
}

void enable_systick(void) {
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}
