idf_component_register(
    SRCS "code_ESP_commande_par_le_site.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver           # GPIO
        nvs_flash        # déjà apporté par esp-idf
        esp_wifi         # driver Wi-Fi officiel
        http_client
        wifi             # ton wrapper wifi_init()
        time_utils
        config
        led_strip   ## pour controler les neopixels
        api_client
        mdns
    )
