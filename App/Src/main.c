#include "main.h"
#include "kernel.h"
#include "led.h"
#include "page.h"
#include "stm32f429xx.h"
#include "sync.h"
#include "task.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>

volatile uint64_t tp0 = 0, tp1 = 0, tp2 = 0;
Mutex_t mutex_test;

void task0(void) {
  BSP_LED_Toggle(LED1);
  while (1) {
    if (tp0 % 1000 == 0 && tp0 > 0) {
      BSP_LED_Toggle(LED1);
    }
    tp0++;
    task_yield(); // Add explicit yield to ensure other tasks get CPU time
  }
}

// main.c

void task1(void) {

  BSP_LED_Toggle(LED2);
  while (1) {
    if (tp1 % 1000 == 0 && tp1 > 0) {
      BSP_LED_Toggle(LED2);

      mutex_acquire(&mutex_test);
      printf("########hello from task1###########\n\r");
      mutex_release(&mutex_test);
    }
    tp1++;
  }
}

void task2(void) {

  BSP_LED_Toggle(LED3);
  while (1) {
    if (tp2 % 1000 == 0 && tp2 > 0) {
      BSP_LED_Toggle(LED3);

      mutex_acquire(&mutex_test);
      printf("---------hello from task2---------\n\r");
      mutex_release(&mutex_test);
    }
    tp2++;
  }
}

void USART3_Init(void) {
  EXT_GPIO_TypeDef usart3_tx = {.Port = GPIOD,
                                .Pin = LL_GPIO_PIN_8,
                                .Alternate = (7U << GPIO_AFRH_AFSEL8_Pos)};
  EXT_GPIO_TypeDef usart3_rx = {.Port = GPIOD,
                                .Pin = LL_GPIO_PIN_9,
                                .Alternate = (7U << GPIO_AFRH_AFSEL9_Pos)};
  USART_Config_t config = {
      .USARTx = USART3, .TX = &usart3_tx, .RX = &usart3_rx, .BaudRate = 115200};
  BSP_USART_Init(&config);
  BSP_USART_Enable(config.USARTx);
}

int __io_putchar(int ch) {
  BSP_USART_SendByte(USART3, ch & 0xFF);
  return ch;
}

int main(void) {
  // Initialize system
  SystemInit();

  // Initialize peripherals
  USART3_Init();
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  // Initialize mutex
  mutex_init(&mutex_test);

  // Initialize variables
  tp0 = tp1 = tp2 = 0;

  task_create((task_func_t)&task0, NULL, PAGE_POLICY_POOL);
  task_create((task_func_t)&task1, NULL, PAGE_POLICY_POOL);
  task_create((task_func_t)&task2, NULL, PAGE_POLICY_DYNAMIC);

  // Launch kernel with 1ms time quantum
  kernel_launch(1);

  // Should never reach here
  while (1)
    ;
}
