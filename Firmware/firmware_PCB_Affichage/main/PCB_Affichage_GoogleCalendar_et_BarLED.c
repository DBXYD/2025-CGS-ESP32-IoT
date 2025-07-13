/* --------------  INCLUDES  -------------- */
#include "config.h"
#include "wifi.h"
#include "oauth.h"
#include "calendar.h"
#include "led_display.h"
#include "app_state.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <time.h>
#include <led_strip.h>


/* ---------------------------------------------------------------------------
   DÉFINES SPÉCIFIQUES À gcal_task()
   ------------------------------------------------------------------------- */
/* combien de secondes avant l’expiration du token on tente un refresh      */
#define TOKEN_MARGIN_S     60      /* 60 s = 1 min avant la fin              */

/* intervalle entre deux interrogations du calendrier                       */
/* (en minutes réelles -– en mode test on multiplie par 6 → secondes)       */
#define TEST_INTERVAL_MIN   1      /* 1 min (→ 6 s si ×6)                   */


/* --------------  DEFS/VAR  -------------- */
static const char *TAG = "main";

#define STRIP_PIN   5
#define STRIP_COUNT 96           /* 4×21 + 2 + 10 */

static led_strip_handle_t strip;

/* LED “on-board” pour clignoter en cas d’évènement Google */
#define LED_PIN            GPIO_NUM_8
#define BLINK_ON_MS        100
#define BLINK_PERIOD_MS    2000

#define TOKEN_MARGIN_S     60
#define TEST_INTERVAL_MIN   1
/* ------------------------------- */
/* --------------  TÂCHE GCAL  -------------- */
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
        // Valider le code sur google device pour accèder aux informations du calendar
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
        time_t start_ts = 0, end_ts = 0;

        bool found = calendar_check_today(access_tok,
                                          title, sizeof(title),
                                          loc,   sizeof(loc),
                                          &start_ts,
                                          &end_ts);
        event_active = found;

        if (found)
        {
            event_start_ts = start_ts;
            event_end_ts   = end_ts;

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
/* --------------  TÂCHE DISPLAY  -------------- */
static void display_task(void *arg)
{
    led_strip_handle_t s = arg;
    if (!s) vTaskDelete(NULL);

    for (;;)
    {
        /* -------- Heure et minutes -------- */
        time_t   now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);

        int h1 = tm_now.tm_hour / 10;
        int h0 = tm_now.tm_hour % 10;
        int m1 = tm_now.tm_min  / 10;
        int m0 = tm_now.tm_min  % 10;

        draw_digit(s, 0, h1, 255, 0, 0);    // red
        draw_digit(s, 1, h0, 255, 0, 0);
        draw_digit(s, 2, m1, 255, 0, 0);
        draw_digit(s, 3, m0, 255, 0, 0);

        /*  deux-points qui clignotent une seconde sur deux  */
        draw_colon(s, (tm_now.tm_sec & 1), 0, 0, 255);

        /* -------- Barre d’avancement -------- */
        if (!event_active) {
            /* aucun évènement → on éteint les 10 LEDs 95…86 */
            for (int i = 0; i < 10; ++i) {
                led_strip_set_pixel(s, 95 - i, 0, 0, 0);   // noir
            }
        } else {
            /* évènement en cours → calcul du ratio & couleurs */
            float ratio;
            time_t now_ts = now;

            if      (now_ts <  event_start_ts) ratio = 0.0f;
            else if (now_ts >= event_end_ts)   ratio = 1.0f;
            else  ratio = (float)(now_ts - event_start_ts) /
                        (float)(event_end_ts - event_start_ts);

            int led_done = (int)(ratio * 10 + 0.5f);       /* 0-10 */

            for (int i = 0; i < 10; ++i) {
                int idx   = 95 - i;            /* gauche → droite */
                bool done = (i < led_done);    /* déjà « passé » ? */

                /* done  → violet   |  à venir → vert  */
                led_strip_set_pixel(s, idx,
                                    done ? 40 : 0,     // R
                                    done ?  0 : 40,    // G
                                    done ? 40 : 0);    // B
            }
        }

        led_strip_refresh(s);
        vTaskDelay(pdMS_TO_TICKS(200));   // ≃5 FPS
    }
}

/* --------------  TÂCHE BLINK  -------------- */
static void blink_task(void *arg)
{
    gpio_set_level(LED_PIN, 1);
    for (;;)
    {
        if (event_active) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(BLINK_ON_MS));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS - BLINK_ON_MS));
        } else {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}


/* --------------  APP_MAIN  -------------- */
void app_main(void)
{
    /* --- WS2812 --- */
    led_strip_config_t cfg = {
        .strip_gpio_num = STRIP_PIN,
        .max_leds       = STRIP_COUNT,
        .led_model      = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt = {
        .clk_src       = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&cfg, &rmt, &strip));
    ESP_ERROR_CHECK(led_strip_clear(strip));

    /* --- Wi-Fi & SNTP --- */
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    wifi_init();

    /* --- GPIO LED --- */
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);

    /* --- Tâches --- */
    xTaskCreate(blink_task,    "blink",   2048, NULL, 5, NULL);
    xTaskCreate(gcal_task,     "gcal",    8192, NULL, 5, NULL);
    xTaskCreate(display_task,  "display", 4096, strip, 4, NULL);
}
