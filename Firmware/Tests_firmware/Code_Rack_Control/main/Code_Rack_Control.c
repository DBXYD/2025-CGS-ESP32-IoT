// -----------------------------------------------------------------------------
//  Firmware ESP‑IDF – Multiprise connectée (6 x RT314L(F)05) – ESP32‑C3
// -----------------------------------------------------------------------------
//  • GPIO3   → I²C SCL
//  • GPIO9   → I²C SDA
//  • GPIO10  → Bouton On/Off (entrée + interruption)
//  • GPIO8   → Chaîne WS2812B (6 NeoPixels)
//  • PCF8575 @ 0x20 → Commande SET/RESET des relais (MOSFET low‑side)
//
//  Séquence : un appui sur le bouton allume les 6 sorties l’une après l’autre
//  (délai STEP_DELAY_MS). Un nouvel appui éteint dans l’ordre inverse.
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "led_strip.h"      // composant officiel ESP‑IDF (driver RMT + WS2812)

//--------------------------------------------------
// === 1.  Paramètres matériels & timing        ===
//--------------------------------------------------

#define TAG                 "RACK"

#define I2C_PORT            I2C_NUM_0
#define SDA_PIN             9
#define SCL_PIN             3
#define I2C_FREQ_HZ         400000
#define PCF8575_ADDR        0x20

#define BUTTON_PIN          10
#define NEOPIXEL_PIN        8
#define NUM_PIXELS          6

#define IDLE_LEVEL      0   // force la sortie à 0 V → MOSFET OFF
#define PULSE_LEVEL     1   // relâche (Hi-Z) → MOSFET ON pendant 15 ms

#define PULSE_MS            15    // largeur d’impulsion SET/RESET
#define STEP_DELAY_MS       1000  // délai entre sorties

//--------------------------------------------------
// === 2.  Mapping PCF8575                     ===
//--------------------------------------------------
static const uint8_t relay_set_bits[NUM_PIXELS]   = { 0, 2, 4, 6, 8, 10 }; // P0,2,4,6,8,10
static const uint8_t relay_reset_bits[NUM_PIXELS] = { 1, 3, 5, 7, 9, 11 }; // P1,3,5,7,9,11

//--------------------------------------------------
// === 3.  Variables globales                  ===
//--------------------------------------------------
static led_strip_handle_t led_strip;
static bool all_outputs_on = false;
static QueueHandle_t btn_evt_queue;

typedef struct {
    uint32_t gpio_num;
} button_evt_t;

//--------------------------------------------------
// === 4.  Utilitaires bas‑niveau              ===
//--------------------------------------------------
//----------------------------- PCF8575 low‑level I²C -----------------------------
static esp_err_t pcf8575_write(uint16_t value)
{
    uint8_t data[2] = { value & 0xFF, value >> 8 };
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8575_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 2, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Vous pourrez l’utiliser plus tard pour lire les entrées du PCF.
static esp_err_t __attribute__((unused)) pcf8575_read(uint16_t *value)
{
    uint8_t data[2];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8575_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        *value = data[0] | (data[1] << 8);   // concatène les 2 octets
    }
    return ret;
}

//--------------------------------------------------
//  Pilotage des bobines (SET / RESET)
//--------------------------------------------------
static uint16_t pcf_shadow = 0x0000;  // HIGH(1)=Hi‑Z (MOSFET OFF)

static void pcf_write_bit(uint8_t bit, uint8_t level)
{
    if (level == 0)   pcf_shadow &= ~(1 << bit);
    else              pcf_shadow |=  (1 << bit);
    pcf8575_write(pcf_shadow);
}

static void pulse_bit(uint8_t bit)
{
    pcf_write_bit(bit, PULSE_LEVEL);   // Hi-Z → MOSFET ON
    vTaskDelay(pdMS_TO_TICKS(PULSE_MS));
    pcf_write_bit(bit, IDLE_LEVEL);    // force à 0 → OFF
}

static void relay_set(uint8_t idx)   { pulse_bit(relay_set_bits[idx]);   }
static void relay_reset(uint8_t idx) { pulse_bit(relay_reset_bits[idx]); }

//--------------------------------------------------
// === 5.  NeoPixel helpers                    ===
//--------------------------------------------------
static void pixel_update(uint8_t idx, bool on)
{
    led_strip_set_pixel(led_strip, idx, on ? 0 : 64, on ? 64 : 0, 0); // vert / rouge léger
    led_strip_refresh(led_strip);
}

//--------------------------------------------------
// === 6.  Séquences ON / OFF                  ===
//--------------------------------------------------
static void sequence_on(void)
{
    ESP_LOGI(TAG, "Séquence ON");
    for (uint8_t i = 0; i < NUM_PIXELS; ++i) {
        relay_set(i);
        pixel_update(i, true);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    all_outputs_on = true;
}

static void sequence_off(void)
{
    ESP_LOGI(TAG, "Séquence OFF");
    for (int8_t i = NUM_PIXELS - 1; i >= 0; --i) {
        relay_reset(i);
        pixel_update(i, false);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    all_outputs_on = false;
}

//--------------------------------------------------
// === 7.  Tâche bouton                        ===
//--------------------------------------------------
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    button_evt_t evt = { .gpio_num = (uint32_t) arg };
    xQueueSendFromISR(btn_evt_queue, &evt, NULL);
}

static void button_task(void *arg)
{
    button_evt_t evt;
    for (;;) {
        if (xQueueReceive(btn_evt_queue, &evt, portMAX_DELAY)) {

            /* anti-rebond - 50 ms */
            vTaskDelay(pdMS_TO_TICKS(50));

            int level = gpio_get_level(evt.gpio_num);   // 1 = ouvert, 0 = fermé

            if (level == 1) {           // bouton ouvert  → demander ON
                if (!all_outputs_on) {
                    sequence_on();
                }
            } else {                    // bouton fermé  → demander OFF
                if (all_outputs_on) {
                    sequence_off();
                }
            }
        }
    }
}


//--------------------------------------------------
// === 8.  Setup hardware                      ===
//--------------------------------------------------
static void i2c_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    i2c_param_config(I2C_PORT, &cfg);
    i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0);

    // Initialiser PCF à Hi‑Z (1)
    pcf8575_write(pcf_shadow);
}

static void neopixel_init(void)
{
    // Configuration compatible IDF 5.0 (pas de LED_PIXEL_FORMAT)
    led_strip_config_t strip_cfg = {
        .strip_gpio_num   = NEOPIXEL_PIN,
        .max_leds         = NUM_PIXELS,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_cfg = {
    .clk_src           = RMT_CLK_SRC_DEFAULT,
    .resolution_hz     = 10 * 1000 * 1000UL, // 10 MHz → 800 kHz WS2812
    .mem_block_symbols = 256,                // >= 144 symboles pour 6 LED
    .flags.with_dma    = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &led_strip));
    led_strip_clear(led_strip);
}

static void button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,   // front montant et descendant (appui)
    };
    gpio_config(&io);
    btn_evt_queue = xQueueCreate(4, sizeof(button_evt_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void *) BUTTON_PIN);
    xTaskCreatePinnedToCore(button_task, "btn", 2048, NULL, 10, NULL, 0);
}

//--------------------------------------------------
// === 9.  app_main                            ===
//--------------------------------------------------
void app_main(void)
{
    ESP_LOGI(TAG, "Démarrage multiprise...");

    i2c_init();
    neopixel_init();
    button_init();

    ESP_LOGI(TAG, "Init OK – Appuyez sur le bouton.");

    // Boucle principale : rien à faire, tout est géré dans les tâches / ISR
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
