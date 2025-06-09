#ifndef HAL_BUTTONS_H
#define HAL_BUTTONS_H
#include "pico/stdlib.h"

void hal_buttons_init(gpio_irq_callback_t callback);

#endif