#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "http_client.h"
#include "cJSON.h"
#include "esp_log.h"
#include "config.h"
#include "time_utils.h"  // pour my_timegm()

static const char *TAG_GCAL = "gcal";

bool calendar_check_today(const char *access_token,
                          char *summary,  size_t sum_len,
                          char *location, size_t loc_len,
                          time_t *start_ts,   /* nouveau */
                          time_t *end_ts)
{
// --- Initialisation des sorties ---
    if (summary && sum_len > 0)   summary[0] = '\0';
    if (location && loc_len > 0)  location[0] = '\0';
    if (end_ts)                   *end_ts = 0;


    // 1) bornes locales jour courant
    time_t now = time(NULL);
    struct tm tm0 = *localtime(&now);
    tm0.tm_hour = tm0.tm_min = tm0.tm_sec = 0;
    time_t start_local = mktime(&tm0);
    tm0.tm_hour = 23; tm0.tm_min = 59; tm0.tm_sec = 59;
    time_t end_local = mktime(&tm0);

    // 2) ISO UTC
    char tmin[32], tmax[32];
    struct tm utc;
    gmtime_r(&start_local, &utc);
    strftime(tmin, sizeof tmin, "%Y-%m-%dT%H:%M:%SZ", &utc);
    gmtime_r(&end_local, &utc);
    strftime(tmax, sizeof tmax, "%Y-%m-%dT%H:%M:%SZ", &utc);

    // 3) URL
    char url[512];
    snprintf(url, sizeof url,
        "https://www.googleapis.com/calendar/v3/calendars/%s/events"
        "?singleEvents=true&orderBy=startTime"
        "&timeMin=%s&timeMax=%s&timeZone=UTC&maxResults=10",
        CALENDAR_ID, tmin, tmax);

    // 4) fetch
    int status;
    char *resp = http_fetch(url, "GET", access_token, NULL, &status);
    if (!resp) return false;
    if (status == 401) { free(resp); return false; }

    // 5) parse + liste
    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) return false;
    cJSON *items = cJSON_GetObjectItem(root, "items");
    if (!cJSON_IsArray(items)) {
        cJSON_Delete(root);
        return false;
    }

    ESP_LOGI(TAG_GCAL, "────── Événements aujourd'hui ──────");
    time_t now_ts = now;
    bool found = false;

    cJSON *ev;
    cJSON_ArrayForEach(ev, items) {
        cJSON *jsum = cJSON_GetObjectItem(ev, "summary");
        const char *tit = (jsum && cJSON_IsString(jsum))
                          ? jsum->valuestring
                          : "(sans titre)";
        cJSON *jloc = cJSON_GetObjectItem(ev, "location");
        const char *loc = (jloc && cJSON_IsString(jloc))
                          ? jloc->valuestring
                          : "(pas de lieu)";

        cJSON *jstart = cJSON_GetObjectItem(
                            cJSON_GetObjectItem(ev, "start"),
                            "dateTime");
        cJSON *jend   = cJSON_GetObjectItem(
                            cJSON_GetObjectItem(ev, "end"),
                            "dateTime");
        if (!cJSON_IsString(jstart) || !cJSON_IsString(jend)) {
            continue;
        }

        struct tm tms = {0}, tme = {0}, loc_tm;
        time_t ts, te;
        // on parse l'ISO UTC puis on convertit en timestamp UTC
        strptime(jstart->valuestring, "%Y-%m-%dT%H:%M:%SZ", &tms);
        ts = my_timegm(&tms);
        strptime(jend->valuestring,   "%Y-%m-%dT%H:%M:%SZ", &tme);
        te = my_timegm(&tme);

        char bs[6], be[6];
        localtime_r(&ts, &loc_tm); strftime(bs, sizeof bs, "%H:%M", &loc_tm);
        localtime_r(&te, &loc_tm); strftime(be, sizeof be, "%H:%M", &loc_tm);

        ESP_LOGI(TAG_GCAL, "• %s @ %s → %s  (Lieu: %s)", tit, bs, be, loc);

        // détection en cours
        if (!found && now_ts >= ts && now_ts < te) {
            found = true;
            strncpy(summary,  tit,    sum_len-1);
            summary[sum_len-1] = '\0';
            strncpy(location, loc,    loc_len-1);
            location[loc_len-1] = '\0';
            *start_ts = ts;
            *end_ts = te;
        }
    }
    ESP_LOGI(TAG_GCAL, "──────────────────────────────────────");

    cJSON_Delete(root);
    return found;
}
