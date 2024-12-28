/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "Arch/stm32f4xx/Inc/isr.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scheduler.h"
#include "stm32f4xx.h"// IWYU pragma: keep
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
 * @brief This function handles Pendable request for system service.
 */
__attribute__((naked)) void PendSV_Handler(void) {
    /* ------ STEP 1 - SAVE THE CURRENT TASK CONTEXT ------ */
    /* At this point the processor has already pushed PSR, PC, LR, R12, R3, R2,
   * R1 and R0 onto the stack. We need to push the rest(i.e R4, R5, R6, R7, R8,
   * R9, R10 & R11) to save the context of the current task
   */
    /* Push registers R4-R11 */
    __asm("PUSH    {R4-R11}");
    /* Load R0 with the address of current tcb pointer */
    __asm("LDR     R0, =current_tcb");
    /* Load R1 with the value of current tcb pointer(i.e after this, R1 will
   * contain the address of current TCB)
   */
    __asm("LDR     R1, [R0]");
    /* Store the value of the stack pointer to the current tasks
   * "stack_pointer" element in its TCB. This marks an end to saving the
   * context of the current task
   */
    __asm("STR     SP, [R1]");

    /* ------ STEP 2: LOAD THE NEW TASK CONTEXT FROM ITS STACK TO THE CPU
   * REGISTERS, THEN UPDATE current_tcb_pointer ------ */
    __asm("PUSH    {R0,LR}");
    __asm("BL      scheduler_rr");
    __asm("POP     {R0,LR}");
    __asm("LDR     R1, [R0]");
    /* Load the newer tasks TCB to the SP */
    __asm("LDR     SP, [R1]");
    /* Pop registers R4-R11 */
    __asm("POP     {R4-R11}");
    /* Return from exception */
    __asm("BX      LR");
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void) { scheduler_trigger(); }
