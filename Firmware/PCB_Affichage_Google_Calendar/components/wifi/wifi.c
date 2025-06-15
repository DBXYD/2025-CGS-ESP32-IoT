#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "config.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi";
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
static int s_retry = 0;

/**
 * @brief Handler pour les événements Wi-Fi et IP
 */
static void on_wifi_event(void *arg, esp_event_base_t base,
                          int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi démarré, connexion en cours...");
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGW(TAG, "Wi-Fi déconnecté (reason=%d), tentative de reconnexion...", 
             ((wifi_event_sta_disconnected_t*)data)->reason);
    esp_wifi_connect();
    // Remarque : on n’incrémente plus s_retry ni on n’arrête jamais les essais
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t*) data;
        ESP_LOGI(TAG, "Adresse IP reçue : " IPSTR, IP2STR(&evt->ip_info.ip));
        s_retry = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Synchronisation SNTP
 */
static void sntp_sync(void)
{
    ESP_LOGI(TAG, "Initialisation SNTP (serveur %s)", SNTP_SERVER);
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_init();

    /* On attend que la date soit > 1er Janvier 2021 */
    time_t now = 0;
    struct tm tm = { 0 };
    int retries = 0;
    do {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &tm);
        retries++;
    } while ((tm.tm_year < (2021 - 1900)) && retries < 10);

    if (tm.tm_year < (2021 - 1900)) {
        ESP_LOGW(TAG, "Échec synchronisation SNTP après %d essais", retries);
    } else {
        ESP_LOGI(TAG, "SNTP synchronisation OK : %s", asctime(&tm));
    }
}

/**
 * @brief Initialise le Wi-Fi en mode Station et attend la connexion
 */
void wifi_init(void)
{
    // 1) NVS (nécessaire pour driver Wi-Fi et stockage SNTP)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 2) TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

    // 3) Event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 4) Création de l'interface Wi-Fi par défaut (station)
    esp_netif_create_default_wifi_sta();

    // 5) Initialisation Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 6) Enregistrement du handler d’événements Wi-Fi et IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, NULL, NULL));

    // 7) Configuration des credentials
    wifi_config_t wifi_cfg = { 0 };
    strncpy((char*)wifi_cfg.sta.ssid, WIFI_SSID, sizeof(wifi_cfg.sta.ssid));
    strncpy((char*)wifi_cfg.sta.password, WIFI_PASS, sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    // 8) Création de l’EventGroup pour attendre la connexion
    wifi_event_group = xEventGroupCreate();

    // 9) Démarrage
    ESP_LOGI(TAG, "Démarrage du Wi-Fi STA...");
    ESP_ERROR_CHECK(esp_wifi_start());

    // 10) Attente de la connexion (timeout possible)
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(10000)
    );
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi connecté avec succès");
    } else {
        ESP_LOGW(TAG, "Timeout de connexion Wi-Fi");
    }

    // 11) Synchronisation temporelle
    sntp_sync();
}
