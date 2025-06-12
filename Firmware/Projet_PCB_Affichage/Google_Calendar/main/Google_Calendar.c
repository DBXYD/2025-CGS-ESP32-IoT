
#include "config.h"
#include "wifi.h"
#include "oauth.h"
#include "calendar.h"     // contient la déclaration de calendar_check_today()
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"  // pour gpio_set_level()
#include <time.h>

static const char *TAG = "main";
#define LED_PIN      GPIO_NUM_8    // ← votre LED
#define BLINK_MS     200           // durée ON/OFF en ms
#define BLINK_COUNT  5

#define LED_ON_LEVEL   0  // active-low : 0 pour allumer
#define LED_OFF_LEVEL  1  // 1 pour éteindre




static void gcal_task(void *arg)
{
    char refresh_tok[256] = {0};
    char access_tok[2048] = {0};
    int  expires = 0;

    // 1) authentification OAuth2
    for (;;) {
        if (!load_refresh_token(refresh_tok, sizeof(refresh_tok))) {
            // Device Flow...
            char device[256], user[32], url[128];
            int  interval = 0;
            ESP_LOGI(TAG, "Start OAuth2 device flow");
            if (!oauth_get_device_code(device, user, url, &interval)) {
                ESP_LOGE(TAG, "oauth_get_device_code() failed");
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
            ESP_LOGW(TAG,
                "\n========================================\n"
                "  ⚠️ AUTHENTIFICATION REQUISE ⚠️\n"
                "----------------------------------------\n"
                "Visitez : %s\n"
                "Code    : %s\n"
                "========================================\n",
                url, user
            );
            // polling
            while (!oauth_poll_token(device, access_tok, &expires, refresh_tok)) {
                vTaskDelay(pdMS_TO_TICKS(interval * 1000));
            }
            save_refresh_token(refresh_tok);
        } else {
            if (!oauth_refresh(refresh_tok, access_tok, &expires)) {
                ESP_LOGW(TAG, "Refresh token invalide, purge et retry");
                save_refresh_token("");
                continue;
            }
        }
        break;
    }

    // 2) boucle de vérification toutes les 5 minutes
    while (1) {
        char evt_name[64];
        char evt_loc[64];
        time_t end_ts;

        // 1) Est-ce qu’un événement est en cours AUJOURD’HUI ?
        if (calendar_check_today(
                access_tok,
                evt_name, sizeof(evt_name),
                evt_loc,  sizeof(evt_loc),
                &end_ts
            ))
        {
            // Formatage de l’heure de fin pour le log
            struct tm tm_end;
            char hfin[6];
            localtime_r(&end_ts, &tm_end);
            strftime(hfin, sizeof(hfin), "%H:%M", &tm_end);

            ESP_LOGW(TAG, "➤ Événement en cours : %s @ %s jusqu'à %s",
                     evt_name, evt_loc, hfin);

            // 2) Tant que l’événement n’est pas terminé, on clignote toutes les 2 s
            while (time(NULL) < end_ts) {
                // LED on
                gpio_set_level(LED_PIN, LED_ON_LEVEL);
                vTaskDelay(pdMS_TO_TICKS(100));
                // LED off
                gpio_set_level(LED_PIN, LED_OFF_LEVEL);
                vTaskDelay(pdMS_TO_TICKS(1900));
            }
            // À la fin de l’événement, on s’assure que la LED reste éteinte
            gpio_set_level(LED_PIN, LED_OFF_LEVEL);

        } else {
            // Pas d’événement en cours : LED éteinte et on attend 5 minutes
            gpio_set_level(LED_PIN, LED_OFF_LEVEL);
            ESP_LOGI(TAG, "Pas d'événement en cours, je reteste dans 1 min");
            vTaskDelay(pdMS_TO_TICKS(1 * 60 * 1000));
        }
    }
}

void app_main(void)
{

    // NVS
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    // timezone
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    ESP_LOGI(TAG, "Fuseau horaire réglé en CET/CEST"); 

    // Wi-Fi + SNTP
    wifi_init();

    // LED
    // LED (GPIO8) : output sans pull-up ni pull-down
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(LED_PIN, LED_OFF_LEVEL);


    // démarrage
    xTaskCreatePinnedToCore(
        gcal_task, "gcal", 8*1024, NULL, 5, NULL, 0
    );
}