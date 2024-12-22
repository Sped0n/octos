#include "main.h"
#include "kernel.h"
#include "task.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

uint32_t tp0, tp1, tp2;

void task0(void) {
  while (1) {
    if (tp0 % 1000 == 0 && tp0 > 0)
      printf("hello from task 0\n\r");
    tp0++;
  }
}

void task1(void) {
  while (1) {
    if (tp1 % 1000 == 0 && tp1 > 0)
      printf("hello from task 1\n\r");
    tp1++;
  }
}
void task2(void) {
  while (1) {
    if (tp2 % 1000 == 0 && tp2 > 0)
      printf("hello from task 2\n\r");
    tp2++;
  }
}

int main(void) {
  uart_tx_unit();

  /*Add Threads*/
  create_task(&task0, &task1, &task2);

  /*Set RoundRobin time quanta*/
  kernel_launch(10);
}
