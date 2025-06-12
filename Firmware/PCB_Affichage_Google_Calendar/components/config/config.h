#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi
static const char WIFI_SSID[]        = "TechTinkerers";
static const char WIFI_PASS[]        = "NoArduinoZone";

// OAuth2 Google
static const char GOOGLE_CLIENT_ID[]     = "1758091333-emsbi2bk4gp86ouv2ojbp2csamfgmlao.apps.googleusercontent.com";
static const char GOOGLE_CLIENT_SECRET[] = "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";
static const char GOOGLE_SCOPE[]         = "https://www.googleapis.com/auth/calendar.readonly";

// Google Calendar
static const char CALENDAR_ID[]    = "primary";
#define MAX_EVENTS       5
#define MAX_WIFI_RETRY   5

// SNTP
static const char SNTP_SERVER[]    = "pool.ntp.org";


// OAuth2 Device Code Flow (Google)
#define OAUTH_CLIENT_ID   "TON_CLIENT_ID.apps.googleusercontent.com"
#define OAUTH_SCOPE       "https://www.googleapis.com/auth/calendar.readonly"

// URL pour obtenir le device_code
#define OAUTH_DEVICE_CODE_URL  "https://oauth2.googleapis.com/device/code"
// POST body pour la requête device_code
#define OAUTH_DEVICE_CODE_POST "client_id=" OAUTH_CLIENT_ID "&scope=" OAUTH_SCOPE

// URL pour récupérer (poll) le token
#define OAUTH_TOKEN_URL        "https://oauth2.googleapis.com/token"
// POST body template pour poll (à sprintf avec device_code)
#define OAUTH_POLL_BODY_FMT    "client_id=" OAUTH_CLIENT_ID \
                               "&device_code=%s&grant_type=urn:ietf:params:oauth:grant-type:device_code"
// POST body template pour refresh (à sprintf avec refresh_token)
#define OAUTH_REFRESH_BODY_FMT "client_id=" OAUTH_CLIENT_ID \
                               "&refresh_token=%s&grant_type=refresh_token"

#endif // CONFIG_H
