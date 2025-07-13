#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
bool api_get_esp_status(bool *should_turn_on);

void api_send_ping(bool current_state);
void api_send_ping_state(bool rack_on); 
void api_send_ping_bits(uint8_t bits);
void api_send_ping_full(bool rack_on, uint8_t mask);
#endif
