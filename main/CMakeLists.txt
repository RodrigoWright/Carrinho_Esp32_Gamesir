idf_component_register(SRCS "main.cpp" "wifi/wifi.cpp" "wifi/server.cpp"
                       REQUIRES spi_flash esp_driver_gpio esp_driver_ledc esp_wifi nvs_flash esp_event esp_netif lwip
                       INCLUDE_DIRS "." "wifi")
