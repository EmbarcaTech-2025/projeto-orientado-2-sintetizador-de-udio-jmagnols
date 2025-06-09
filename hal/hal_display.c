#include "hal_display.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <string.h>

#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

// buffer e área de renderização agora são locais ao driver do display
static uint8_t display_buffer[ssd1306_buffer_length];
static struct render_area frame_area;

void hal_display_init() {
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    ssd1306_init();

    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);
}

void hal_display_message(const char* line1, const char* line2) {
    memset(display_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display_buffer, 0, 0, (char*)line1);
    ssd1306_draw_string(display_buffer, 0, 16, (char*)line2);
    render_on_display(display_buffer, &frame_area);
}

void hal_display_waveform(uint16_t* audio_buffer, uint32_t buffer_size) {
    memset(display_buffer, 0, ssd1306_buffer_length);
    int samples_per_pixel = buffer_size / ssd1306_width;
    for (int x = 0; x < ssd1306_width; ++x) {
        uint16_t sample = audio_buffer[x * samples_per_pixel];
        int y = (sample * (ssd1306_height - 1)) / 4095;
        ssd1306_draw_line(display_buffer, x, ssd1306_height / 2, x, y, true);
    }
    render_on_display(display_buffer, &frame_area);
}