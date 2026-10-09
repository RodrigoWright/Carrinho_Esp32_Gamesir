#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "wifi.hpp"
#include "server.hpp"
#include "motor.hpp"
#include "freertos/queue.h"
#include "cJSON.h" // Biblioteca nativa da ESP-IDF para ler o JSON

int SERVO_PWM_PIN = 14;
#define SERVO_MIN_PULSEWIDTH 204  
#define SERVO_MAX_PULSEWIDTH 1024

bool state = false;

// Estrutura que guarda os comandos traduzidos
typedef struct {
    float eixoX;
    float eixoY;
} ComandoControle;

typedef struct {
    char mensagem[128];
    char type;
} MensagemLog;

// A nossa "caixa de correio" entre o Server e o Motor
QueueHandle_t fila_controle;

extern "C" void app_main(void)
{
    fila_controle = xQueueCreate(1, sizeof(ComandoControle));

    // CRIA O SEGUNDO PROCESSO:
    // 1º: A função que vai rodar
    // 2º: O nome da tarefa (para debug)
    // 3º: O tamanho da memória RAM reservada para ela (4096 bytes é excelente pro Wi-Fi)
    // 4º: Parâmetros extras (NULL)
    // 5º: Prioridade (5 é uma prioridade alta)
    // 6º: Identificador da tarefa (NULL)
    xTaskCreate(iniciar_wifi_task, "Task_WiFi", 4096, NULL, 1, NULL);

    xTaskCreate(iniciar_motor_task, "Task_Motor", 4096, NULL, 5, NULL);

    vTaskDelete(NULL); // Mata a task principal (app_main) para liberar memória
}
