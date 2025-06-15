idf_component_register(
    SRCS        "Google_Calendar.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver           # GPIO
        nvs_flash        # déjà apporté par esp-idf
        esp_wifi         # driver Wi-Fi officiel
        http_client
        wifi             # ton wrapper wifi_init()
        oauth
        calendar
        time_utils
        config
)
