/*  firmware_PCB_repetition.c  –  « Rack v2 »  (ESP32-C3 SuperMini)
 *  ────────────────────────────────────────────────────────────────
 *  Boucle principale :
 *    • toutes les  5 s : requête « order »  (power + route)
 *    • heartbeat JSON   : toutes les 10 s, si aucune séquence en cours
 *
 *  Dépendances :
 *      rack_ctrl.[ch]    ▸ pilotage local (séquences, route, masque)
 *      api_client.[ch]   ▸ api_get_esp_order() / api_send_ping_full()
 *      wifi.[ch]         ▸ connexion STA + SNTP
 *      config.h          ▸ SSID, URLs, STUDIO_NAME …
 */
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "mdns.h"

#include "wifi.h"
#include "rack_ctrl.h"
#include "api_client.h"
#include "config.h"

static const char *TAG = "main";

/*──────────────────────── 1. Fuseau horaire/SNTP ────────────────────────*/
static void init_timezone(void)
{
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);   /* Europe/Paris */
    tzset();
}

/*──────────────────────── 2. Boucle principale ─────────────────────────*/
void app_main(void)
{
    init_timezone();

    /* — logs — */
    esp_log_level_set("*",    ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_INFO);

    /* — Wi-Fi + SNTP — */
    wifi_init();

    /* — Sous-système rack (I²C, GPIO, LED, ISR…) — */
    rack_ctrl_init();

    /* — mDNS (optionnel) — */
    ESP_ERROR_CHECK(mdns_init());
    mdns_hostname_set("esp32-rackv2");
    mdns_instance_name_set("ESP32 Rack v2");

    /* — boucle — */
    int64_t last_hb_us = 0;                           /* heartbeat µs */

    while (true) {

        /* 1) Consigne serveur (5 s) ------------------------------------ */
        bool         want_on;
        route_mode_t want_route;

        if (!rack_ctrl_is_busy() &&
            api_get_esp_order(&want_on, &want_route))
        {
            /* Puissance ------------------------------------------------*/
            if ( want_on && !rack_ctrl_is_on()) rack_ctrl_sequence_on();
            if (!want_on &&  rack_ctrl_is_on()) rack_ctrl_sequence_off();

            /* Routage A/B ---------------------------------------------*/
            if (want_route == BUS_AB && rack_ctrl_route_get() == DIRECT)
                rack_ctrl_route_bus_ab();
            else if (want_route == DIRECT && rack_ctrl_route_get() == BUS_AB)
                rack_ctrl_route_direct();
        }

        /* 2) Heart-beat JSON (10 s) ------------------------------------*/
        int64_t now = esp_timer_get_time();           /* µs depuis boot */

        if ((now - last_hb_us) > 10LL * 1000 * 1000     /* >10 s */
            && !rack_ctrl_is_busy())
        {
            api_send_ping_full(
                rack_ctrl_is_on(),        /* état global ON/OFF  */
                rack_ctrl_get_mask(),     /* bits AC 0-2         */
                rack_ctrl_route_get()     /* DIRECT ou BUS_AB    */
            );
            last_hb_us = now;
        }

        /* 3) Tempo de boucle (5 s) ------------------------------------*/
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
