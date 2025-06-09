#include "hal_mic.h"
#include "hardware/adc.h"
#include <math.h>

#define MIC_PIN 28
#define MIC_ADC_CHANNEL 2
#define DC_OFFSET 2048

void hal_mic_init() {
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_ADC_CHANNEL);
    adc_set_clkdiv(0);
}

void hal_mic_record(uint16_t* buffer, uint32_t buffer_size, uint32_t sample_rate) {
    uint32_t delay_us = 1000000 / sample_rate;
    for (int i = 0; i < buffer_size; i++) {
        buffer[i] = adc_read();
        sleep_us(delay_us);
    }
}

void hal_mic_apply_blackman_window(uint16_t* buffer, uint32_t buffer_size) {
    for (int n = 0; n < buffer_size; n++) {
        double blackman_coeff = 0.42 - 
                                0.5 * cos(2 * M_PI * n / (buffer_size - 1)) + 
                                0.08 * cos(4 * M_PI * n / (buffer_size - 1));
        
        int16_t ac_sample = (int16_t)buffer[n] - DC_OFFSET;
        int16_t windowed_ac_sample = (int16_t)(ac_sample * blackman_coeff);
        buffer[n] = (uint16_t)(windowed_ac_sample + DC_OFFSET);
    }
}