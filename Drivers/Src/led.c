#include "led.h"

void led_init(void) {
  /* Enable clock access to the led_port (Port B)*/
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  /* Set led pin as output pin */
  GPIOB->MODER |= GPIO_MODER_MODER0_0;
}

void led_on(void) {
  /* Set led pin HIGH (PB0)*/
  GPIOB->ODR |= GPIO_ODR_OD0;
}

void led_off(void) {
  /* Set led pin LOW (PB0)*/
  GPIOB->ODR &= ~GPIO_ODR_OD0;
}
