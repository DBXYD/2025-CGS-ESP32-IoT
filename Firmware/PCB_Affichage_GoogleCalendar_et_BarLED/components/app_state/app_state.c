#include "app_state.h"

/* Allocation unique */
volatile bool   event_active    = false;
volatile time_t event_start_ts  = 0;
volatile time_t event_end_ts    = 0;
