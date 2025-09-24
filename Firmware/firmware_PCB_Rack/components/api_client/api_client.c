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
    int status = 0;
    char url[256];
    char *resp = NULL;

    /* 1) tentative directe */
    snprintf(url, sizeof(url), "%s?name=%s", STATUS_URL, STUDIO_NAME);
    resp = http_fetch(url, "GET", NULL, NULL, &status);

    /* 2) fallback si échec socket/DNS (status==0) ou code != 200 */
    if ((!resp || status != 200) && (status == 0)) {
        ESP_LOGW(TAG, "mDNS/DNS KO, bascule sur IP fixe");
        // Ici STATUS_URL est déjà en IP fixe d'après ton config.h,
        // mais on redemande proprement au cas où.
        snprintf(url, sizeof(url), "%s?name=%s", STATUS_URL, STUDIO_NAME);
        if (resp) { free(resp); resp = NULL; }
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

    bool ok = false;
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (state) {
        if (cJSON_IsBool(state)) {
            *should_turn_on = cJSON_IsTrue(state);
            ok = true;
        } else if (cJSON_IsString(state)) {
            if (!strcmp(state->valuestring, "ON"))  { *should_turn_on = true;  ok = true; }
            if (!strcmp(state->valuestring, "OFF")) { *should_turn_on = false; ok = true; }
        }
    }
    cJSON_Delete(root);
    return ok;
}

void api_send_ping(bool current_state)
{
    char body[96];
    snprintf(body, sizeof(body),
             "{\"name\":\"%s\",\"state\":%s}",
             STUDIO_NAME,
             current_state ? "true" : "false");

    int st;
    http_fetch(PING_URL,    "POST", NULL, body, &st);
    if (st != 200)
        http_fetch(PING_URL, "POST", NULL, body, &st);
}

void api_send_ping_bits(uint8_t bits)
{
    char body[128];
    snprintf(body, sizeof(body),
             "{\"name\":\"%s\",\"mask\":%u}",  // 0-63
             STUDIO_NAME, bits);

    int st;
    http_fetch(PING_URL,    "POST", NULL, body, &st);
    if (st != 200)
        http_fetch(PING_URL, "POST", NULL, body, &st);
}

void api_send_ping_state(bool rack_on)
{
    char body[128];
    snprintf(body, sizeof(body),
             "{\"name\":\"%s\",\"state\":%s}",
             STUDIO_NAME,
             rack_on ? "true":"false");

    int st;
    http_fetch(PING_URL,    "POST", NULL, body, &st);
    if (st != 200)
        http_fetch(PING_URL, "POST", NULL, body, &st);
}

void api_send_ping_full(bool rack_on, uint8_t mask)
{
    char body[160];
    /*  ➜ un seul JSON avec les deux champs               */
    snprintf(body,sizeof(body),
             "{\"name\":\"%s\",\"state\":%s,\"mask\":%u}",
             STUDIO_NAME,
             rack_on ? "true":"false",
             mask);

    int st;
    http_fetch(PING_URL, "POST", NULL, body, &st);
    if(st!=200)
        http_fetch(PING_URL, "POST", NULL, body, &st);
}

