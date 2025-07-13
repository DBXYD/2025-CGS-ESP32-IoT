// -----------------------------------------------------------------------------
// Multiprise 6 × RT314L  –  ESP32-C3
// -----------------------------------------------------------------------------
// • GPIO3  I²C SCL   • GPIO9  I²C SDA   • GPIO10  Bouton   • GPIO8  WS2812B
// • PCF8575 @0x20 pour SET/RESET des relais
// Séquence ON : 0→5 / OFF : 5→0   (STEP_DELAY_MS)
// Un unique ping JSON est envoyé à la fin de la séquence.
// -----------------------------------------------------------------------------
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

/* -------------------------------------------------------------------------- */
#define TAG                 "RACK"
#define I2C_PORT            I2C_NUM_0
#define SDA_PIN             9
#define SCL_PIN             3
#define I2C_FREQ_HZ         400000
#define PCF8575_ADDR        0x20

#define BUTTON_PIN          10
#define NEOPIXEL_PIN        8
#define NUM_PIXELS          6

#define PULSE_MS            15
#define STEP_DELAY_MS       1000
#define BTN_STACK           4096          /* pile tache bouton */

/* Bits SET / RESET dans le PCF */
static const uint8_t set_bits[NUM_PIXELS] = {0,2,4,6, 8,10};
static const uint8_t rst_bits[NUM_PIXELS] = {1,3,5,7, 9,11};

/* -------------------------------------------------------------------------- */
/*                         Variables module-globales                          */
static led_strip_handle_t strip;
static QueueHandle_t      btn_q;

static volatile bool busy   = false;      /* séquence en cours ?    */
static volatile bool all_on = false;      /* étât global sorties    */
static uint8_t            mask = 0;       /* bits 0-5 des relais    */

/*  Helpers pour exposer l’état “busy” */
static inline void start_busy(void){ busy = true;  }
static inline void stop_busy (void){ busy = false; }
bool rack_ctrl_is_busy(void){ return busy; }       /* API externe     */

/* -------------------------------------------------------------------------- */
/*                             Bas-niveau I²C / PCF                           */
static uint16_t pcf_shadow = 0x0000;              /* 1 = Hi-Z          */

static esp_err_t pcf8575_write(uint16_t v)
{
    uint8_t d[2] = { v & 0xFF, v >> 8 };
    i2c_cmd_handle_t c = i2c_cmd_link_create();
    i2c_master_start(c);
    i2c_master_write_byte(c, (PCF8575_ADDR<<1)|I2C_MASTER_WRITE, true);
    i2c_master_write(c, d, 2, true);
    i2c_master_stop(c);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, c, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(c);
    return ret;
}
static inline void pcf_set_level(uint8_t bit, uint8_t hi)
{
    if(hi) pcf_shadow |=  1<<bit; else pcf_shadow &= ~(1<<bit);
    pcf8575_write(pcf_shadow);
}
static void pulse(uint8_t bit)
{
    pcf_set_level(bit, 1);                    /* MOSFET ON  */
    vTaskDelay(pdMS_TO_TICKS(PULSE_MS));
    pcf_set_level(bit, 0);                    /* MOSFET OFF */
}

/* -------------------------------------------------------------------------- */
/*                          Relais + indicateurs LED                           */
static inline void pixel(uint8_t i, bool on)
{
    led_strip_set_pixel(strip, i, on?0:64, on?64:0, 0);
    led_strip_refresh(strip);
}
static inline void relay_set(uint8_t i){ pulse(set_bits[i]); mask |=  1<<i; }
static inline void relay_rst(uint8_t i){ pulse(rst_bits[i]); mask &= ~(1<<i); }

/* -------------------------------------------------------------------------- */
/*                                 Séquences                                  */
static void seq_on(void)
{
    start_busy();
    for(uint8_t i=0;i<NUM_PIXELS;i++){
        relay_set(i);
        pixel(i,true);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    all_on = true;
    api_send_ping_full(true,  mask);   /*  1 SEUL fetch  */
    stop_busy();
}
static void seq_off(void)
{
    start_busy();
    for(int8_t i=NUM_PIXELS-1;i>=0;i--){
        relay_rst(i);
        pixel(i,false);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    all_on = false;
    api_send_ping_full(false, mask);   /* 0b000000       */
    stop_busy();
}

/* -------------------------------------------------------------------------- */
/*                             Bouton physique                                */
typedef struct{ uint32_t gpio; } btn_evt_t;

static void IRAM_ATTR isr(void* arg)
{
    btn_evt_t e = { .gpio = (uint32_t)arg };
    xQueueSendFromISR(btn_q,&e,NULL);
}
static void btn_task(void* arg)
{
    btn_evt_t e;
    for(;;){
        if(xQueueReceive(btn_q,&e,portMAX_DELAY)){
            vTaskDelay(pdMS_TO_TICKS(50));     /* anti-rebond */
            if(busy)         continue;
            all_on ? seq_off() : seq_on();
        }
    }
}

/* -------------------------------------------------------------------------- */
/*                                Init HW                                     */
static void i2c_init(void)
{
    i2c_config_t cfg={
        .mode=I2C_MODE_MASTER,.sda_io_num=SDA_PIN,.scl_io_num=SCL_PIN,
        .sda_pullup_en=GPIO_PULLUP_ENABLE,.scl_pullup_en=GPIO_PULLUP_ENABLE,
        .master.clk_speed=I2C_FREQ_HZ};
    i2c_param_config(I2C_PORT,&cfg);
    i2c_driver_install(I2C_PORT,cfg.mode,0,0,0);
    pcf8575_write(pcf_shadow);                    /* Hi-Z partout */
}
static void neopixel_init(void)
{
    led_strip_config_t sc={ .strip_gpio_num=NEOPIXEL_PIN,.max_leds=NUM_PIXELS };
    led_strip_rmt_config_t rc={ .clk_src=RMT_CLK_SRC_DEFAULT,
                                .resolution_hz=10*1000*1000UL,
                                .mem_block_symbols=64 };
    led_strip_new_rmt_device(&sc,&rc,&strip);
    led_strip_clear(strip);
}
static void button_init(void)
{
    gpio_config_t io={ .pin_bit_mask=1ULL<<BUTTON_PIN,.mode=GPIO_MODE_INPUT,
                       .pull_up_en=GPIO_PULLUP_ENABLE,.intr_type=GPIO_INTR_ANYEDGE };
    gpio_config(&io);
    btn_q=xQueueCreate(4,sizeof(btn_evt_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    gpio_isr_handler_add(BUTTON_PIN,isr,(void*)BUTTON_PIN);
    xTaskCreatePinnedToCore(btn_task,"btn",BTN_STACK,NULL,10,NULL,0);
}

/* -------------------------------------------------------------------------- */
void rack_ctrl_init(void)
{
    ESP_LOGI(TAG,"Initialisation rack…");
    i2c_init();
    neopixel_init();
    button_init();
    ESP_LOGI(TAG,"Prêt !");
}

/* ----------------------------- API publiques ------------------------------ */
void rack_ctrl_sequence_on (void){ if(!busy && !all_on) seq_on();  }
void rack_ctrl_sequence_off(void){ if(!busy &&  all_on) seq_off(); }
bool rack_ctrl_is_on(void)        { return all_on; }
