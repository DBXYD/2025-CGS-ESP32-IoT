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
#include "esp_wifi.h"          // <-- à ajouter


static const char *TAG = "main";

#define LED_PIN            GPIO_NUM_8
#define TEST_INTERVAL_MIN  1       // intervalle en minutes (pour test: 6 s)
#define BLINK_ON_MS        100
#define BLINK_PERIOD_MS    2000    // clignote toutes les 2 s
#define TOKEN_MARGIN_S     60      // rafraîchir 60 s avant expiration

static volatile bool event_active = false;

// Buffers pour OAuth
static char access_tok[512];
static char refresh_tok[256];
static uint64_t token_expiry_epoch = 0;

// -----------------------------------------------------------------------------
// Blink Task : clignote la LED si event_active == true
// -----------------------------------------------------------------------------
static void blink_task(void *arg)
{
    gpio_set_level(LED_PIN, 1);
    while (1) {
        if (event_active) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS - BLINK_ON_MS));
        } else {
            gpio_set_level(LED_PIN, 1);  // assure LED éteinte (1 ou 0 selon ta polarité)
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
// -----------------------------------------------------------------------------
// GCal Task : authentifie, interroge Calendar, rafraîchit le token
// -----------------------------------------------------------------------------
static void gcal_task(void *arg)
{
    /*  ✱✱✱  TAILLE DES BUFFERS  ✱✱✱
     * access_token : potentiellement >1 500 o → 2 048 pour être tranquille
     * refresh_token : ~200 o max
     */
    char access_tok[2048] = {0};
    char refresh_tok[256] = {0};
    int  expires_in       = 0;
    uint64_t token_expiry = 0;

    /* ---------------------------------------------------------------------
     * 1) Tentative de « refresh » immédiat si un refresh_token est déjà
     *    stocké en NVS ; sinon on démarre le Device-Code Flow.
     * ------------------------------------------------------------------ */
    if (load_refresh_token(refresh_tok, sizeof(refresh_tok)) &&
        oauth_refresh(refresh_tok, access_tok, &expires_in))
    {
        token_expiry = time(NULL) + expires_in;
        ESP_LOGI(TAG, "Refresh initial OK, valable %d s", expires_in);
    }
    else
    {
        char device[256], user_code[32], verify_url[128];
        int  poll_interval = 0;

        ESP_LOGI(TAG, "Démarrage Device-Code Flow");
        while (!oauth_get_device_code(device, user_code,
                                      verify_url, &poll_interval))
        {
            vTaskDelay(pdMS_TO_TICKS(5000));   // réessayer dans 5 s
        }

        ESP_LOGW(TAG,
                 "⚠️  Autorisation requise  ⚠️\n"
                 "Visitez : %s\n"
                 "Code    : %s",
                 verify_url, user_code);

        /* On poll tant que l’utilisateur n’a pas validé le code. */
        while (!oauth_poll_token(device, access_tok,
                                 &expires_in, refresh_tok))
        {
            vTaskDelay(pdMS_TO_TICKS(poll_interval * 1000));
        }
        ESP_LOGI(TAG, "Device-Flow terminé, token OK");

        /* 1ʳᵉ fois : on persiste le refresh-token (Google ne le renvoie qu’une fois) */
        if (refresh_tok[0]) {
            save_refresh_token(refresh_tok);  /* écrit UNE seule fois en NVS */
        }
        token_expiry = time(NULL) + expires_in;
    }

    /* ---------------------------------------------------------------------
     * 2) Boucle principale : refresh automatique + lecture du calendrier
     * ------------------------------------------------------------------ */
    for (;;)
    {
        uint64_t now = time(NULL);

        /* Rafraîchit le token 60 s avant son expiration ----------------- */
        if (now + TOKEN_MARGIN_S >= token_expiry)
        {
            if (oauth_refresh(refresh_tok, access_tok, &expires_in))
            {
                token_expiry = now + expires_in;
                ESP_LOGI(TAG, "Auto-refresh OK (+%d s)", expires_in);
                //save_refresh_token(refresh_tok);   // au cas où il changerait  commenter pour /* Pas de ré-écriture NVS : le refresh-token reste inchangé */
            }
            else
            {
                ESP_LOGW(TAG, "Auto-refresh KO, retry dans 60 s");
                vTaskDelay(pdMS_TO_TICKS(60000));
                continue;
            }
        }

        /* Lecture du planning du jour ----------------------------------- */
        char title[128] = "", loc[128] = "";
        time_t end_ts   = 0;

        bool found = calendar_check_today(access_tok,
                                          title, sizeof(title),
                                          loc,   sizeof(loc),
                                          &end_ts);
        event_active = found;

        if (found)
        {
            struct tm tm_end;
            localtime_r(&end_ts, &tm_end);
            ESP_LOGI(TAG, "Événement : \"%s\" @ %s  fin %02d:%02d",
                     title, loc, tm_end.tm_hour, tm_end.tm_min);
        }
        else
        {
            ESP_LOGI(TAG, "Aucun événement — nouveau test dans %d s",
                     TEST_INTERVAL_MIN * 6 /* 6 s pour le mode test */);
        }

        /* Pause --------------------------------------------------------- */
        vTaskDelay(pdMS_TO_TICKS(TEST_INTERVAL_MIN * 6000));
    }
}


void app_main(void)
{
    //ESP_ERROR_CHECK(nvs_flash_init());  <- commenter cette ligne pour ne pas redevoir se connecter au google device à chaque reboot
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    wifi_init();

    // config LED
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL<<LED_PIN,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 1);

    // créer les deux tâches
    xTaskCreate(blink_task, "blink", 2048, NULL, 5, NULL);
    xTaskCreate(gcal_task,  "gcal",  8192, NULL, 5, NULL);
}
