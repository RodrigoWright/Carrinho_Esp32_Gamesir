idf_component_register(SRCS "main.cpp" "wifi/wifi.cpp"
                       REQUIRES spi_flash esp_driver_gpio esp_driver_ledc esp_wifi nvs_flash esp_event esp_netif
                       INCLUDE_DIRS ".")
