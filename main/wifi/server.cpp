#include "server.hpp"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h" // Biblioteca padrão de redes (Sockets) do ESP-IDF
#include "freertos/queue.h"
#include "cJSON.h" // Biblioteca nativa da ESP-IDF para ler o JSON
#include "driver/gpio.h"

#define PORTA_UDP 1234
gpio_num_t BOARD_LED = GPIO_NUM_2;

// Estrutura que guarda os comandos traduzidos
typedef struct {
    float eixoX;
    float eixoY;
} ComandoControle;

// A nossa "caixa de correio" entre o Server e o Motor
extern QueueHandle_t fila_controle;

void server_setup(void)
{
    //Configuração LED azul
    gpio_reset_pin(BOARD_LED);
    gpio_set_direction(BOARD_LED, GPIO_MODE_OUTPUT);
}

void iniciar_servidor_udp(void *pvParameters) {
    char buffer_recebimento[128]; // Espaço para guardar a mensagem recebida

    server_setup();

    while (1) {
        // 1. Configura o endereço IP e a Porta
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Ouve em qualquer IP da rede do carrinho
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORTA_UDP);

        // 2. Cria o Socket UDP
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE("UDP", "Erro ao criar socket. Tentando novamente...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // 3. Associa o socket à porta (Bind)
        bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        ESP_LOGW("UDP", "Servidor UDP rodando! Aguardando comandos na porta %d...", PORTA_UDP);

        // 4. Loop infinito recebendo dados1
        while (1) {
            struct sockaddr_storage source_addr;
            socklen_t socklen = sizeof(source_addr);
            
            int tamanho = 0;
            bool pacote_recebido = false;

            // =================================================================
            // DRENAGEM DE BUFFER: Lê e descarta os pacotes velhos num piscar de olhos
            // =================================================================
            while (1) {
                int lido = recvfrom(sock, buffer_recebimento, sizeof(buffer_recebimento) - 1, MSG_DONTWAIT, (struct sockaddr *)&source_addr, &socklen);
                if (lido > 0) {
                    tamanho = lido;             // Salva só o tamanho do mais novo
                    pacote_recebido = true;     
                } else {
                    break; // Fila vazia, sai do loop
                }
            }

            // =================================================================
            // PROCESSAMENTO: Lê apenas o JSON do último comando que sobreviveu
            // =================================================================
            if (pacote_recebido) {
                buffer_recebimento[tamanho] = 0; 
                
                cJSON *json = cJSON_Parse(buffer_recebimento);
                if (json != NULL) {
                    cJSON *itemX = cJSON_GetObjectItem(json, "eixoX");
                    cJSON *itemY = cJSON_GetObjectItem(json, "eixoY");

                    if (itemX != NULL && itemY != NULL) {
                        ComandoControle cmd;
                        cmd.eixoX = itemX->valuedouble;
                        cmd.eixoY = itemY->valuedouble;

                        xQueueOverwrite(fila_controle, &cmd);
                    }
                    cJSON_Delete(json); 
                }
            }

            // A placa dorme 20ms para zerar o Watchdog e não travar
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}