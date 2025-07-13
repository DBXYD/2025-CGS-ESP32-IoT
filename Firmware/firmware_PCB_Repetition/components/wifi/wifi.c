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

/* ────────────── Callbacks Wi-Fi / IP ────────────── */
static void on_wifi_event(void *arg, esp_event_base_t base,
                          int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi démarré, connexion en cours…");
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = (wifi_event_sta_disconnected_t *)data;
        ESP_LOGW(TAG, "Wi-Fi déconnecté (reason=%d), reconnexion…", d->reason);
        esp_wifi_connect();

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Adresse IP reçue : " IPSTR, IP2STR(&evt->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ────────────── SNTP ────────────── */
static void sntp_sync(void)
{
    ESP_LOGI(TAG, "Initialisation SNTP (%s)", SNTP_SERVER);
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_init();

    /* on attend que l’horloge soit > 01/01/2021 */
    time_t now = 0;  struct tm tm = {0};  int retry = 0;
    do {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);  localtime_r(&now, &tm);
    } while (tm.tm_year < (2021-1900) && ++retry < 10);

    if (tm.tm_year < (2021-1900))
        ESP_LOGW(TAG, "SNTP KO après %d essais", retry);
    else
        ESP_LOGI(TAG, "SNTP OK : %s", asctime(&tm));
}

/* ────────────── Initialisation Wi-Fi ────────────── */
void wifi_init(void)
{
    /* NVS */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* TCP/IP + events */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* Wi-Fi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* callbacks */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, NULL, NULL));

    /* credentials */
    wifi_config_t wcfg = {0};
    strncpy((char *)wcfg.sta.ssid,     WIFI_SSID, sizeof(wcfg.sta.ssid));
    strncpy((char *)wcfg.sta.password, WIFI_PASS, sizeof(wcfg.sta.password));
    wcfg.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wcfg.sta.pmf_cfg.capable  = true;
    wcfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));

    /* ► désactivation explicite du power-save ◄ */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    /* démarrage + attente IP */
    wifi_event_group = xEventGroupCreate();
    ESP_LOGI(TAG, "Démarrage du Wi-Fi STA…");
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT)
        ESP_LOGI(TAG, "Wi-Fi connecté avec succès");
    else
        ESP_LOGW(TAG, "Timeout de connexion Wi-Fi");

    sntp_sync();
}
