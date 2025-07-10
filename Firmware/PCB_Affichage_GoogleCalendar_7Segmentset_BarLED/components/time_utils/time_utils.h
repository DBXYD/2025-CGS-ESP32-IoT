#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

/**
 * Convertit un struct tm UTC en time_t sans dépendre du TZ local.
 */
time_t my_timegm(struct tm *tm);

#endif // TIME_UTILS_H

