#include "uart.h"
#include "stm32f429xx.h"

static void uart_write(int data) {
  /* Write data to transmit */
  USART3->DR = data & 0xFF;

  /* Wait for transmit complete */
  while (!(USART3->SR & USART_SR_TC))
    ;
}

void uart_tx_unit(void) {
  /* Enable clock access to GPIOD */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
  /* Set PD8 to alternate function mode */
  GPIOD->MODER |= GPIO_MODER_MODER8_1;
  /* Set alternate function type to AF7 */
  GPIOD->AFR[1] |= GPIO_AFRH_AFSEL8_0 | GPIO_AFRH_AFSEL8_1 | GPIO_AFRH_AFSEL8_2;
  /* Enable clock access to USART3 */
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
  /* Configure baud rate */
  USART3->BRR = ((16000000 + (115200 / 2U)) / 115200);
  /* Configure tranfer direction and enable uart module */
  USART3->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

int __io_putchar(int ch) {
  uart_write(ch);
  return ch;
}
