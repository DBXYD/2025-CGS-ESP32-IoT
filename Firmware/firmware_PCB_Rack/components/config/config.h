#ifndef CONFIG_H
#define CONFIG_H

/* --- Wi-Fi --------------------------------------------------- */
static const char WIFI_SSID[] = "TechTinkerers";
static const char WIFI_PASS[] = "NoArduinoZone";

/* --- SNTP ---------------------------------------------------- */
static const char SNTP_SERVER[] = "pool.ntp.org";

/* --- API (IP fixe du PC) ------------------------------------ */
/* PC Django: 192.168.0.253:8000 */
#define API_HOST         "http://192.168.0.253:8000"
#define API_BASE         API_HOST "/api/esp/"

/* Endpoints uniques (pas de variantes MDNS/FB) */
#define STATUS_URL       API_BASE "status/"
#define PING_URL         API_BASE "ping/"

/* --- Divers -------------------------------------------------- */
#define STUDIO_NAME      "Rack_Rainbow"

#endif /* CONFIG_H */
