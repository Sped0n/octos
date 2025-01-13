#include <stdint.h>
#include <stdlib.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "Kernel/Inc/utils.h"
#include "task.h"
#include "tcb.h"

#include "stm32f4xx.h"// IWYU pragma: keep

extern TCB_t *volatile current_tcb;

volatile uint32_t critical_nesting = 0;

/**
  * @brief Launches the first task by restoring its context and enabling interrupts
  * @note This function is marked as naked to prevent compiler from adding prologue/epilogue
  * @note Function performs stack manipulation to restore saved context and exception frame
  * @retval None
  */
OCTOS_NAKED void OCTOS_SCHED_LAUNCH(void) {
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

/**
 * @brief Configures the Nested Vectored Interrupt Controller (NVIC) priority settings
 * @note Sets the priority grouping to use all bits for preempt priority
 * @note Configures priority for SysTick and PendSV interrupts
 * @retval None
 */
void OCTOS_SETUP_INTPRI(void) {
    /* Assign all the priority bits to be preempt priority bits */
    NVIC_SetPriorityGrouping(0x00000003U);
    /* Set priority for interrupts (Max 0, Min 15) */
    NVIC_SetPriority(SysTick_IRQn, OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(PendSV_IRQn, 15);
}

/**
  * @brief Initializes the SysTick timer with specified time quantum
  * @param quanta Pointer to Quanta structure containing timer configuration
  * @note Configures SysTick for specified time slice scheduling
  * @retval None
  */
void OCTOS_SETUP_SYSTICK(Quanta_t *quanta) {
    /* Reset systick */
    SysTick->CTRL = 0;
    /* Clear systick current value register */
    SysTick->VAL = 0;
    /* Load quanta */
    SysTick->LOAD = (quanta->Value * (SystemCoreClock / quanta->Unit)) - 1;
    /* Setup systick */
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
}

/**
  * @brief Enables the SysTick timer
  * @note Starts the SysTick counter by setting the ENABLE bit
  * @retval None
  */
void OCTOS_ENABLE_SYSTICK(void) { SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; }

/**
 * @brief Handles assertion failure by entering a critical section and halting execution
 * @param file Source file where assertion failed
 * @param line Line number where assertion failed
 * @note Marked with OCTOS_UNUSED to suppress unused parameter warnings
 * @note Enters an infinite loop to prevent further execution after assertion failure
 */
void OCTOS_ASSERT_CALLED(OCTOS_UNUSED const char *file,
                         OCTOS_UNUSED uint64_t line) {
    OCTOS_ENTER_CRITICAL();
    for (;;);
}

/**
  * @brief Allocate memory in a thread-safe manner
  * @note This function suspends all tasks before allocating memory to ensure thread safety
  * @param wanted_size The size of the memory block to allocate
  * @retval Pointer to the allocated memory block, or NULL if allocation fails
  */
void *OCTOS_MALLOC(size_t wanted_size) {
    void *result;

    task_suspend_all();

    result = malloc(wanted_size);

    task_resume_all();

    return result;
}

/**
  * @brief Free memory in a thread-safe manner
  * @note This function suspends all tasks before freeing memory to ensure thread safety
  * @param ptr_to_free Pointer to the memory block to free
  * @retval None
  */
void OCTOS_FREE(void *ptr_to_free) {
    if (ptr_to_free) {
        task_suspend_all();

        free(ptr_to_free);

        task_resume_all();
    }
}
