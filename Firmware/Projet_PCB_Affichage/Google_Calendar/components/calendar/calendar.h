#ifndef CALENDAR_H
#define CALENDAR_H
#include <esp_err.h>

esp_err_t calendar_fetch_and_print(const char *access_token);

#endif // CALENDAR_H
