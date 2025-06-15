#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi
static const char WIFI_SSID[] = "TechTinkerers";
static const char WIFI_PASS[] = "NoArduinoZone";

// OAuth2 Google
static const char GOOGLE_CLIENT_ID[]     = "1758091333-emsbi2bk4gp86ouv2ojbp2csamfgmlao.apps.googleusercontent.com";
static const char GOOGLE_CLIENT_SECRET[] = "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";
static const char GOOGLE_SCOPE[]         = "https://www.googleapis.com/auth/calendar.readonly";

// Google Calendar
static const char CALENDAR_ID[] = "primary";

// SNTP
static const char SNTP_SERVER[] = "pool.ntp.org";

// Corps des requêtes OAuth2 (formats pour snprintf)
#define OAUTH_DEVICE_CODE_POST_FMT   "client_id=%s&scope=%s"
#define OAUTH_POLL_BODY_FMT  \
  "client_id=%s&client_secret=%s&device_code=%s&grant_type=urn:ietf:params:oauth:grant-type:device_code"

#define OAUTH_REFRESH_BODY_FMT       "client_id=%s&refresh_token=%s&grant_type=refresh_token"

// URLs
#define OAUTH_DEVICE_CODE_URL  "https://oauth2.googleapis.com/device/code"
#define OAUTH_TOKEN_URL        "https://oauth2.googleapis.com/token"

#endif // CONFIG_H
