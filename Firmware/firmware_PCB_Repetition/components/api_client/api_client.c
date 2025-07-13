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

bool api_get_esp_order(bool *want_on, route_mode_t *want_route)
{
    int st; char url[160];
    snprintf(url,sizeof(url), STATUS_URL "?name=%s", STUDIO_NAME);
    char *resp = http_fetch(url,"GET",NULL,NULL,&st);
    if(!resp || st!=200){ free(resp); return false; }

    cJSON *root = cJSON_Parse(resp); free(resp);
    if(!root) return false;

    cJSON *state  = cJSON_GetObjectItem(root,"state");
    cJSON *route  = cJSON_GetObjectItem(root,"route");  /* "DIRECT"/"BUS_AB" */

    bool ok = false;
    if(state && cJSON_IsString(state)){
        *want_on = strcmp(state->valuestring,"ON")==0;
        ok = true;
    }
    if(route && cJSON_IsString(route)){
        *want_route = (strcmp(route->valuestring,"BUS_AB")==0) ? BUS_AB : DIRECT;
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
        http_fetch(PING_URL_FB, "POST", NULL, body, &st);
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
        http_fetch(PING_URL_FB, "POST", NULL, body, &st);
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
        http_fetch(PING_URL_FB, "POST", NULL, body, &st);
}

void api_send_ping_full(bool rack_on,
                        uint8_t mask,
                        route_mode_t route)
{
    /* JSON = {"name":"…","state":true/false,"mask":N,"route":"DIRECT|BUS_AB"} */
    char body[128];
    snprintf(body,sizeof(body),
        "{\"name\":\"%s\",\"state\":%s,\"mask\":%u,\"route\":\"%s\"}",
        STUDIO_NAME,
        rack_on ? "true" : "false",
        mask,
        (route==BUS_AB) ? "BUS_AB" : "DIRECT");

    int st;
    http_fetch(PING_URL, "POST", NULL, body, &st);
    if(st!=200)
        http_fetch(PING_URL_FB,"POST",NULL,body,&st);

    ESP_LOGI(TAG,"Ping → %d (rack=%d mask=0x%02X route=%s)",
                  st, rack_on, mask, (route==BUS_AB)?"BUS_AB":"DIRECT");
}
