#include "led_display.h"

/* définition UNE seule fois ici */
const uint8_t DIGIT_SEG[10] = {    /*  d c g f a b e  (bit0→bit6) */
    0b1111011, 0b0100010, 0b1110101, 0b0110111, 0b0101110,
    0b0011111, 0b1011111, 0b0110010, 0b1111111, 0b0111111
};

void draw_digit(led_strip_handle_t strip, int digit_idx, int value,
                uint8_t r, uint8_t g, uint8_t b)
{
    if (digit_idx < 0 || digit_idx > 3 || value < 0 || value > 9) return;

    uint8_t pattern = DIGIT_SEG[value];
    int base = digit_base(digit_idx);

    for (int s = 0; s < 7; ++s) {
        bool on = (pattern >> s) & 1;
        for (int i = 0; i < 3; ++i) {
            int idx = base + s*3 + i;
            led_strip_set_pixel(strip, idx,
                                on ? r : 0,
                                on ? g : 0,
                                on ? b : 0);
        }
    }
}

void draw_colon(led_strip_handle_t strip, bool on,
                uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 2; ++i) {
        int idx = 42 + i;               /* LEDs 42 & 43 */
        led_strip_set_pixel(strip, idx,
                            on ? r : 0,
                            on ? g : 0,
                            on ? b : 0);
    }
}
