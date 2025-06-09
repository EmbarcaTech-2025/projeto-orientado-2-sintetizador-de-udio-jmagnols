#ifndef HAL_BUZZER_H
#define HAL_BUZZER_H
#include "pico/stdlib.h"

// define o tipo para a variável de estado para que o ponteiro possa ser passado
typedef enum { STATE_IDLE, STATE_RECORDING, STATE_HAS_RECORDING, STATE_PLAYBACK } synthesizer_state_t;

void hal_buzzer_init();
void hal_buzzer_playback(uint16_t* buffer, uint32_t buffer_size, uint32_t sample_rate, volatile synthesizer_state_t* state_ptr);

#endif