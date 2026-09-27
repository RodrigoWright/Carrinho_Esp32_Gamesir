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
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"

gpio_num_t GPIO_LED = GPIO_NUM_2;
bool state = false;

void setup(void)
{
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
};

void blink(void)
{
    gpio_set_level(GPIO_LED, !state);
}

extern "C" void app_main(void)
{
    setup();
    printf("Hello world!\n");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        blink();
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
