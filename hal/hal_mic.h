#ifndef HAL_MIC_H
#define HAL_MIC_H
#include "pico/stdlib.h"

void hal_mic_init();
void hal_mic_record(uint16_t* buffer, uint32_t buffer_size, uint32_t sample_rate);
void hal_mic_apply_blackman_window(uint16_t* buffer, uint32_t buffer_size);

#endif