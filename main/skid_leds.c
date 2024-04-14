#include <esp_log.h>
#include <led_strip.h>

static const char *TAG = "skid_leds";

static const gpio_num_t SKID_LEDS_PIN = CONFIG_SKID_LEDS_PIN;
static led_strip_handle_t led_strip = 0;
static const uint32_t max_leds = 2;

typedef uint32_t rgb_t[3];
static const rgb_t SKID_LED_PRESET[] = {
        {0,   0,   0},
        {255, 255, 255},
        {170, 170, 170},
        {100, 100, 100},
        {170, 20,  150},
        {20,  170, 150},
};
static const size_t SKID_LED_PRESET_COUNT = sizeof(SKID_LED_PRESET) / sizeof(rgb_t);
static size_t selected_preset = 0;

void skid_leds_init() {
    ESP_LOGI(TAG, "init");

    led_strip_config_t strip_config = {
            .strip_gpio_num = SKID_LEDS_PIN,
            .max_leds = max_leds,
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

void skids_leds_set(uint32_t red, uint32_t green, uint32_t blue) {
    if (led_strip) {
        for (uint32_t index = 0; index < max_leds; index++) {
            esp_err_t err = led_strip_set_pixel(led_strip, index, red, green, blue);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set pixel %lud: %s", index, esp_err_to_name(err));
            }
        }
        led_strip_refresh(led_strip);
    }
}

void skid_leds_preset(uint32_t preset) {
    selected_preset = preset % SKID_LED_PRESET_COUNT;
    const rgb_t *rgb = &SKID_LED_PRESET[selected_preset];
    skids_leds_set((*rgb)[0], (*rgb)[1], (*rgb)[2]);
}

void skid_leds_cycle() {
    skid_leds_preset(selected_preset + 1);
}
