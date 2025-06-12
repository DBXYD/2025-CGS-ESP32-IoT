#include "config.h"
#include "wifi.h"
#include "oauth.h"
#include "calendar.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <time.h>

static const char *TAG = "main";

#define LED_PIN       GPIO_NUM_8
#define TEST_INTERVAL_MIN    1    // intervalle de test en minutes
#define BLINK_ON_MS          100
#define BLINK_PERIOD_MS     2000  // clignotement toutes les 2s

// flag partagé entre les tâches
static volatile bool event_active = false;

// Tâche qui clignote la LED si event_active vaut true
static void blink_task(void *arg)
{
    while (1) {
        if (event_active) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS - BLINK_ON_MS));
        } else {
            // LED bien éteinte quand pas d'événement
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// Tâche qui interroge Google Calendar toutes les TEST_INTERVAL_MIN minutes
static void gcal_task(void *arg)
{
    char refresh_tok[256] = {0};
    char access_tok[2048] = {0};
    int  expires = 0;

    // 1) Authentification OAuth2 (device flow)
    for (;;) {
        if (!load_refresh_token(refresh_tok, sizeof(refresh_tok))) {
            char device[256], user[32], url[128];
            int  interval = 0;
            ESP_LOGI(TAG, "Start OAuth2 device flow");
            if (!oauth_get_device_code(device, user, url, &interval)) {
                ESP_LOGE(TAG, "oauth_get_device_code() failed");
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
            ESP_LOGW(TAG,
                "⚠️ AUTH REQ ⚠️\n"
                "Visitez : %s\n"
                "Code    : %s\n",
                url, user
            );
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

    // 2) Boucle de test
    while (1) {
        char evt_name[64], evt_loc[64];
        time_t end_ts;

        bool found = calendar_check_today(
            access_tok,
            evt_name, sizeof(evt_name),
            evt_loc,  sizeof(evt_loc),
            &end_ts
        );

        event_active = found;

        if (found) {
            struct tm tm_end;
            char hfin[6];
            localtime_r(&end_ts, &tm_end);
            strftime(hfin, sizeof(hfin), "%H:%M", &tm_end);
            ESP_LOGW(TAG, "➤ Événement en cours : %s @ %s jusqu'à %s",
                     evt_name, evt_loc, hfin);
        } else {
            ESP_LOGI(TAG, "Pas d'événement en cours, je reteste dans %d min",
                     TEST_INTERVAL_MIN);
        }
                            /// temps en ms :
        vTaskDelay(pdMS_TO_TICKS(TEST_INTERVAL_MIN * 6 * 1000)); /// Attention, pour faciliter les tests j'ai remplacé 60 par 6. à remettre pour avoir l'intervalle en minute.
    }
}

void app_main(void)
{
    // 1) NVS
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    // 2) Fuseau horaire
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone CET/CEST");

    // 3) Wi-Fi + SNTP
    wifi_init();

    // 4) Config LED
    gpio_config_t io_conf = {
        .pin_bit_mask   = (1ULL << LED_PIN),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 1);

    // 5) Lancer les 2 tâches
    xTaskCreatePinnedToCore(blink_task, "blink", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(gcal_task,  "gcal",  8192, NULL, 5, NULL, 0);
}
