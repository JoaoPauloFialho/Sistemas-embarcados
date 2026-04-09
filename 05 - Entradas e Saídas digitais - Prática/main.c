#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED GPIO_NUM_4
#define BUTTON GPIO_NUM_2
#define TEMPO_LIMITE_MS 10000

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configuração do Botão
    io_conf.pin_bit_mask = (1ULL << BUTTON);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    int ultimo_estado_botao = 1;
    TickType_t momento_que_ligou = 0;

    for(;;) {
        int estado_atual_botao = gpio_get_level(BUTTON);
        int estado_led = gpio_get_level(LED);

        if (ultimo_estado_botao == 1 && estado_atual_botao == 0) {
            estado_led = !estado_led;
            gpio_set_level(LED, estado_led);

            if (estado_led == 1) {
                momento_que_ligou = xTaskGetTickCount();
            }
            
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        ultimo_estado_botao = estado_atual_botao;

        if (estado_led == 1) {
            TickType_t tempo_atual = xTaskGetTickCount();
            if ((tempo_atual - momento_que_ligou) >= pdMS_TO_TICKS(TEMPO_LIMITE_MS)) {
                gpio_set_level(LED, 0);
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}