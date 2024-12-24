#include "usart.h"
#include "Drivers/Inc/utils.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"
#include <stdint.h>

/**
 * @brief  Init USART GPIO.
 * @param  config: The config for USART init.
 */
static void BSP_USART_GPIO_Init(USART_Config_t *config) {
  /* Configure TX and RX pin */
  EXT_GPIO_TypeDef *x;
  for (uint8_t i = 0; i < 2; i++) {
    if (i == 0)
      x = config->TX;
    else
      x = config->RX;

    /* Enable the corresponding GPIO Clock */
    LL_AHB1_GRP1_EnableClock(GPIO_Get_Clock_Enable_Flag(x->Port));

    /* Set the pin flag */
    LL_GPIO_SetPinMode(x->Port, x->Pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(x->Port, x->Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(x->Port, x->Pin, LL_GPIO_PULL_UP);
    if (i == 0)
      LL_GPIO_SetPinOutputType(x->Port, x->Pin, LL_GPIO_OUTPUT_PUSHPULL);

    /* Set the alternate function */
    if (x->Pin <= LL_GPIO_PIN_7)
      LL_GPIO_SetAFPin_0_7(x->Port, x->Pin, x->Alternate);
    else
      LL_GPIO_SetAFPin_8_15(x->Port, x->Pin, x->Alternate);
  }
}

/**
 * @brief  DeInit USART GPIO.
 * @param  config: The same config for USART init.
 * @note   The DeInit function does not disable the GPIO clock.
 */
static void BSP_USART_GPIO_DeInit(USART_Config_t *config) {
  /* Reset GPIO pins */
  EXT_GPIO_TypeDef *x;
  for (uint8_t i = 0; i < 2; i++) {
    if (i == 0)
      x = config->TX;
    else
      x = config->RX;

    /* Reset GPIO configuration */
    LL_GPIO_SetPinMode(x->Port, x->Pin, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinSpeed(x->Port, x->Pin, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(x->Port, x->Pin, LL_GPIO_PULL_NO);

    /* Reset alternate function */
    if (x->Pin <= LL_GPIO_PIN_7)
      LL_GPIO_SetAFPin_0_7(x->Port, x->Pin, LL_GPIO_AF_0);
    else
      LL_GPIO_SetAFPin_8_15(x->Port, x->Pin, LL_GPIO_AF_0);
  }
}

/**
 * @brief  Init USART.
 * @param  config: The config for USART init.
 * @note   This function doesn't enable USARTx for you, you have to do it
 *         implicitly.
 */
void BSP_USART_Init(USART_Config_t *config) {
  /* Enable USART clocks */
  if (config->USARTx == USART1) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  } else if (config->USARTx == USART2) {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
  } else if (config->USARTx == USART3) {
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
  } else if (config->USARTx == USART6) {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);
  } else {
    return;
  }

  /* Initialize GPIO */
  BSP_USART_GPIO_Init(config);

  /* Configure USART */
  LL_USART_SetBaudRate(config->USARTx, SystemCoreClock,
                       LL_USART_OVERSAMPLING_16, config->BaudRate);
  LL_USART_SetDataWidth(config->USARTx, LL_USART_DATAWIDTH_8B);
  LL_USART_SetParity(config->USARTx, LL_USART_PARITY_NONE);
  LL_USART_SetTransferDirection(config->USARTx, LL_USART_DIRECTION_TX_RX);
  LL_USART_SetHWFlowCtrl(config->USARTx, LL_USART_HWCONTROL_NONE);
}

/**
 * @brief  DeInit USART.
 * @param  config: The same config for USART init.
 * @note   The DeInit function does not disable the GPIO clock.
 */
void BSP_USART_DeInit(USART_Config_t *config) {
  /* Disable interrupt */
  BSP_USART_DisableIT_RXNE(config->USARTx);
  /* Disable USART */
  BSP_USART_Disable(config->USARTx);

  /* Disable USART clocks */
  if (config->USARTx == USART1) {
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART1);
  } else if (config->USARTx == USART2) {
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);
  } else if (config->USARTx == USART3) {
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3);
  } else if (config->USARTx == USART6) {
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART6);
  }

  /* DeInit GPIO */
  BSP_USART_GPIO_DeInit(config);
}

/**
 * @brief  Enable USART.
 * @param  USARTx: USARTx to enable.
 */
void BSP_USART_Enable(USART_TypeDef *USARTx) { LL_USART_Enable(USARTx); }

/**
 * @brief  Diable USART.
 * @param  USARTx: USARTx to disable.
 */
void BSP_USART_Disable(USART_TypeDef *USARTx) { LL_USART_Disable(USARTx); }

/**
 * @brief  Send a byte with the specified USART.
 * @param  USARTx: The USARTx interface.
 * @param  data: Data to be sent.
 */
void BSP_USART_SendByte(USART_TypeDef *USARTx, uint8_t data) {
  /* Wait for TX empty */
  while (!LL_USART_IsActiveFlag_TXE(USARTx) || !LL_USART_IsEnabled(USARTx))
    ;
  LL_USART_TransmitData8(USARTx, data);
  /* Wait for TX complete */
  while (!LL_USART_IsActiveFlag_TC(USARTx) || !LL_USART_IsEnabled(USARTx))
    ;
}

/**
 * @brief  Send a string with the specified USART.
 * @param  USARTx: The USARTx interface.
 * @param  str: Pointer to the string.
 */
void BSP_USART_SendString(USART_TypeDef *USARTx, const char *str) {
  while (*str) {
    BSP_USART_SendByte(USARTx, *str++);
  }
}

/**
 * @brief  Send data inside provided buffer with the specified USART.
 * @param  USARTx: The USARTx interface.
 * @param  str: Pointer to the buffer.
 */
void BSP_USART_SendBuffer(USART_TypeDef *USARTx, const uint8_t *buffer,
                          uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    BSP_USART_SendByte(USARTx, buffer[i]);
  }
}

/**
 * @brief   Recieve data with the specified USART.
 * @param   USARTx: The USARTx interface.
 * @retval  Value between Min_Data=0x00 and Max_Data=0xFF
 */
uint8_t BSP_USART_ReceiveByte(USART_TypeDef *USARTx) {
  while (!LL_USART_IsActiveFlag_RXNE(USARTx) || !LL_USART_IsEnabled(USARTx))
    ;
  return LL_USART_ReceiveData8(USARTx);
}

/**
 * @brief  Enable RX Not Empty Interrupt with the specified USART.
 * @param  USARTx: The USARTx interface.
 */
void BSP_USART_EnableIT_RXNE(USART_TypeDef *USARTx) {
  LL_USART_EnableIT_RXNE(USARTx);
}

/**
 * @brief  Disable RX Not Empty Interrupt with the specified USART.
 * @param  USARTx: The USARTx interface.
 */
void BSP_USART_DisableIT_RXNE(USART_TypeDef *USARTx) {
  LL_USART_DisableIT_RXNE(USARTx);
}
