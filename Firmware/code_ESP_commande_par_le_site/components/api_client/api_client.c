#include "api_client.h"
#include "esp_log.h"
#include "config.h"
#include "cJSON.h"
#include "http_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "api";

bool api_get_esp_status(bool *should_turn_on)
{
    int status;
    char url[160];

    /* 1) tentative via mDNS ---------------------------------- */
    snprintf(url, sizeof(url), STATUS_URL "?name=%s", STUDIO_NAME);
    char *resp = http_fetch(url, "GET", NULL, NULL, &status);

    /* 2) si échec DNS ou socket, on passe à l’IP de secours --- */
    if ((!resp || status != 200) && (status == 0 || status == 202)) {
    ESP_LOGW(TAG, "mDNS KO, bascule sur IP fixe");
    snprintf(url, sizeof(url), STATUS_URL_FB "?name=%s", STUDIO_NAME);
    resp = http_fetch(url, "GET", NULL, NULL, &status);
    }

    if (!resp || status != 200) {
        ESP_LOGW(TAG, "Échec HTTP (%d)", status);
        if (resp) free(resp);
        return false;
    }

    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) return false;

    cJSON *state = cJSON_GetObjectItem(root, "state");
    bool ok = false;

    if (state) {
        if (cJSON_IsBool(state)) {
            *should_turn_on = cJSON_IsTrue(state);
            ok = true;
        } else if (cJSON_IsString(state)) {
            if (strcmp(state->valuestring, "ON") == 0) {
                *should_turn_on = true;
                ok = true;
            } else if (strcmp(state->valuestring, "OFF") == 0) {
                *should_turn_on = false;
                ok = true;
            }
        }
    }


    cJSON_Delete(root);
    return ok;
}

void api_send_ping(void)
{
    char body[64];
    snprintf(body, sizeof(body), "{\"name\":\"%s\"}", STUDIO_NAME);

    int http_st;

    /* tentative mDNS */
    http_fetch(PING_URL, "POST", NULL, body, &http_st);

    /* repli IP si la 1ʳᵉ tentative échoue */
    if (http_st != 200) {
        http_fetch(PING_URL_FB, "POST", NULL, body, &http_st);
    }

    ESP_LOGD(TAG, "Ping status=%d", http_st);
}


