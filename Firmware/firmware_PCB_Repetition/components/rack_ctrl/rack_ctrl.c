/******************************************************************************
 *  rack_ctrl.c – « Rack v2 » : 3 relais 230 V + routage audio A/B
 *  ───────────────────────────────────────────────────────────────────────────
 *  ESP32-C3 SuperMini – voir README pour le brochage complet
 ******************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "led_strip.h"
#include "rack_ctrl.h"
#include "api_client.h"

/* ──────────── Constantes HW ──────────── */
#define TAG                 "RACK"

#define I2C_PORT            I2C_NUM_0
#define SDA_PIN             8
#define SCL_PIN             9
#define I2C_FREQ_HZ         400000
#define PCF8575_ADDR        0x20

#define BTN_PWR_PIN         10      /* POWER  (actif bas)  */
#define BTN_ROUTE_PIN        1      /* ROUTE  (front ↑)    */

#define NEOPIXEL_PIN         5
#define NUM_LED              5

#define PIN_FWD              7      /* ZXBM5210 – BUS_AB   */
#define PIN_REV              6      /* ZXBM5210 – DIRECT   */
#define ZX_PULSE_MS          20

#define PULSE_MS             15     /* impulsion RT314F05  */
#define STEP_DELAY_MS        1000
#define BTN_STACK            4096

/* ─────────── Relais 230 V (PCF8575) ───────────
 * R1 : P14 SET / P15 RESET
 * R2 : P16 SET / P17 RESET
 * R3 : P2  SET / P3  RESET
 */
#define NUM_PWR 3
static const uint8_t SET_P[NUM_PWR] = {15, 17, 3};
static const uint8_t RST_P[NUM_PWR] = {14, 16, 2};

/* ─────────── Variables état ─────────── */
static led_strip_handle_t  strip;
static QueueHandle_t       btn_q;

static volatile bool  busy    = false;    /* séquence en cours      */
static uint8_t        maskPwr = 0x00;     /* bits AC actifs         */
static route_mode_t   modeSig = DIRECT;   /* routage courant        */

/* ─────────── PCF8575 – bas niveau ─────────── */
static uint16_t pcf_shadow = 0xFFFF;      /* 1 = Hi-Z (sortie Off)  */

