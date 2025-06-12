#include "calendar.h"
#include "http_client.h"
#include "time_utils.h"
#include "config.h"
#include "esp_log.h"
#include "cJSON.h"
#include <time.h>

static const char *TAG_GCAL = "gcal";   

esp_err_t calendar_fetch_and_print(const char *access_token) {
    // 1) Calcul des bornes UTC pour la semaine (de minuit locale à 23:59:59 dimanche)
    char tmin_iso[40], tmax_iso[40];
    time_t now = time(NULL);

    // a) Début local à 00:00:00
    struct tm local_tm = *localtime(&now);
    local_tm.tm_hour = 0;
    local_tm.tm_min  = 0;
    local_tm.tm_sec  = 0;
    time_t start_local = mktime(&local_tm);

    // b) Passage à dimanche 23:59:59
    struct tm end_tm = local_tm;
    int wday = end_tm.tm_wday;                // dimanche=0, lundi=1, etc.
    int days_to_sunday = (7 - (wday == 0 ? 7 : wday));
    end_tm.tm_mday += days_to_sunday;
    end_tm.tm_hour = 23;
    end_tm.tm_min  = 59;
    end_tm.tm_sec  = 59;
    time_t end_local = mktime(&end_tm);

    // c) Conversion en UTC pour l’API
    struct tm utc_tm;
    gmtime_r(&start_local, &utc_tm);
    strftime(tmin_iso, sizeof(tmin_iso), "%Y-%m-%dT%H:%M:%SZ", &utc_tm);
    gmtime_r(&end_local, &utc_tm);
    strftime(tmax_iso, sizeof(tmax_iso), "%Y-%m-%dT%H:%M:%SZ", &utc_tm);

    // 2) Construction de l’URL
    char url[512];
    snprintf(url, sizeof(url),
        "https://www.googleapis.com/calendar/v3/calendars/%s/events"
        "?singleEvents=true"
        "&orderBy=startTime"
        "&timeMin=%s"
        "&timeMax=%s"
        "&timeZone=UTC"
        "&maxResults=%d",
        CALENDAR_ID, tmin_iso, tmax_iso, MAX_EVENTS
    );

    // 3) Requête HTTP
    int status;
    char *resp = http_fetch(url, "GET", access_token, NULL, &status);
    if (!resp) return -1;
    if (status == 401) { free(resp); return 401; }

    // 4) Parsing JSON
    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) return -1;
    cJSON *items = cJSON_GetObjectItem(root, "items");
    if (!cJSON_IsArray(items)) {
        cJSON_Delete(root);
        return status;
    }

    // 5) Affichage formaté
    static const char *jours[] = {
        "Dimanche","Lundi","Mardi","Mercredi",
        "Jeudi","Vendredi","Samedi"
    };

    ESP_LOGI(TAG_GCAL, "========= Planning de la semaine =========");
    cJSON *ev;
    cJSON_ArrayForEach(ev, items) {
        // Titre
        cJSON *sum = cJSON_GetObjectItem(ev, "summary");
        const char *titre = (sum && cJSON_IsString(sum)) ? sum->valuestring : "(Sans titre)";

        // Lieu
        cJSON *loc = cJSON_GetObjectItem(ev, "location");
        const char *lieu = (loc && cJSON_IsString(loc)) ? loc->valuestring : "(pas de lieu)";

        // Récupération start/end en ISO
        cJSON *start = cJSON_GetObjectItem(ev, "start");
        cJSON *end   = cJSON_GetObjectItem(ev, "end");
        const char *sstr = "", *estr = "";
        if (start) {
            cJSON *d = cJSON_GetObjectItem(start, "dateTime");
            if (d && cJSON_IsString(d)) sstr = d->valuestring;
            else {
                d = cJSON_GetObjectItem(start, "date");
                if (d && cJSON_IsString(d)) sstr = d->valuestring;
            }
        }
        if (end) {
            cJSON *d = cJSON_GetObjectItem(end, "dateTime");
            if (d && cJSON_IsString(d)) estr = d->valuestring;
            else {
                d = cJSON_GetObjectItem(end, "date");
                if (d && cJSON_IsString(d)) estr = d->valuestring;
            }
        }

        // Conversion ISO → struct tm pour jour & heures (en Europe/Paris)
        struct tm tmp = {0};

        // Heure de début
        // va parser "2025-06-13T16:30:00+02:00" en tm, et remplir tmp.tm_gmtoff
        strptime(sstr, "%Y-%m-%dT%H:%M:%S%z", &tmp);
        time_t ts = my_timegm(&tmp);      // mktime tient compte de tmp.tm_gmtoff
        localtime_r(&ts, &tmp);        // tmp est maintenant en heure locale
        char jstart[16], hdeb[6];
        snprintf(jstart, sizeof(jstart), "%s", jours[tmp.tm_wday]);
        strftime(hdeb, sizeof(hdeb), "%H:%M", &tmp);

        // Heure de fin
        strptime(estr, "%Y-%m-%dT%H:%M:%SZ", &tmp);
        ts = my_timegm(&tmp);                      // <— idem
        localtime_r(&ts, &tmp);
        char hfin[6];
        strftime(hfin, sizeof(hfin), "%H:%M", &tmp);

        ESP_LOGI(TAG_GCAL,
            "• %s\n   ↳ %s de %s à %s\n   ↳ Lieu : %s",
            titre, jstart, hdeb, hfin, lieu
        );
    }
    ESP_LOGI(TAG_GCAL, "========================================");

    cJSON_Delete(root);
    return status;
}
