#ifndef CONFIG_H
#define CONFIG_H

/* ─────────────── Wi-Fi ─────────────── */
static const char WIFI_SSID[] = "TechTinkerers";
static const char WIFI_PASS[] = "NoArduinoZone";

/* ─────────────── SNTP ──────────────── */
static const char SNTP_SERVER[] = "pool.ntp.org";

/* ───────────── API backend ─────────── */
/* hôte principal (mDNS) et secours (IP fixe du serveur) */
#define API_HOST_MDNS      "http://janusbot.local:8000"
#define API_HOST_FALLBACK  "http://192.168.0.253:8000"

/* bases */
#define API_BASE_MDNS      API_HOST_MDNS     "/api/esp/"
#define API_BASE_FALLBACK  API_HOST_FALLBACK "/api/esp/"

/* endpoints */
#define STATUS_URL     API_BASE_MDNS     "status/"
#define PING_URL       API_BASE_MDNS     "ping/"
#define STATUS_URL_FB  API_BASE_FALLBACK "status/"
#define PING_URL_FB    API_BASE_FALLBACK "ping/"

/* ─────────────── Rack ID ───────────── */
/* DOIT correspondre à « name » dans la table *StudioESP* */
#define STUDIO_NAME "Rack_Scene"

#endif /* CONFIG_H */
