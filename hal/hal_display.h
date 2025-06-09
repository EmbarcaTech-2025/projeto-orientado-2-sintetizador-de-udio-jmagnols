#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H
#include "pico/stdlib.h"

void hal_display_init();
void hal_display_message(const char* line1, const char* line2);
void hal_display_waveform(uint16_t* audio_buffer, uint32_t buffer_size);

#endif