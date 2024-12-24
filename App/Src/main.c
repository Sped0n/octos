#include "main.h"
#include "kernel.h"
#include "led.h"
#include "stm32f429xx.h"
#include "task.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>

uint64_t tp0, tp1, tp2;

void task0(void) {
  while (1) {
    if (tp0 % 10000 == 0 && tp0 > 0)
      BSP_LED_Toggle(LED1);

    tp0++;
  }
}

void task1(void) {
  while (1) {
    if (tp1 % 50000 == 0 && tp1 > 0)
      BSP_LED_Toggle(LED2);
    tp1++;
  }
}
void task2(void) {
  while (1) {
    if (tp2 % 100000 == 0 && tp2 > 0) {
      BSP_LED_Toggle(LED3);
      printf("hello\n\r");
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
                                .Alternate = (7U << GPIO_AFRH_AFSEL9_Pos)

  };
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
  USART3_Init();
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  /*Add Threads*/
  create_task(&task0, &task1, &task2);

  /*Set RoundRobin time quanta*/
  kernel_launch(10);
}
