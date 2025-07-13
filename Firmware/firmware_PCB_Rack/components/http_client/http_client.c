#include "http_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "http";
#define MAX_HTTP_RESPONSE_SIZE 4096
#define MAX_HTTP_RESPONSE_CAP 16384  // limite RAM à 16 ko (ou moins

typedef struct {
    char *body;
    int len;
    int cap;
} response_t;

/**
 * @brief Callback HTTP pour collecter le corps de la réponse.
 */
static esp_err_t on_data(esp_http_client_event_t *evt)
{
    response_t *resp = evt->user_data;

    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        if (evt->data_len > MAX_HTTP_RESPONSE_CAP) {
            ESP_LOGE(TAG, "Chunk trop gros : %d octets", evt->data_len);
            return ESP_FAIL;
        }

        if (resp->cap == 0) {
            resp->cap = MAX_HTTP_RESPONSE_SIZE;
            resp->body = malloc(resp->cap);
            if (!resp->body) {
                ESP_LOGE(TAG, "Allocation initiale échouée");
                return ESP_FAIL;
            }
            resp->len = 0;
        }

        while (resp->len + evt->data_len > resp->cap) {
            int new_cap = resp->cap * 2;
            if (new_cap > MAX_HTTP_RESPONSE_CAP) {
                ESP_LOGE(TAG, "Réponse trop grande (cap %d), abandon", new_cap);
                // NE PAS free ici
                return ESP_FAIL;
            }

            char *new_body = realloc(resp->body, new_cap);
            if (!new_body) {
                ESP_LOGE(TAG, "Échec de réallocation mémoire à %d octets", new_cap);
                // NE PAS free ici
                return ESP_FAIL;
            }

            resp->cap = new_cap;
            resp->body = new_body;
        }

        memcpy(resp->body + resp->len, evt->data, evt->data_len);
        resp->len += evt->data_len;

        if (resp->len > MAX_HTTP_RESPONSE_CAP) {
            ESP_LOGE(TAG, "Dépassement mémoire après accumulation (%d octets)", resp->len);
            return ESP_FAIL;
        }
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
        .method = (strcmp(method, "POST") == 0) ? HTTP_METHOD_POST : HTTP_METHOD_GET,
        .event_handler = on_data,
        .user_data = &resp,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "Initialisation client HTTP échouée");
        return NULL;
    }

    // En-têtes
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    if (bearer_token) {
        char hdr[256];
        snprintf(hdr, sizeof(hdr), "Bearer %s", bearer_token);
        esp_http_client_set_header(client, "Authorization", hdr);
    }
    if (post_fields) {
        esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
        esp_http_client_set_post_field(client, post_fields, strlen(post_fields));
    }

    // Exécution de la requête
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);  // 👈 seule déclaration
    if (out_status) {
        *out_status = status;
    }

    if (status != 200) {
        ESP_LOGW(TAG, "Code HTTP inattendu : %d", status);
        if (resp.body) free(resp.body);
        esp_http_client_cleanup(client);
        return NULL;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform error: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        if (resp.body) free(resp.body);
        return NULL;
    }

    ESP_LOGI(TAG, "HTTP status: %d, length: %d", status, resp.len);



    // Null-terminer
    char *body = NULL;
    if (resp.body) {
        body = realloc(resp.body, resp.len + 1);
        if (body) {
            body[resp.len] = '\0';
        } else {
            ESP_LOGE(TAG, "Allocation pour terminator échouée");
            free(resp.body);
            body = NULL;
        }
    }

    esp_http_client_cleanup(client);
    if (body) {
    ESP_LOGI(TAG, "Corps JSON : %s", body);
    }
    return body;
}





