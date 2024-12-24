#include "main.h"
#include "kernel.h"
#include "led.h"
#include "task.h"
#include <stdint.h>

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
    if (tp2 % 100000 == 0 && tp2 > 0)
      BSP_LED_Toggle(LED3);
    tp2++;
  }
}

int main(void) {
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  /*Add Threads*/
  create_task(&task0, &task1, &task2);

  /*Set RoundRobin time quanta*/
  kernel_launch(10);
}
