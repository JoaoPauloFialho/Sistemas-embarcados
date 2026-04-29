#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define LED GPIO_NUM_4
#define TX GPIO_NUM_16
#define RX GPIO_NUM_17
#define UART UART_NUM_2
#define RX_BUFFER_SIZE 1024

void app_main(void) {

    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_OUTPUT
    });

    uart_driver_install(UART, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(
        UART,
        &(uart_config_t){
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .stop_bits = UART_STOP_BITS_1
        }
    );
    uart_set_pin(UART, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t *dados = (uint8_t *) malloc(RX_BUFFER_SIZE);

    char* ligar = "LIGAR";
    char* desligar = "DESLIGAR";
    uint8_t estado = 0;

    for(;;){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if(estado){
            uart_write_bytes(UART, ligar, 6);
            printf("Enviado via TX: %s\n", (char*)ligar);
        } else {
            uart_write_bytes(UART, desligar, 9);
            printf("Enviado via TX: %s\n", (char*)desligar);
        }
        estado = !estado;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        int len = uart_read_bytes(UART, dados, (RX_BUFFER_SIZE - 1), 20 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            dados[len] = '\0';
            
            if(!strcmp(ligar, (char *)dados)){
                gpio_set_level(LED, 1);
            } else {
                gpio_set_level(LED, 0);
            }
            
            printf("Recebido via RX: %s\n", (char*)dados);
            uart_flush(UART);
        }
    }
}
