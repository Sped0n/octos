#include <stdint.h>
#include <stdio.h>

#include "kernel.h"
#include "led.h"
#include "main.h"
#include "mqueue.h"
#include "sync.h"
#include "task.h"
#include "usart.h"
#include "utils.h"

#define QUEUE_SIZE 10
#define ITEM_SIZE sizeof(char)


volatile uint64_t tp0 = 0, tp1 = 0, tp2 = 0, tp3 = 0;
uint32_t task1_buffer[512];

MsgQueue_t queue_test;

Mutex_t mutex_test;

void task0(void) {
    BSP_LED_Toggle(LED1);
    while (1) {
        task_delay(time_to_ticks(1, SECONDS));
        BSP_LED_Toggle(LED1);
        mutex_acquire(&mutex_test, UINT32_MAX);
        printf("-hello from task0---------\n\r");
        mutex_release(&mutex_test);
    }
}

// main.c

void task1(void) {
    char recv_buffer[50];

    uint32_t i = 0;
    BSP_LED_Toggle(LED2);
    while (1) {
        if (tp1 % 500 == 0 && tp1 > 0) { BSP_LED_Toggle(LED2); }
        if (tp1 == 501) {
            task_create((TaskFunc_t) &task0, NULL, "Task 0", 1, 512);
        }
        if (mqueue_recv(&queue_test, &recv_buffer[i], 3)) {
            if (recv_buffer[i] == '\r') {
                recv_buffer[i + 1] = '\0';
                i = 0;
                mutex_acquire(&mutex_test, UINT32_MAX);
                printf("%s", recv_buffer);
                mutex_release(&mutex_test);
            } else {
                i++;
            }
        }

        tp1++;
    }
}


void task2(void) {
    while (1) {
        if (tp2 % 3000 == 0 && tp2 > 0) {
            mutex_acquire(&mutex_test, UINT32_MAX);
            printf("---------hello from task2---------\n\r");
            mutex_release(&mutex_test);
        }
        tp2++;
    }
}

void task3(void) {
    const char sentence[] =
            "---------hello from task1, from task3---------\n\r";
    BSP_LED_Toggle(LED3);
    while (1) {
        if (tp3 == 5001) {
            task_create((TaskFunc_t) &task2, NULL, "Task 2", 0, 512);
        }
        if (tp3 % 3000 == 0 && tp3 > 0) {
            BSP_LED_Toggle(LED3);
            for (size_t i = 0; sentence[i] != '\0'; i++) {
                mqueue_send(&queue_test, &sentence[i], UINT32_MAX);
            }
        }
        tp3++;
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
    mqueue_init(&queue_test, queue_storage, ITEM_SIZE, QUEUE_SIZE);

    task_create_static((TaskFunc_t) &task1, NULL, "Task 1", 0, task1_buffer,
                       512);
    task_create((TaskFunc_t) &task3, NULL, "Task 3", 0, 512);

    // Launch kernel with 1ms time quantum
    Quanta_t quanta = {.Unit = MILISECONDS, .Value = 5};
    kernel_launch(&quanta);

    // Should never reach here
    while (1);
}
