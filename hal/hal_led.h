#ifndef HAL_LED_H
#define HAL_LED_H
#include "pico/stdlib.h"

void hal_led_init();
void hal_led_set_color(bool r, bool g, bool b);

#endif