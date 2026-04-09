#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_BIT0 GPIO_NUM_4
#define LED_BIT1 GPIO_NUM_5
#define LED_BIT2 GPIO_NUM_6
#define LED_BIT3 GPIO_NUM_7

#define BTN_A GPIO_NUM_1
#define BTN_B GPIO_NUM_2

void update_leds(uint8_t value) {
    gpio_set_level(LED_BIT0, (value >> 0) & 0x01);
    gpio_set_level(LED_BIT1, (value >> 1) & 0x01);
    gpio_set_level(LED_BIT2, (value >> 2) & 0x01);
    gpio_set_level(LED_BIT3, (value >> 3) & 0x01);
}

void app_main(void) {
    gpio_config_t led_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_BIT0) | (1ULL << LED_BIT1) | (1ULL << LED_BIT2) | (1ULL << LED_BIT3),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&led_conf);

    gpio_config_t btn_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BTN_A) | (1ULL << BTN_B),
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    gpio_config(&btn_conf);

    uint8_t contador = 0;
    uint8_t passo = 1;

    int ultimo_estado_btn_a = 1; 
    int ultimo_estado_btn_b = 1;

    update_leds(contador);

    while(1) {
        int estado_atual_btn_a = gpio_get_level(BTN_A);
        int estado_atual_btn_b = gpio_get_level(BTN_B);

        if (ultimo_estado_btn_a == 1 && estado_atual_btn_a == 0) {
            contador = (contador + passo) & 0x0F;
            update_leds(contador);
            
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (ultimo_estado_btn_b == 1 && estado_atual_btn_b == 0) {
            passo = (passo == 1) ? 2 : 1;
            
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        ultimo_estado_btn_a = estado_atual_btn_a;
        ultimo_estado_btn_b = estado_atual_btn_b;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}