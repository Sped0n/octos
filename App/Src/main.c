#include <stdint.h>
#include <stdio.h>

#include "itc.h"
#include "kernel.h"
#include "led.h"
#include "main.h"
#include "sync.h"
#include "task.h"
#include "usart.h"
#include "utils.h"

#define QUEUE_SIZE 20
#define ITEM_SIZE sizeof(char)


volatile uint64_t tp0 = 0, tp1 = 0, tp2 = 0;
uint32_t task1_buffer[256];

MsgQueue_t queue_test;

Mutex_t mutex_test;

void task0(void) {
    BSP_LED_Toggle(LED1);
    while (1) {
        task_delay(time_to_ticks(1, SECONDS));
        BSP_LED_Toggle(LED1);
    }
}

// main.c

void task1(void) {
    char recv_buffer;

    BSP_LED_Toggle(LED2);
    while (1) {
        if (tp1 % 1000 == 0 && tp1 > 0) { BSP_LED_Toggle(LED2); }
        if (tp1 == 5000) { task_create((TaskFunc_t) &task0, NULL, 2, 256); }
        if (msg_queue_recv(&queue_test, &recv_buffer, 0)) {
            mutex_acquire(&mutex_test);
            printf("%s", &recv_buffer);
            if (recv_buffer == '\r') { mutex_release(&mutex_test); }
        }
        tp1++;
    }
}

void task2(void) {
    const char sentence[] =
            "---------hello from task1, from task2---------\n\r";
    BSP_LED_Toggle(LED3);
    while (1) {
        if (tp2 % 5000 == 0 && tp2 > 0) {
            BSP_LED_Toggle(LED3);
            for (size_t i = 0; sentence[i] != '\0'; i++) {
                if (!msg_queue_send(&queue_test, &sentence[i], 0)) {
                    mutex_acquire(&mutex_test);
                    printf("queue send timeout !!!\n\r");
                    mutex_release(&mutex_test);
                }
            }
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
    USART_Config_t config = {.USARTx = USART3,
                             .TX = &usart3_tx,
                             .RX = &usart3_rx,
                             .BaudRate = 115200};
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

    // Initialize queue
    uint8_t queue_storage[QUEUE_SIZE * ITEM_SIZE];
    msg_queue_init(&queue_test, queue_storage, ITEM_SIZE, QUEUE_SIZE);

    // Initialize variables
    tp0 = tp1 = tp2 = 0;

    task_create_static((TaskFunc_t) &task1, NULL, 2, task1_buffer, 256);
    task_create((TaskFunc_t) &task2, NULL, 0, 256);

    // Launch kernel with 1ms time quantum
    Quanta_t quanta = {.Unit = MILISECONDS, .Value = 1};
    kernel_launch(&quanta);

    // Should never reach here
    while (1);
}
