#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"

#define STRIP_GPIO     21
#define STRIP_LED_NUM  10

static const char *TAG = "TEST_LED";
led_strip_handle_t strip;

void app_main(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = STRIP_GPIO,
        .max_leds       = STRIP_LED_NUM,
        .led_model      = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,
        .resolution_hz  = 10 * 1000 * 1000, // 10 MHz
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Échec de l'initialisation du ruban");
        return;
    }

    ESP_LOGI(TAG, "Ruban initialisé, effacement...");
    led_strip_clear(strip);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Allumage LED 0 en rouge");
    led_strip_set_pixel(strip, 0, 255, 0, 0);
    led_strip_refresh(strip);
}
