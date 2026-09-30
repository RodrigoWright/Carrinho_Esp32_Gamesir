#include "server.hpp"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h" // Biblioteca padrão de redes (Sockets) do ESP-IDF

#define PORTA_UDP 3333

void iniciar_servidor_udp(void *pvParameters) {
    char buffer_recebimento[128]; // Espaço para guardar a mensagem recebida

    while (1) {
        // 1. Configura o endereço IP e a Porta
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Ouve em qualquer IP da rede do carrinho
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORTA_UDP);

        // 2. Cria o Socket UDP
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Erro ao criar socket. Tentando novamente...\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // 3. Associa o socket à porta (Bind)
        bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        printf("Servidor UDP rodando! Aguardando comandos na porta %d...\n", PORTA_UDP);

        // 4. Loop infinito recebendo dados
        while (1) {
            struct sockaddr_storage source_addr;
            socklen_t socklen = sizeof(source_addr);
            
            // O código "pausa" nesta linha até o celular enviar algo
            int tamanho = recvfrom(sock, buffer_recebimento, sizeof(buffer_recebimento) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            if (tamanho > 0) {
                buffer_recebimento[tamanho] = 0; // Coloca um terminador de string no final
                printf("Comando recebido do celular: %s\n", buffer_recebimento);
                
                // Exemplo de como você vai acionar os motores futuramente:
                // if (strcmp(buffer_recebimento, "FRENTE") == 0) { motor_frente(); }
                // else if (strcmp(buffer_recebimento, "ESQUERDA") == 0) { servo_esquerda(); }
            }
        }
    }
}