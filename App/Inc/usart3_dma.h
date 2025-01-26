#ifndef __USART3_DMA_H__
#define __USART3_DMA_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attr.h"

#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_usart.h"

void usart3_dma_init(void);
void usart3_dma_rx_check(void);
void usart3_send_string(const char *str);

/** 
 * @brief Check if the DMA half-transfer interrupt flag is set
 * @retval true Half-transfer interrupt flag is set
 * @retval false Half-transfer interrupt flag is not set
 */
OCTOS_INLINE static inline bool usart3_dma_rx_check_ht(void) {
    const bool result = LL_DMA_IsEnabledIT_HT(DMA1, LL_DMA_STREAM_1) &&
                        LL_DMA_IsActiveFlag_HT1(DMA1);
    if (result) LL_DMA_ClearFlag_HT1(DMA1);
    return result;
}

/** 
 * @brief Check if the DMA transfer-complete interrupt flag is set
 * @retval true Transfer-complete interrupt flag is set
 * @retval false Transfer-complete interrupt flag is not set
 */
OCTOS_INLINE static inline bool usart3_dma_rx_check_tc(void) {
    const bool result = LL_DMA_IsEnabledIT_TC(DMA1, LL_DMA_STREAM_1) &&
                        LL_DMA_IsActiveFlag_TC1(DMA1);
    if (result) LL_DMA_ClearFlag_TC1(DMA1);
    return result;
}

/** 
 * @brief Check if the USART idle line interrupt flag is set
 * @retval true Idle line interrupt flag is set
 * @retval false Idle line interrupt flag is not set
 */
OCTOS_INLINE static inline bool usart3_dma_rx_check_idle(void) {
    const bool result = LL_USART_IsEnabledIT_IDLE(USART3) &&
                        LL_USART_IsActiveFlag_IDLE(USART3);
    if (result) LL_USART_ClearFlag_IDLE(USART3);
    return result;
}

/** 
 * @brief Process data and transmit it via USART3
 * @param data Pointer to the data to be processed and transmitted
 * @param len Length of the data to be processed
 * @return None
 */
OCTOS_INLINE static inline void usart3_dma_process_data(const void *data,
                                                        size_t len) {
    const uint8_t *tmp = data;

    for (; len > 0; --len, ++tmp) {
        LL_USART_TransmitData8(USART3, *tmp);
        while (!LL_USART_IsActiveFlag_TXE(USART3));
    }
    while (!LL_USART_IsActiveFlag_TC(USART3));
}

#endif
