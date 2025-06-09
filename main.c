#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// --- pinos de hardware ---
#define BUTTON_A_PIN 5      // pino do botão a
#define BUTTON_B_PIN 6      // pino do botão b
#define MIC_PIN 28          // pino do microfone (entrada adc)
#define MIC_ADC_CHANNEL 2   // canal adc do microfone
#define BUZZER_PIN 21       // pino do buzzer (saída pwm)
#define LED_R_PIN 13        // pino do led vermelho
#define LED_G_PIN 11        // pino do led verde
#define LED_B_PIN 12        // pino do led azul
#define I2C_SDA_PIN 14      // pino sda para o oled
#define I2C_SCL_PIN 15      // pino scl para o oled

// --- configuração de áudio --
#define SAMPLING_RATE 8000  // taxa de amostragem (hz)
#define RECORDING_S 2       // tempo de gravação (s)
#define BUFFER_SIZE (SAMPLING_RATE * RECORDING_S)
#define DC_OFFSET 2048      // ponto médio do adc de 12 bits (nível de silêncio)

// --- variáveis globais ---
uint16_t sample_buffer[BUFFER_SIZE]; // buffer para as amostras de áudio
struct render_area frame_area;       // área de renderização do display

// enum para a máquina de estados principal
typedef enum { STATE_IDLE, STATE_RECORDING, STATE_HAS_RECORDING, STATE_PLAYBACK } synthesizer_state_t;
// estado atual do sistema (volátil por ser alterado na isr)
volatile synthesizer_state_t current_state = STATE_IDLE;

// --- protótipos de funções---
void gpio_callback(uint gpio, uint32_t events);
void setup_hardware();
void apply_blackman_window();
void record_audio();
void playback_audio();
void display_waveform(uint8_t* buf);
void display_message(uint8_t* buf, const char* line1, const char* line2);
void set_led_color(bool r, bool g, bool b);

// controla o led rgb
void set_led_color(bool r, bool g, bool b) {
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
}

// isr callback para os botões
void gpio_callback(uint gpio, uint32_t events) {
    // botão a: solicita gravação
    if (gpio == BUTTON_A_PIN && (current_state == STATE_IDLE || current_state == STATE_HAS_RECORDING)) {
        current_state = STATE_RECORDING;
    }
    // botão b: solicita reprodução
    if (gpio == BUTTON_B_PIN && current_state == STATE_HAS_RECORDING) {
        current_state = STATE_PLAYBACK;
    }
}

// inicializa todo o hardware
void setup_hardware() {
    // configura leds
    gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN); gpio_set_dir(LED_B_PIN, GPIO_OUT);
    
    // configura adc
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_ADC_CHANNEL);
    adc_set_clkdiv(0); // adc na velocidade máxima, loop controlado por delay

    // configura pwm do buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, (1 << 12) - 1);
    pwm_init(slice_num, &config, true);

    // configura botões com interrupção
    gpio_init(BUTTON_A_PIN); gpio_set_dir(BUTTON_A_PIN, GPIO_IN); gpio_pull_up(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN); gpio_set_dir(BUTTON_B_PIN, GPIO_IN); gpio_pull_up(BUTTON_B_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    
    // configura i2c e display oled
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    ssd1306_init();

    // configura área de renderização para a tela inteira
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);
}

// exibe uma mensagem de duas linhas no oled
void display_message(uint8_t* buf, const char* line1, const char* line2) {
    memset(buf, 0, ssd1306_buffer_length);
    ssd1306_draw_string(buf, 0, 0, (char*)line1);
    ssd1306_draw_string(buf, 0, 16, (char*)line2);
    render_on_display(buf, &frame_area);
}

// desenha a forma de onda no oled
void display_waveform(uint8_t* buf) {
    memset(buf, 0, ssd1306_buffer_length);
    // downsampling do áudio para caber nos 128 pixels da tela
    int samples_per_pixel = BUFFER_SIZE / ssd1306_width;
    for (int x = 0; x < ssd1306_width; ++x) {
        uint16_t sample = sample_buffer[x * samples_per_pixel];
        // mapeia a amplitude da amostra para a altura do display
        int y = (sample * (ssd1306_height - 1)) / 4095;
        // desenha uma linha vertical do centro até a altura da amostra
        ssd1306_draw_line(buf, x, ssd1306_height / 2, x, y, true);
    }
    render_on_display(buf, &frame_area);
}

// aplica o filtro de janela de blackman para suavizar o áudio
void apply_blackman_window() {
    for (int n = 0; n < BUFFER_SIZE; n++) {
        // calcula o coeficiente da janela para a amostra 'n'
        double blackman_coeff = 0.42 - 
                                0.5 * cos(2 * M_PI * n / (BUFFER_SIZE - 1)) + 
                                0.08 * cos(4 * M_PI * n / (BUFFER_SIZE - 1));

        // remove o offset dc para isolar o sinal de áudio (componente ac)
        int16_t ac_sample = (int16_t)sample_buffer[n] - DC_OFFSET;

        // aplica o coeficiente da janela ao componente ac
        int16_t windowed_ac_sample = (int16_t)(ac_sample * blackman_coeff);

        // adiciona o offset dc de volta para o formato do adc (0-4095)
        sample_buffer[n] = (uint16_t)(windowed_ac_sample + DC_OFFSET);
    }
}

// grava o áudio do microfone para o buffer
void record_audio() {
    uint32_t delay_us = 1000000 / SAMPLING_RATE;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sample_buffer[i] = adc_read();
        sleep_us(delay_us);
    }
}

// reproduz o áudio do buffer no buzzer
void playback_audio() {
    uint32_t delay_us = 1000000 / SAMPLING_RATE;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // interrompe a reprodução se outro comando for dado
        if (current_state != STATE_PLAYBACK) break;
        pwm_set_gpio_level(BUZZER_PIN, sample_buffer[i]);
        sleep_us(delay_us);
    }
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// ponto de entrada principal
int main() {
    stdio_init_all();
    sleep_ms(1000); // aguarda a inicialização do terminal serial
    setup_hardware();
    
    uint8_t display_buffer[ssd1306_buffer_length];

    while (true) {
        switch (current_state) {
            // estado ocioso: pisca led azul e aguarda comando
            case STATE_IDLE:
                set_led_color(0, 0, 1);
                display_message(display_buffer, "Sintetizador", "Aperte A->Gravar");
                sleep_ms(500);
                set_led_color(0, 0, 0);
                sleep_ms(500);
                break;

            // estado de gravação: acionado pelo botão a
            case STATE_RECORDING:
                set_led_color(1, 0, 0);
                display_message(display_buffer, "Gravando...", "");
                record_audio();
                apply_blackman_window();
                display_waveform(display_buffer);
                current_state = STATE_HAS_RECORDING;
                break;

            // estado de espera: áudio gravado, aguarda reprodução
            case STATE_HAS_RECORDING:
                set_led_color(0, 0, 1);
                tight_loop_contents(); // aguarda interrupção do botão
                break;

            // estado de reprodução: acionado pelo botão b
            case STATE_PLAYBACK:
                set_led_color(0, 1, 0);
                display_message(display_buffer, "Tocando...", "");
                playback_audio();
                display_waveform(display_buffer);
                current_state = STATE_HAS_RECORDING;
                break;
        }
    }
    return 0;
}