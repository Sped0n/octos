#include "main.h"

int main(void) {
  led_init();

  while (1) {
    led_on();
    for (int i = 0; i < 900; i++)
      ;
    led_off();
    for (int i = 0; i < 900; i++)
      ;
  }
}
