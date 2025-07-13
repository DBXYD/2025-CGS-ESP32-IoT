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
#include "esp_timer.h"
#include "mdns.h"
#include "rack_ctrl.h"


static const char *TAG = "main";




void app_main(void) {
    setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1);  tzset();

    /* — niveaux de log — */
    esp_log_level_set("*", ESP_LOG_INFO);        // tout en INFO le temps du debug
    esp_log_level_set("wifi", ESP_LOG_INFO);
    esp_log_level_set("wifi_init", ESP_LOG_INFO);

    /* — Wi-Fi — */
    wifi_init();   // < les logs détaillés vont s’afficher
    rack_ctrl_init();
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip) == ESP_OK && ip.ip.addr) {
        ESP_LOGI(TAG, "IP déjà attribuée = " IPSTR, IP2STR(&ip.ip));
    }
    esp_log_level_set("wifi", ESP_LOG_INFO);   // log détaillé

    
    /* --- mDNS --- */
    ESP_ERROR_CHECK(mdns_init());
    mdns_hostname_set("esp32");          // optionnel, visible sur le LAN
    mdns_instance_name_set("ESP32 Supernova");

    /* ------------- */
    int64_t last_hb_us = 0;          // time() en micro-secondes

    bool desired_on = false;   // l'état de la LED

    while (1) {
        /* 1) lecture de la consigne toutes les 5 s */
        if (!rack_ctrl_is_busy()) {
            bool turn_on;
            if (api_get_esp_status(&turn_on)) {
                if (turn_on && !rack_ctrl_is_on())  rack_ctrl_sequence_on();
                if (!turn_on && rack_ctrl_is_on())  rack_ctrl_sequence_off();
            }
        }

        /* 2) heartbeat toutes les 10 s */
        int64_t now = esp_timer_get_time();         // µs depuis boot
        if (now - last_hb_us > 10 * 1000 * 1000LL   // 10 s
            && !rack_ctrl_is_busy())
        {
            api_send_ping_state(rack_ctrl_is_on()); // {name, state:true/false}
            last_hb_us = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));            // boucle 5 s
    }
}

