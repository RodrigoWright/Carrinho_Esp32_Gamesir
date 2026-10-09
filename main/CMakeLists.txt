idf_component_register(SRCS "main.cpp" "wifi/wifi.cpp" "wifi/server.cpp" "motor/motor.cpp"
                       REQUIRES spi_flash driver esp_wifi nvs_flash esp_event esp_netif lwip json
                       INCLUDE_DIRS "." "wifi" "motor")
