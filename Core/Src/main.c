#include "main.h"
#include "uart.h"
#include <stdio.h>

int main(void) {
  led_init();
  uart_tx_unit();

  while (1) {
    led_on();
    for (int i = 0; i < 90000; i++)
      ;
    led_off();
    for (int i = 0; i < 90000; i++)
      ;
    printf("hello\n\r");
  }
}
