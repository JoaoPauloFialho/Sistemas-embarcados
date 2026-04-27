#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define BUTTON GPIO_NUM_4
#define LED GPIO_NUM_15
#define LED_DEFAULT 0
#define DEBOUNCE_TIME_MS 50
#define TIMER_PERIOD_MS 10000
#define LONG_PRESS_MS 2000

QueueHandle_t queue;
TimerHandle_t led_timer;

void led_timer_callback(TimerHandle_t xTimer) {
    gpio_set_level(LED, 0);
}

//isr
void IRAM_ATTR gpio_isr(void* arg){
    uint32_t pin = (uint32_t) arg;
    BaseType_t highPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, &pin, &highPriorityTaskWoken);

    if(highPriorityTaskWoken){
        portYIELD_FROM_ISR();
    }
}

//task
void button_task(void* params){
    uint32_t pin_num;

    for(;;){
        if(xQueueReceive(queue, &pin_num, portMAX_DELAY)){
            vTaskDelay(DEBOUNCE_TIME_MS / portTICK_PERIOD_MS);
            
            if(gpio_get_level(pin_num) == 0){
                uint32_t press_duration = 0;
                
                while(gpio_get_level(pin_num) == 0){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    press_duration += 10;
                    
                    if(press_duration >= LONG_PRESS_MS){
                        break;
                    }
                }

                if(press_duration >= LONG_PRESS_MS){
                    gpio_set_level(LED, 0);
                    xTimerStop(led_timer, 0);
                    
                    while(gpio_get_level(pin_num) == 0){
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                    }
                } else {
                    gpio_set_level(LED, 1);
                    xTimerReset(led_timer, 0); 
                }
                
                xQueueReset(queue);
            }
        }
    }
}

void app_main() {
    led_timer = xTimerCreate("led_timer", TIMER_PERIOD_MS / portTICK_PERIOD_MS, pdFALSE, 0, led_timer_callback);

    queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    gpio_config(&(gpio_config_t) 
    {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
    });
    
    gpio_config(&(gpio_config_t) 
    {
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_OUTPUT,
    });

    gpio_install_isr_service(0);
    gpio_set_level(LED, LED_DEFAULT);
    gpio_isr_handler_add(BUTTON, gpio_isr, (void*) BUTTON);

    for(;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}