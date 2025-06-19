#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi
static const char WIFI_SSID[] = "TechTinkerers";
static const char WIFI_PASS[] = "NoArduinoZone";

// OAuth2 Google
static const char GOOGLE_CLIENT_ID[]     = "1758091333-emsbi2bk4gp86ouv2ojbp2csamfgmlao.apps.googleusercontent.com";
static const char GOOGLE_CLIENT_SECRET[] = "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";
static const char GOOGLE_SCOPE[]         = "https://www.googleapis.com/auth/calendar.readonly";


// SNTP
static const char SNTP_SERVER[] = "pool.ntp.org";

// Pour le site
#define SERVER_COMMAND_URL "http://192.168.0.253:8000/api/esp/status/"



#endif // CONFIG_H
