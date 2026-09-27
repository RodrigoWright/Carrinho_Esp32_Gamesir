#include "wifi.hpp"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Define o nome e senha da rede do seu carrinho
#define NOME_REDE_CARRINHO "Carrinho_Gamesir"
#define SENHA_REDE_CARRINHO "12345678" // Mínimo de 8 caracteres

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        printf("Celular conectou no carrinho! MAC do celular: " MACSTR "\n", MAC2STR(event->mac));
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        printf("Celular desconectou do carrinho. Pare os motores por segurança!\n");
    }
}

void iniciar_wifi_task(void *pvParameters) {
    printf("Iniciando task de wifi\n");
    // Inicializa a memória Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      nvs_flash_erase();
      nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    
    // Cria o Wi-Fi no modo AP (Access Point)
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    // Configura os parâmetros da sua rede
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, NOME_REDE_CARRINHO);
    strcpy((char*)wifi_config.ap.password, SENHA_REDE_CARRINHO);
    wifi_config.ap.ssid_len = strlen(NOME_REDE_CARRINHO);
    wifi_config.ap.max_connection = 2; // Só permite até 2 aparelhos conectados
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    // Inicia a antena como Access Point
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    printf("Rede Wi-Fi '%s' criada com sucesso! IP do carrinho: 192.168.4.1\n", NOME_REDE_CARRINHO);

    vTaskDelete(NULL);
}