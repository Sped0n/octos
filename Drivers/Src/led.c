/*
 * Attention: this LED BSP package require stm32f4xx LL library to run!
 */

#include "led.h"
#include "stm32f4xx_ll_gpio.h"

GPIO_TypeDef *LED_GPIO_PORT[LEDn] = {LED1_GPIO_PORT, LED2_GPIO_PORT,
                                     LED3_GPIO_PORT};

const uint16_t LED_GPIO_PIN[LEDn] = {LED1_PIN, LED2_PIN, LED3_PIN};

/**
 * @brief  Configures LED GPIO.
 * @param  Led: Specifies the Led to be configured.
 */
void BSP_LED_Init(LED_TypeDef Led) {
  LL_GPIO_InitTypeDef gpio_init_struct;

  /* Enable the LED_GPIO Clock */
  LEDx_GPIO_CLK_ENABLE();

  /* Configure the LED GPIO pin */
  gpio_init_struct.Pin = LED_GPIO_PIN[Led];
  gpio_init_struct.Mode = LL_GPIO_MODE_OUTPUT;
  gpio_init_struct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init_struct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init_struct.Pull = LL_GPIO_PULL_NO;

  /* Initialize the LED GPIO pin */
  LL_GPIO_Init(LED_GPIO_PORT[Led], &gpio_init_struct);
  /* Default off */
  LL_GPIO_ResetOutputPin(LED_GPIO_PORT[Led], LED_GPIO_PIN[Led]);
}

/**
 * @brief  DeInit LEDs.
 * @param  Led: LED to be de-init.
 * @note   THe DeInit function does not disable the GPIO clock
 */
void BSP_LED_DeInit(LED_TypeDef Led) {
  /* Turn off LED */
  LL_GPIO_ResetOutputPin(LED_GPIO_PORT[Led], LED_GPIO_PIN[Led]);
  /* DeInit the GPIO_LED pin */
  LL_GPIO_DeInit(LED_GPIO_PORT[Led]);
}

/**
 * @brief  Turns selected LED On.
 * @param  Led: Specifies the Led to be set on.
 */
void BSP_LED_On(LED_TypeDef Led) {
  LL_GPIO_SetOutputPin(LED_GPIO_PORT[Led], LED_GPIO_PIN[Led]);
}

/**
 * @brief  Turns selected LED off.
 * @param  Led: Specifies the Led to be set off.
 */
void BSP_LED_Off(LED_TypeDef Led) {
  LL_GPIO_ResetOutputPin(LED_GPIO_PORT[Led], LED_GPIO_PIN[Led]);
}

/**
 * @brief  Toggles selected LED.
 * @param  Led: Specifies the led to be toggled.
 */
void BSP_LED_Toggle(LED_TypeDef Led) {
  LL_GPIO_TogglePin(LED_GPIO_PORT[Led], LED_GPIO_PIN[Led]);
}
