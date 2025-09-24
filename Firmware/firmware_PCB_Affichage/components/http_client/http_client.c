#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "http";

typedef struct {
    char *body;
    int len;
    int cap;
} response_t;

static esp_err_t on_data(esp_http_client_event_t *evt)
{
    response_t *resp = evt->user_data;
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0) {
            if (resp->cap == 0) {
                resp->cap = 1024;
                resp->body = malloc(resp->cap);
                if (!resp->body) {
                    ESP_LOGE(TAG, "Allocation initiale échouée");
                    return ESP_FAIL;
                }
                resp->len = 0;
            }
            while (resp->len + evt->data_len > resp->cap) {
                int new_cap = resp->cap * 2;
                char *p = realloc(resp->body, new_cap);
                if (!p) {
                    ESP_LOGE(TAG, "Realloc à %d échouée", new_cap);
                    free(resp->body);
                    resp->body = NULL;
                    resp->cap = resp->len = 0;
                    return ESP_FAIL;
                }
                resp->body = p;
                resp->cap = new_cap;
            }
            memcpy(resp->body + resp->len, evt->data, evt->data_len);
            resp->len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    default:
        break;
    }
    return ESP_OK;
}

char *http_fetch(const char *url,
                 const char *method,
                 const char *bearer_token,
                 const char *post_fields,
                 int *out_status)
{
    response_t resp = { .body = NULL, .len = 0, .cap = 0 };

    esp_http_client_config_t cfg = {
        .url = url,
        .method = (method && strcmp(method, "POST") == 0) ? HTTP_METHOD_POST : HTTP_METHOD_GET,
        .event_handler = on_data,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,            // ★ un peu plus large
        .buffer_size = 4096,            // ★ buffer réception (headers + statut + un peu de corps)
        .buffer_size_tx = 2048,         // ★ buffer émission (headers requête)
        //.disable_auto_redirect = false // défaut OK
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "Init client HTTP échouée");
        return NULL;
    }

    // En-têtes
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    esp_http_client_set_header(client, "Accept", "application/json"); // ★
    esp_http_client_set_header(client, "Connection", "close");        // ★ évite le keep-alive pénible

    if (bearer_token && bearer_token[0]) {
        char hdr[1024]; // ★ grand, tokens Google sont longs
        int n = snprintf(hdr, sizeof(hdr), "Bearer %s", bearer_token);
        if (n <= 0 || n >= (int)sizeof(hdr)) {
            ESP_LOGE(TAG, "Token trop long pour l'en-tête Authorization");
            esp_http_client_cleanup(client);
            return NULL;
        }
        esp_http_client_set_header(client, "Authorization", hdr);
    }

    if (post_fields) {
        esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
        esp_http_client_set_post_field(client, post_fields, strlen(post_fields));
    }

    // Requête
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform error: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        if (resp.body) free(resp.body);
        return NULL;
    }

    int status = esp_http_client_get_status_code(client);
    if (out_status) *out_status = status;

    // Longueur réelle lue (utile si Transfer-Encoding: chunked)
    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP status: %d, length: %d (content_length=%d)", status, resp.len, content_length);

    // Null-terminate
    char *body = NULL;
    if (resp.body) {
        char *p = realloc(resp.body, resp.len + 1);
        if (p) {
            p[resp.len] = '\0';
            body = p;
        } else {
            ESP_LOGE(TAG, "Allocation pour terminator échouée");
            free(resp.body);
            body = NULL;
        }
    }

    esp_http_client_cleanup(client);
    return body;
}
