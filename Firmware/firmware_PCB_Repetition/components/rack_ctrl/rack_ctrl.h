#pragma once
#include <stdbool.h>
#include <stdint.h>


/* ---------- état général ---------- */
void     rack_ctrl_init(void);
void     rack_ctrl_sequence_on (void);
void     rack_ctrl_sequence_off(void);
bool     rack_ctrl_is_on  (void);
bool     rack_ctrl_is_busy(void);
uint8_t  rack_ctrl_get_mask(void);          /* bits AC 0-2 */

/* ---------- routage signaux ---------- */
typedef enum { DIRECT /* IN→OUT */, BUS_AB /* A+B bus */ } route_mode_t;

route_mode_t rack_ctrl_route_get   (void);
void         rack_ctrl_route_bus_ab(void);
void         rack_ctrl_route_direct(void);
