#include <stdio.h>
#include "pico/stdlib.h"

#include "hal_buttons.h"
#include "hal_led.h"
#include "hal_mic.h"
#include "hal_buzzer.h"
#include "hal_display.h"

// --- configuração de áudio --
#define SAMPLING_RATE 16000  // taxa de amostragem (hz)
#define RECORDING_S 5       // tempo de gravação (s)
#define BUFFER_SIZE (SAMPLING_RATE * RECORDING_S)

// --- variáveis globais ---
uint16_t sample_buffer[BUFFER_SIZE]; // buffer para as amostras de áudio
volatile synthesizer_state_t current_state = STATE_IDLE; // o estado precisa continuar aqui, já q a lógica da aplicação que o controla

// callback para os botões que altera o estado da aplicação
void app_gpio_callback(uint gpio, uint32_t events) {
    if (gpio == 5 && (current_state == STATE_IDLE || current_state == STATE_HAS_RECORDING)) {
        current_state = STATE_RECORDING;
    }
    if (gpio == 6 && current_state == STATE_HAS_RECORDING) {
        current_state = STATE_PLAYBACK;
    }
}

// inicializa toda a aplicação
void setup_app() {
    stdio_init_all();
    sleep_ms(1000);

    // inicializa todos os módulos de hardware
    hal_led_init();
    hal_mic_init();
    hal_buzzer_init();
    hal_display_init();
    hal_buttons_init(app_gpio_callback); // passa o endereço da nossa função de callback
}

// ponto de entrada principal
int main() {
    setup_app();
    
    while (true) {
        switch (current_state) {
            case STATE_IDLE:
                hal_led_set_color(0, 0, 1);
                hal_display_message("Sintetizador", "Aperte A->Gravar");
                sleep_ms(500);
                hal_led_set_color(0, 0, 0);
                sleep_ms(500);
                break;

            case STATE_RECORDING:
                hal_led_set_color(1, 0, 0);
                hal_display_message("Gravando...", "");
                hal_mic_record(sample_buffer, BUFFER_SIZE, SAMPLING_RATE);
                hal_mic_apply_blackman_window(sample_buffer, BUFFER_SIZE);
                hal_display_waveform(sample_buffer, BUFFER_SIZE);
                current_state = STATE_HAS_RECORDING;
                break;

            case STATE_HAS_RECORDING:
                hal_led_set_color(0, 0, 1); // led azul fixo
                tight_loop_contents(); // aguarda interrup
                break;

            case STATE_PLAYBACK:
                hal_led_set_color(0, 1, 0);
                hal_display_message("Tocando...", "");
                hal_buzzer_playback(sample_buffer, BUFFER_SIZE, SAMPLING_RATE, &current_state);
                hal_display_waveform(sample_buffer, BUFFER_SIZE);
                current_state = STATE_HAS_RECORDING;
                break;
        }
    }
    return 0;
}