static esp_err_t pcf_write(uint16_t val)
{
    uint8_t d[2] = { val & 0xFF, val >> 8 };
    i2c_cmd_handle_t c = i2c_cmd_link_create();
    i2c_master_start(c);
    i2c_master_write_byte(c, (PCF8575_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(c, d, 2, true);
    i2c_master_stop(c);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, c, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(c);
    return ret;
}

static inline void pcf_set_bit(uint8_t bit, bool hi)
{
    if (hi) pcf_shadow |=  (1 << bit);
    else    pcf_shadow &= ~(1 << bit);
    pcf_write(pcf_shadow);
}

static inline void pcf_pulse(uint8_t bit)
{
    pcf_set_bit(bit, true);
    vTaskDelay(pdMS_TO_TICKS(PULSE_MS));
    pcf_set_bit(bit, false);
}

/* ─────────── WS2812 helper ─────────── */
static void pixel(uint8_t idx, bool on)
{
    if (idx < NUM_PWR) {            /* LED 0-2 : vert = ON, rouge = OFF */
        led_strip_set_pixel(strip, idx, on ? 0 : 64, on ? 64 : 0, 0);
    } else {                        /* LED 3-4 : bleu / jaune */
        if (on) led_strip_set_pixel(strip, idx, 0, 0, 64);            /* BUS_AB */
        else    led_strip_set_pixel(strip, idx, 64, 64, 0);           /* DIRECT */
    }
    led_strip_refresh(strip);
}

/* ─────────── ZXBM5210 helper ─────────── */
static inline void zxbm_pulse(bool fwd)
{
    gpio_set_level(fwd ? PIN_FWD : PIN_REV, 1);
    vTaskDelay(pdMS_TO_TICKS(ZX_PULSE_MS));
    gpio_set_level(fwd ? PIN_FWD : PIN_REV, 0);
}

/* ─────────── Séquences puissance ─────────── */
static void seq_ac_on(void)
{
    busy = true;
    for (uint8_t i = 0; i < NUM_PWR; ++i) {
        pcf_pulse(SET_P[i]);
        maskPwr |= 1 << i;
        pixel(i, true);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    busy = false;
    api_send_ping_full(true, maskPwr, modeSig);
}

static void seq_ac_off(void)
{
    busy = true;
    for (int8_t i = NUM_PWR - 1; i >= 0; --i) {
        pcf_pulse(RST_P[i]);
        maskPwr &= ~(1 << i);
        pixel(i, false);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    busy = false;
    api_send_ping_full(false, maskPwr, modeSig);
}

/* ─────────── Routage signaux ─────────── */
static void route_bus_ab(void)
{
    if (modeSig == BUS_AB) return;
    modeSig = BUS_AB;
    zxbm_pulse(true);
    pixel(3, true);  pixel(4, true);
    api_send_ping_full(rack_ctrl_is_on(), maskPwr, modeSig);
}

static void route_direct(void)
{
    if (modeSig == DIRECT) return;
    modeSig = DIRECT;
    zxbm_pulse(false);
    pixel(3, false); pixel(4, false);
    api_send_ping_full(rack_ctrl_is_on(), maskPwr, modeSig);
}

/* ─────────── Boutons GPIO ─────────── */
typedef struct { uint8_t gpio; } btn_evt_t;

static void IRAM_ATTR gpio_isr(void *arg)
{
    btn_evt_t ev = { .gpio = (uint8_t)(uint32_t)arg };
    xQueueSendFromISR(btn_q, &ev, NULL);
}

static void btn_task(void *arg)
{
    btn_evt_t ev;
    for (;;) {
        if (xQueueReceive(btn_q, &ev, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(30));   /* anti-rebond */

            if (ev.gpio == BTN_PWR_PIN && gpio_get_level(BTN_PWR_PIN) == 0 && !busy)
                (maskPwr == 0x07) ? seq_ac_off() : seq_ac_on();

            if (ev.gpio == BTN_ROUTE_PIN && gpio_get_level(BTN_ROUTE_PIN) == 1 && !busy)
                (modeSig == DIRECT) ? route_bus_ab() : route_direct();
        }
    }
}

/* ─────────── Initialisation HW ─────────── */
static void i2c_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN, .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    i2c_param_config(I2C_PORT, &cfg);
    i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0);

    /* toutes les sorties en Hi-Z au boot */
    pcf_write(pcf_shadow);
}

static void neopixel_init(void)
{
    led_strip_config_t sc = { .strip_gpio_num = NEOPIXEL_PIN, .max_leds = NUM_LED };
    led_strip_rmt_config_t rc = { .clk_src = RMT_CLK_SRC_DEFAULT,
                                  .resolution_hz = 10 * 1000 * 1000UL,
                                  .mem_block_symbols = 64 };
    led_strip_new_rmt_device(&sc, &rc, &strip);
    led_strip_clear(strip);
}

static void zxbm_init(void)
{
    gpio_config_t io = { .pin_bit_mask = (1ULL << PIN_FWD) | (1ULL << PIN_REV),
                         .mode = GPIO_MODE_OUTPUT };
    gpio_config(&io);
    gpio_set_level(PIN_FWD, 0);
    gpio_set_level(PIN_REV, 0);
}

static void buttons_init(void)
{
    gpio_config_t cfg_btn = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    cfg_btn.pin_bit_mask = 1ULL << BTN_PWR_PIN;  gpio_config(&cfg_btn);
    cfg_btn.pin_bit_mask = 1ULL << BTN_ROUTE_PIN; gpio_config(&cfg_btn);

    btn_q = xQueueCreate(8, sizeof(btn_evt_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    gpio_isr_handler_add(BTN_PWR_PIN,  gpio_isr, (void *)BTN_PWR_PIN);
    gpio_isr_handler_add(BTN_ROUTE_PIN, gpio_isr, (void *)BTN_ROUTE_PIN);

    xTaskCreatePinnedToCore(btn_task, "btn", BTN_STACK, NULL, 10, NULL, 0);
}

/* ─────────── API publique ─────────── */
void rack_ctrl_init(void)
{
    ESP_LOGI(TAG, "Init rack v2…");
    i2c_init();
    neopixel_init();
    zxbm_init();
    buttons_init();
    ESP_LOGI(TAG, "Prêt !");
}

bool      rack_ctrl_is_busy(void)          { return busy; }
bool      rack_ctrl_is_on(void)            { return maskPwr == 0x07; }
uint8_t   rack_ctrl_get_mask(void)         { return maskPwr; }
route_mode_t rack_ctrl_route_get(void)     { return modeSig; }

void rack_ctrl_sequence_on (void) { if (!busy && maskPwr != 0x07) seq_ac_on();  }
void rack_ctrl_sequence_off(void) { if (!busy && maskPwr)          seq_ac_off(); }

void rack_ctrl_route_bus_ab(void) { if (!busy && modeSig == DIRECT) route_bus_ab(); }
void rack_ctrl_route_direct(void) { if (!busy && modeSig == BUS_AB) route_direct(); }
