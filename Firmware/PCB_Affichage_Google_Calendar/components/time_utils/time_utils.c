#include "time_utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Sauvegarde et restauration de TZ pour mktime en UTC.
 */
time_t my_timegm(struct tm *tm) {
    char *old_tz = getenv("TZ");
    char *tz_copy = old_tz ? strdup(old_tz) : NULL;
    setenv("TZ", "UTC0", 1);
    tzset();

    time_t t = mktime(tm);

    if (tz_copy) {
        setenv("TZ", tz_copy, 1);
        free(tz_copy);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return t;
}
