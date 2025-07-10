#pragma once
#include <stdbool.h>
#include <stdint.h>      /* pour uint8_t si besoin */

void  rack_ctrl_init(void);

void  rack_ctrl_sequence_on(void);
void  rack_ctrl_sequence_off(void);

bool  rack_ctrl_is_on(void);
bool  rack_ctrl_is_busy(void);      /*  <<< NOUVEAU  */
