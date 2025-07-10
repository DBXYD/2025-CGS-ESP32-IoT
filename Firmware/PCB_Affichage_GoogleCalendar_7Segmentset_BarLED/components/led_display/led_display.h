#ifndef LED_DISPLAY_H
#define LED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <led_strip.h>

/* déclaration seulement */
extern const uint8_t DIGIT_SEG[10];

/* index LED du premier segment du chiffre `digit_idx` (0-3) */
static inline int digit_base(int digit_idx)
{
    return (digit_idx < 2)     ? digit_idx * 21      /* 0 & 1   */
                               : 44 + (digit_idx-2)*21; /* 2 & 3 */
}

/* API */
void draw_digit(led_strip_handle_t strip, int digit_idx, int value,
                uint8_t r, uint8_t g, uint8_t b);

void draw_colon(led_strip_handle_t strip, bool on,
                uint8_t r, uint8_t g, uint8_t b);

#endif /* LED_DISPLAY_H */
