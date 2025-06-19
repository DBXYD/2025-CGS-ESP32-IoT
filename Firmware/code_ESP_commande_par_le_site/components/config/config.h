#ifndef CONFIG_H
#define CONFIG_H

/* --- Wi-Fi --------------------------------------------------- */
static const char WIFI_SSID[] = "Bbox-584AA565";
static const char WIFI_PASS[] = "G6esPcWF22hukeYvSM";

/* --- SNTP ---------------------------------------------------- */
static const char SNTP_SERVER[] = "pool.ntp.org";

/* --- URLs ---------------------------------------------------- */
#define API_HOST_MDNS     "http://janusbot.local:8000/api/esp/"
#define API_HOST_FALLBACK "http://192.168.1.149:8000/api/esp/"

/*  base courant (mDNS) : */
#define STATUS_URL  API_HOST_MDNS "status/"
#define PING_URL    API_HOST_MDNS "ping/"

/*  même endpoints, mais sur l’IP fixe : */
#define STATUS_URL_FB  API_HOST_FALLBACK "status/"
#define PING_URL_FB    API_HOST_FALLBACK "ping/"

/* --- Divers -------------------------------------------------- */
#define STUDIO_NAME  "Rainbow"

#endif /* CONFIG_H */
