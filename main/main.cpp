/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "wifi.hpp"
#include "server.hpp"

gpio_num_t BOARD_LED = GPIO_NUM_2;
int SERVO_PWM_PIN = 14;
#define SERVO_MIN_PULSEWIDTH 204  
#define SERVO_MAX_PULSEWIDTH 1024

bool state = false;

void setup(void)
{
    //Configuração LED azul
    gpio_reset_pin(BOARD_LED);
    gpio_set_direction(BOARD_LED, GPIO_MODE_OUTPUT);

    // 1. Configurar o Temporizador do PWM (Frequência e Resolução)
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution  = LEDC_TIMER_13_BIT; // 8192 fatias por ciclo
    ledc_timer.timer_num        = LEDC_TIMER_0;
    ledc_timer.freq_hz          = 50;                // Servos operam estritamente em 50Hz
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    
    ledc_timer_config(&ledc_timer);

    // 2. Configurar o Canal do PWM (Vincular o temporizador ao Pino)
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = SERVO_PWM_PIN;
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = LEDC_CHANNEL_0;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.duty           = 0;
    ledc_channel.hpoint         = 0;

    ledc_channel_config(&ledc_channel);
};

void blink(void)
{
    state = !state;
    gpio_set_level(BOARD_LED, state);
}

extern "C" void app_main(void)
{
    setup();
    printf("Hello world!\n");

    // CRIA O SEGUNDO PROCESSO:
    // 1º: A função que vai rodar
    // 2º: O nome da tarefa (para debug)
    // 3º: O tamanho da memória RAM reservada para ela (4096 bytes é excelente pro Wi-Fi)
    // 4º: Parâmetros extras (NULL)
    // 5º: Prioridade (5 é uma prioridade alta)
    // 6º: Identificador da tarefa (NULL)
    xTaskCreate(iniciar_wifi_task, "Task_WiFi", 4096, NULL, 1, NULL);

    while (1) {
        printf("Servo: Movendo para Mínimo (0 graus)\n");
        // Ajusta a largura do pulso e aplica na porta
        gpio_set_level(BOARD_LED, false);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_MIN_PULSEWIDTH);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(2000 / portTICK_PERIOD_MS); // Espera 2 segundos

        printf("Servo: Movendo para Máximo (180 graus)\n");
        gpio_set_level(BOARD_LED, true);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_MAX_PULSEWIDTH);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
