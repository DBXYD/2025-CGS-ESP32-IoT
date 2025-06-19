#include "api_client.h"
#include "http_client.h"
#include "esp_log.h"
#include "config.h"
#include "cJSON.h"
#include "api_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "api";

bool api_get_esp_status(bool *should_turn_on) {
    int status;
    char *resp = http_fetch(SERVER_COMMAND_URL, "GET", NULL, NULL, &status);

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

    if (state && cJSON_IsString(state)) {
        if (strcmp(state->valuestring, "ON") == 0) {
            *should_turn_on = true;
            ok = true;
        } else if (strcmp(state->valuestring, "OFF") == 0) {
            *should_turn_on = false;
            ok = true;
        }
    }

    cJSON_Delete(root);
    return ok;
}
