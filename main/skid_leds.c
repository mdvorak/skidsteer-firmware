#include <esp_log.h>
#include <led_strip.h>

static const char *TAG = "skid_leds";

static const gpio_num_t SKID_LEDS_PIN = CONFIG_SKID_LEDS_PIN;
static led_strip_handle_t led_strip = 0;

void skid_leds_init() {
    ESP_LOGI(TAG, "init");

    led_strip_config_t strip_config = {
            .strip_gpio_num = SKID_LEDS_PIN,
            .max_leds = 2,
            .led_pixel_format = LED_PIXEL_FORMAT_GRB,
            .led_model = LED_MODEL_WS2812,
            .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
            .rmt_channel = 0,
#else
            .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
            .resolution_hz = 10 * 1000 * 1000, // 10MHz
            .flags.with_dma = false, // whether to enable the DMA feature
#endif
    };
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    // Log but continue
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "configured");
    } else {
        ESP_LOGE(TAG, "Failed to create LED strip device: %s", esp_err_to_name(err));
    }
}
