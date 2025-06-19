idf_component_register(
    SRCS        "PCB_Affichage_GoogleCalendar_et_BarLED.c"
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
        led_strip   ## pour controler les neopixels
        app_state 
)
