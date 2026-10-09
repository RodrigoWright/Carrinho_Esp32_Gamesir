#include "motor.hpp"
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern QueueHandle_t fila_log;

void log_carrinho(const char* mensagem) {
    while(1) {
        if(xQueueReceive(fila_log, (void*)mensagem, pdMS_TO_TICKS(2000) == pdTRUE)) {
            ESP_LOGI("LOG_CARRINHO", "%s", mensagem);
        }
        else {
            ESP_LOGE("LOG_CARRINHO", "Erro ao receber mensagem da fila de log.");
        }
        
    }
}