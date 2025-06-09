#include "hal_buzzer.h"
#include "hardware/pwm.h"

#define BUZZER_PIN 21

void hal_buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, (1 << 12) - 1);
    pwm_init(slice_num, &config, true);
}

void hal_buzzer_playback(uint16_t* buffer, uint32_t buffer_size, uint32_t sample_rate, volatile synthesizer_state_t* state_ptr) {
    uint32_t delay_us = 1000000 / sample_rate;
    for (int i = 0; i < buffer_size; i++) {
        if (*state_ptr != STATE_PLAYBACK) break;
        pwm_set_gpio_level(BUZZER_PIN, buffer[i]);
        sleep_us(delay_us);
    }
    pwm_set_gpio_level(BUZZER_PIN, 0);
}