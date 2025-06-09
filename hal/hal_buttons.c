#include "hal_buttons.h"
#include "hardware/gpio.h"

#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6

void hal_buttons_init(gpio_irq_callback_t callback) {
    gpio_init(BUTTON_A_PIN); 
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN); 
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN); 
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN); 
    gpio_pull_up(BUTTON_B_PIN);

    // registra o callback fornecido pelo main para ambos os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, callback);
}