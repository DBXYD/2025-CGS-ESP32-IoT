#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <stdbool.h>

bool api_get_esp_status(bool *should_turn_on);

void api_send_ping(void);

#endif
