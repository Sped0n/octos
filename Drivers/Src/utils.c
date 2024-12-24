#include "stm32f4xx_ll_bus.h"
#include <stdint.h>

/**
 * @brief  Get clock enable flag for provided GPIO port.
 * @param  GPIOx: The GPIO port.
 */
uint32_t GPIO_Get_Clock_Enable_Flag(GPIO_TypeDef *GPIOx) {
  switch ((uint32_t)GPIOx) {
  case GPIOA_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOA;
  case GPIOB_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOB;
  case GPIOC_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOC;
  case GPIOD_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOD;
  case GPIOE_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOE;
  case GPIOF_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOF;
  case GPIOG_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOG;
  case GPIOH_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOH;
  case GPIOI_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOI;
  case GPIOJ_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOJ;
  case GPIOK_BASE:
    return LL_AHB1_GRP1_PERIPH_GPIOK;
  default:
    return 0; // Return 0 for invalid GPIO
  }
}
