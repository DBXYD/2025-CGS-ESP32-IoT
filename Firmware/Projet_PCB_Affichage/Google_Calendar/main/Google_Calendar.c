#include "config.h"
#include "wifi.h"
#include "oauth.h"
#include "calendar.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include "esp_err.h"

static const char *TAG = "main";
static const char *TAG_OAUTH = "oauth";  // si jamais tu utilises des logs OAuth ici

static void gcal_task(void *arg) {
    char refresh_tok[256] = {0};
    char access_tok[2048] = {0};
    int expires = 0;

    for (;;) {
        // Charge le token de rafraîchissement ou lance le device flow
        if (!load_refresh_token(refresh_tok, sizeof(refresh_tok))) {
            char device_code[256];
            char user_code[32];
            char verify_url[128];
            int interval = 0;

            ESP_LOGI(TAG, "Démarrage du Device Flow OAuth2...");
            if (!oauth_get_device_code(device_code, user_code, verify_url, &interval)) {
                ESP_LOGE(TAG, "Échec obtention device_code");
                vTaskDelay(pdMS_TO_TICKS(10000));
                continue;
            }
            ///Affichage connecxion
            ESP_LOGW(TAG, "\n"
                        "========================================\n"
                        "     ⚠️  AUTHENTIFICATION REQUISE  ⚠️    \n"
                        "----------------------------------------\n"
                        "Visitez : %s\n"
                        "Code    : %s\n"
                        "========================================\n",
                        verify_url, user_code
                    );


            // Poll jusqu'à récupération du token
            while (!oauth_poll_token(device_code, access_tok, &expires, refresh_tok)) {
                ESP_LOGI(TAG, "En attente token (intervalle %ds)...", interval);
                vTaskDelay(pdMS_TO_TICKS(interval * 1000));
            }
            save_refresh_token(refresh_tok);

        } else {
            // Rafraîchit le token d'accès
            if (!oauth_refresh(refresh_tok, access_tok, &expires)) {
                ESP_LOGW(TAG, "Refresh token invalide, purge et ré-auth");
                save_refresh_token("");
                continue;
            }
        }

        // Boucle principale : fetch + refresh périodique
        while (true) {
            esp_err_t err = calendar_fetch_and_print(access_tok);
            if (err == ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "Token expiré, purge et retour au flow");
                save_refresh_token("");
                break;
            }
            // Attendre avant le prochain fetch
            int sleep_s = (expires > 600) ? expires - 300 : 60;
            ESP_LOGI(TAG, "Prochain fetch dans %ds", sleep_s);
            vTaskDelay(pdMS_TO_TICKS(sleep_s * 1000));
            // Rafraîchit le token en fond
            if (!oauth_refresh(refresh_tok, access_tok, &expires)) {
                ESP_LOGW(TAG, "Impossible de rafraîchir le token, purge");
                save_refresh_token("");
                break;
            }
        }
    }
}

void app_main(void) {
    // Initialise NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Réglage du fuseau horaire
    setenv("TZ", "Europe/Paris", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone réglée sur Europe/Paris");

    // Initialisation du Wi-Fi et SNTP
    wifi_init();

    // Création de la tâche Google Calendar
    xTaskCreatePinnedToCore(
        gcal_task,
        "gcal",
        32768,
        NULL,
        5,
        NULL,
        0
    );
}