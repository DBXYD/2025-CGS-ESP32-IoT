#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <time.h>

/* Exposés à tous les composants */
extern volatile bool   event_active;
extern volatile time_t event_start_ts;   /* début de l’évènement */
extern volatile time_t event_end_ts;     /* fin de l’évènement  */

#endif
