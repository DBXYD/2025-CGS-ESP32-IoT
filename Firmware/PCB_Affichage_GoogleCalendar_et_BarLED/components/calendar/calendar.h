#ifndef CALENDAR_H
#define CALENDAR_H
#include <esp_err.h>
#include <time.h>

bool calendar_check_today(const char *access_token,
                          char *summary,  size_t sum_len,
                          char *location, size_t loc_len,
                          time_t *start_ts,
                          time_t *end_ts);




#endif // CALENDAR_H
