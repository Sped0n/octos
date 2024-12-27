#ifndef __DRIVER_LED_H__
#define __DRIVER_LED_H__

#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"

typedef enum {
    LED1 = 0,
    LED_GREEN = LED1,
    LED2 = 1,
    LED_BLUE = LED2,
    LED3 = 2,
    LED_RED = LED3
} LED_TypeDef;

#define LEDn 3

#define LED1_PIN LL_GPIO_PIN_0
#define LED1_GPIO_PORT GPIOB

#define LED2_PIN LL_GPIO_PIN_7
#define LED2_GPIO_PORT GPIOB

#define LED3_PIN LL_GPIO_PIN_14
#define LED3_GPIO_PORT GPIOB

#define LEDx_GPIO_CLK_ENABLE() \
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB)
#define LEDx_GPIO_CLK_DISABLE() \
    LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOB)

void BSP_LED_Init(LED_TypeDef Led);
void BSP_LED_DeInit(LED_TypeDef Led);
void BSP_LED_On(LED_TypeDef Led);
void BSP_LED_Off(LED_TypeDef Led);
void BSP_LED_Toggle(LED_TypeDef Led);

#endif
