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

    while (1) {
        bool turn_on = false;
        if (api_get_esp_status(&turn_on)) {
            gpio_set_level(LED_PIN, turn_on ? 0 : 1);
            ESP_LOGI(TAG, "LED %s", turn_on ? "ON" : "OFF");
        }

        api_send_ping();                     // <— ping même si le GET a échoué
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
