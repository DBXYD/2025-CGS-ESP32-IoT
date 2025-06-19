#include "wifi.h"
#include "api_client.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

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

    while (1) {
        bool turn_on = false;
        if (api_get_esp_status(&turn_on)) {
            gpio_set_level(LED_PIN, turn_on ? 0 : 1);
            ESP_LOGI(TAG, "ESP => LED %s", turn_on ? "ON" : "OFF");
        } else {
            ESP_LOGW(TAG, "Pas de commande valide reçue.");
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Check toutes les 5 secondes
    }
}
