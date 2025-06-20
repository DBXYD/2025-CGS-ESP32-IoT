#include "wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_client.h"   
#include "config.h"  
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "api_client.h"
#include "mdns.h"



#define LED_PIN GPIO_NUM_8
static const char *TAG = "main";






void app_main(void) {
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    wifi_init();

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    /* --- mDNS --- */
    ESP_ERROR_CHECK(mdns_init());
    mdns_hostname_set("esp32");          // optionnel, visible sur le LAN
    mdns_instance_name_set("ESP32 Supernova");

    /* ------------- */

    bool led_on = false;   // l'état de la LED

    while (1) {
        /* 1) on lit l’ordre dans la BD -------------------------------- */
        if (api_get_esp_status(&led_on)) {

        /* 2) on applique sur le GPIO ------------------------------ */
        gpio_set_level(LED_PIN, led_on ? 0 : 1);

        /* 3) on laisse 80 ms au hard puis on confirme au serveur --- */
        vTaskDelay(pdMS_TO_TICKS(80));
        api_send_ping(led_on);       // ← ici led_on, pas turn_on

        }
    /* 4) on recommence dans 5 s ----------------------------------- */
    vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

