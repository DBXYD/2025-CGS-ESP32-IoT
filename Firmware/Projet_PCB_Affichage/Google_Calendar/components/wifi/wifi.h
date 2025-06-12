#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"      
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise le Wi-Fi en station et attend la connexion.
 */
void wifi_init(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H
