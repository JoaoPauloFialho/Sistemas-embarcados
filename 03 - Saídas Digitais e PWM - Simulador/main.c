#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

// Mapeamento de Pinos
#define LED_YELLOW_GPIO 4
#define LED_BLUE_GPIO   5
#define LED_RED_GPIO    6
#define LED_GREEN_GPIO  7
#define BUZZER_GPIO     1

// Configurações do LEDC
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT 
#define MAX_DUTY                8191              
#define LED_FREQ_HZ             1000              
#define BUZZER_START_FREQ       1000              

// Timers
#define LEDC_LED_TIMER          LEDC_TIMER_0
#define LEDC_BUZZER_TIMER       LEDC_TIMER_1

// Canais
#define CH_LED_YELLOW           LEDC_CHANNEL_0
#define CH_LED_BLUE             LEDC_CHANNEL_1
#define CH_LED_RED              LEDC_CHANNEL_2
#define CH_LED_GREEN            LEDC_CHANNEL_3
#define CH_BUZZER               LEDC_CHANNEL_4

// Parâmetros Configuráveis de Tempo
#define DELAY_FADE_MS           15
#define DELAY_PHASE_MS          500

const ledc_channel_t led_channels[] = {CH_LED_YELLOW, CH_LED_BLUE, CH_LED_RED, CH_LED_GREEN};

void set_led_duty(ledc_channel_t channel, uint8_t percentage) {
    if (percentage > 100) percentage = 100;
    
    uint32_t duty_calc = MAX_DUTY - ((percentage * MAX_DUTY) / 100);
    
    ledc_set_duty(LEDC_MODE, channel, duty_calc);
    ledc_update_duty(LEDC_MODE, channel);
}

void set_buzzer_freq(uint32_t freq) {
    if (freq == 0) {
        ledc_set_duty(LEDC_MODE, CH_BUZZER, 0);
        ledc_update_duty(LEDC_MODE, CH_BUZZER);
    } else {
        ledc_set_freq(LEDC_MODE, LEDC_BUZZER_TIMER, freq);
        ledc_set_duty(LEDC_MODE, CH_BUZZER, MAX_DUTY / 2);
        ledc_update_duty(LEDC_MODE, CH_BUZZER);
    }
}

void init_ledc_config(void) {
    ledc_timer_config_t led_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_LED_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LED_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&led_timer);

    ledc_timer_config_t buzzer_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_BUZZER_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = BUZZER_START_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    int gpios[] = {LED_YELLOW_GPIO, LED_BLUE_GPIO, LED_RED_GPIO, LED_GREEN_GPIO};
    
    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t led_conf = {
            .speed_mode     = LEDC_MODE,
            .channel        = led_channels[i],
            .timer_sel      = LEDC_LED_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = gpios[i],
            .duty           = MAX_DUTY,
            .hpoint         = 0
        };
        ledc_channel_config(&led_conf);
    }

    ledc_channel_config_t buzzer_conf = {
        .speed_mode     = LEDC_MODE,
        .channel        = CH_BUZZER,
        .timer_sel      = LEDC_BUZZER_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BUZZER_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&buzzer_conf);
}

void app_main(void) {
    init_ledc_config();
    vTaskDelay(pdMS_TO_TICKS(1000));

    for(;;) {
        printf("Iniciando Fase 1: Fading Sincronizado\n");
        for (int p = 0; p <= 100; p += 2) {
            for (int i = 0; i < 4; i++) set_led_duty(led_channels[i], p);
            vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
        }
        for (int p = 100; p >= 0; p -= 2) {
            for (int i = 0; i < 4; i++) set_led_duty(led_channels[i], p);
            vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY_PHASE_MS));

        printf("Iniciando Fase 2: Fading Sequencial\n");
        for (int i = 0; i < 4; i++) {
            for (int p = 0; p <= 100; p += 5) {
                set_led_duty(led_channels[i], p);
                vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
            }
            for (int p = 100; p >= 0; p -= 5) {
                set_led_duty(led_channels[i], p);
                vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY_PHASE_MS));

        printf("Iniciando Fase 3: Teste Sonoro (500Hz a 2000Hz)\n");
        for (int f = 500; f <= 2000; f += 20) {
            set_buzzer_freq(f);
            vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
        }
        for (int f = 2000; f >= 500; f -= 20) {
            set_buzzer_freq(f);
            vTaskDelay(pdMS_TO_TICKS(DELAY_FADE_MS));
        }
        set_buzzer_freq(0); 
        
        printf("Ciclo completo. Reiniciando...\n\n");
        vTaskDelay(pdMS_TO_TICKS(DELAY_PHASE_MS * 2));
    }
}