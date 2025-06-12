#include "oauth.h"
#include "http_client.h"
#include "config.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG_OAUTH = "oauth";

bool oauth_get_device_code(char *device_code, char *user_code, char *verification_url, int *interval)
{
    int status;
    // Build POST body for device code request
    char post_fields[256];
    snprintf(post_fields, sizeof(post_fields),
             "client_id=%s&scope=%s",
             GOOGLE_CLIENT_ID, GOOGLE_SCOPE);

    // Fetch device code
    char *resp = http_fetch(
        "https://oauth2.googleapis.com/device/code",
        "POST",
        NULL,
        post_fields,
        &status
    );
    ESP_LOGI(TAG_OAUTH, "Device-code HTTP status=%d", status);
    if (!resp || status != 200) {
        ESP_LOGE(TAG_OAUTH, "Device-code fetch failed (status=%d)", status);
        free(resp);
        return false;
    }

    // Parse JSON response
    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) {
        ESP_LOGE(TAG_OAUTH, "JSON parse error");
        return false;
    }

    cJSON *dc = cJSON_GetObjectItem(root, "device_code");
    cJSON *uc = cJSON_GetObjectItem(root, "user_code");
    cJSON *vu = cJSON_GetObjectItem(root, "verification_url");
    cJSON *it = cJSON_GetObjectItem(root, "interval");
    if (!dc || !uc || !vu || !it) {
        ESP_LOGE(TAG_OAUTH, "Missing JSON fields");
        cJSON_Delete(root);
        return false;
    }

    // Copy values with termination
    strncpy(device_code, dc->valuestring, 255); device_code[255] = '\0';
    strncpy(user_code,   uc->valuestring,   31);  user_code[31]   = '\0';
    strncpy(verification_url, vu->valuestring, 127); verification_url[127] = '\0';
    *interval = it->valueint;

    cJSON_Delete(root);
    ESP_LOGI(TAG_OAUTH, "Got device_code flow");
    return true;
}

bool oauth_poll_token(const char *device_code, char *access_token, int *expires_in, char *refresh_tok)
{
    int status;
    // Build POST body for token polling
    char post_fields[512];
    snprintf(post_fields, sizeof(post_fields),
             "client_id=%s&client_secret=%s&device_code=%s&grant_type=urn:ietf:params:oauth:grant-type:device_code",
             GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET, device_code);

    ESP_LOGI(TAG_OAUTH, "Polling token, device_code=%s", device_code);
    char *resp = http_fetch(
        "https://oauth2.googleapis.com/token",
        "POST",
        NULL,
        post_fields,
        &status
    );
    if (!resp) {
        ESP_LOGE(TAG_OAUTH, "Token poll fetch failed");
        return false;
    }
    ESP_LOGI(TAG_OAUTH, "Poll HTTP status=%d, body=%s", status, resp);

    cJSON *j = cJSON_Parse(resp);
    free(resp);
    if (!j) {
        ESP_LOGE(TAG_OAUTH, "JSON parse error");
        return false;
    }

    cJSON *err = cJSON_GetObjectItem(j, "error");
    if (err && cJSON_IsString(err)) {
        cJSON_Delete(j);
        return false;
    }

    cJSON *at = cJSON_GetObjectItem(j, "access_token");
    cJSON *ex = cJSON_GetObjectItem(j, "expires_in");
    cJSON *rt = cJSON_GetObjectItem(j, "refresh_token");
    bool ok = at && ex;
    if (ok) {
        strncpy(access_token, at->valuestring, 2047);
        access_token[2047] = '\0';
        *expires_in = ex->valueint;
        if (rt) {
            strncpy(refresh_tok, rt->valuestring, 255);
            refresh_tok[255] = '\0';
        }
    }
    cJSON_Delete(j);
    return ok;
}

bool oauth_refresh(const char *refresh_tok, char *access_token, int *expires_in)
{
    int status;
    char post_fields[512];
    snprintf(post_fields, sizeof(post_fields),
             "client_id=%s&client_secret=%s&refresh_token=%s&grant_type=refresh_token",
             GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET, refresh_tok);

    char *resp = http_fetch(
        "https://oauth2.googleapis.com/token",
        "POST",
        NULL,
        post_fields,
        &status
    );
    if (!resp || status != 200) {
        ESP_LOGE(TAG_OAUTH, "Refresh failed (status=%d)", status);
        free(resp);
        return false;
    }

    cJSON *j = cJSON_Parse(resp);
    free(resp);
    if (!j) {
        ESP_LOGE(TAG_OAUTH, "JSON parse error");
        return false;
    }

    cJSON *at = cJSON_GetObjectItem(j, "access_token");
    cJSON *ex = cJSON_GetObjectItem(j, "expires_in");
    bool ok = at && ex;
    if (ok) {
        strncpy(access_token, at->valuestring, 2047);
        access_token[2047] = '\0';
        *expires_in = ex->valueint;
    }
    cJSON_Delete(j);
    return ok;
}

bool save_refresh_token(const char *tok)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open("google", NVS_READWRITE, &h);
    if (e != ESP_OK) {
        ESP_LOGE(TAG_OAUTH, "NVS open failed: %s", esp_err_to_name(e));
        return false;
    }
    e = nvs_set_str(h, "refresh_token", tok ? tok : "");
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    if (e != ESP_OK) {
        ESP_LOGE(TAG_OAUTH, "NVS commit failed: %s", esp_err_to_name(e));
        return false;
    }
    return true;
}

bool load_refresh_token(char *buf, size_t len)
{
    nvs_handle_t h;
    if (nvs_open("google", NVS_READONLY, &h) != ESP_OK) return false;
    esp_err_t e = nvs_get_str(h, "refresh_token", buf, &len);
    nvs_close(h);
    return (e == ESP_OK && buf[0] != '\0');
}
