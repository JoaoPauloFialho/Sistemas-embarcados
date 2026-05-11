#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define BUTTON GPIO_NUM_15
#define LED GPIO_NUM_4
#define LED_DEFAULT 0
#define DEBOUNCE_TIME_MS 50
#define TENSAO_MAXIMA 3.3
#define CANAL ADC_CHANNEL_6
#define CANAL_LEDC LEDC_CHANNEL_0
#define UNIT_ADC ADC_UNIT_1

QueueHandle_t queue;
TimerHandle_t led_timer;
TaskHandle_t rising_task_handle = NULL;
TaskHandle_t falling_task_handle = NULL;

static uint8_t hold = (uint8_t) 0;

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
void button_task_neg(void* params){
  uint32_t pin_num;
  printf("entrou na task\n");
  for(;;){
    if(xQueueReceive(queue, &pin_num, portMAX_DELAY)){
      vTaskDelay(DEBOUNCE_TIME_MS / portTICK_PERIOD_MS);
      if(gpio_get_level(pin_num) == 0){
        hold = !hold;
      }
      xQueueReset(queue);
    }
  }
}

void app_main() {

  queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(button_task_neg, "button_task_neg", 2048, NULL, 10, NULL);

  gpio_config(&(gpio_config_t) {
    .intr_type = GPIO_INTR_ANYEDGE,
    .pin_bit_mask = (1ULL << BUTTON),
    .mode = GPIO_MODE_INPUT,
  });
  gpio_config(&(gpio_config_t) {
    .pin_bit_mask = (1ULL << LED),
    .mode = GPIO_MODE_OUTPUT,
  });
  gpio_install_isr_service(0);
  gpio_set_level(LED, LED_DEFAULT);
  gpio_isr_handler_add(BUTTON, gpio_isr, (void*) BUTTON);


  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = UNIT_ADC,
      .clk_src = 0,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,        
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CANAL, &config));
  adc_cali_handle_t cali_handle = NULL;
  adc_cali_line_fitting_config_t cali_config = {
      .unit_id = UNIT_ADC,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));

  ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_LOW_SPEED_MODE,
      .timer_num        = LEDC_TIMER_0,
      .duty_resolution  = LEDC_TIMER_12_BIT, 
      .freq_hz          = 5000,              
      .clk_cfg          = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);
  ledc_channel_config_t ledc_channel = {
      .speed_mode     = LEDC_LOW_SPEED_MODE,
      .channel        = CANAL_LEDC,
      .timer_sel      = LEDC_TIMER_0,
      .intr_type      = LEDC_INTR_DISABLE,
      .gpio_num       = LED,
      .duty           = 0,
      .hpoint         = 0
  };
  ledc_channel_config(&ledc_channel);

  for(;;) {
    int value_leitura;
    float tensao_v;
    if ((adc_oneshot_read(adc1_handle, CANAL, &value_leitura) == ESP_OK) && !hold) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, CANAL_LEDC, value_leitura);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, CANAL_LEDC);
      tensao_v = (float)value_leitura / 1000.0f;
    }
    vTaskDelay(pdMS_TO_TICKS(500)); 
    float tensao_calculada = ((value_leitura)*TENSAO_MAXIMA/4095);
    printf("Valor ADC: %d| Tensão calculada: %.1f"
           "| Estado: %s\n", value_leitura, tensao_calculada, hold ? "HOLD":"LIVE");
  }
}