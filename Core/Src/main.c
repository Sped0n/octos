#include "main.h"
#include "timebase.h"
#include "uart.h"
#include <stdio.h>

int main(void) {
  led_init();
  uart_tx_unit();
  timebase_init();

  while (1) {
    led_on();
    delay(1);
    led_off();
    delay(1);
    printf("hello\n\r");
  }
}
