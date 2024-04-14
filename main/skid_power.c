#include "skid.h"
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <driver/gpio.h>

static const char *TAG = "skid_power";

// Config
static const gpio_num_t SKID_POWER_PIN = CONFIG_SKID_POWER_PIN;
static const int64_t SKID_POWER_TIMER_PERIOD = 1000000ll; // 1 seconds
static const int64_t SKID_POWER_TIMEOUT_US = (CONFIG_SKID_POWER_TIMEOUT_S) * 1000000ll;
static esp_timer_handle_t skid_power_timer = NULL;
static volatile int64_t skid_power_last_activity = 0;

void skid_power_timer_callback(void *) {
    int64_t now = esp_timer_get_time();
    if (now - skid_power_last_activity > SKID_POWER_TIMEOUT_US) {
        ESP_LOGI(TAG, "Timeout");
        skid_power(false);
    }
}

void skid_power_init() {
    ESP_LOGI(TAG, "init");
    skid_power(true);

    esp_timer_create_args_t timerArgs = {
            .callback = skid_power_timer_callback,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "skid_power",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &skid_power_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(skid_power_timer, SKID_POWER_TIMER_PERIOD));
}

void skid_power(bool enabled) {
    if (enabled) {
        ESP_ERROR_CHECK(gpio_set_direction(SKID_POWER_PIN, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(SKID_POWER_PIN, 1));
        ESP_LOGI(TAG, "Power enabled");
    } else {
        ESP_LOGI(TAG, "Powering off");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(gpio_set_direction(SKID_POWER_PIN, GPIO_MODE_INPUT));
        esp_deep_sleep_start();
    }
}

void skid_power_report_activity() {
    skid_power_last_activity = esp_timer_get_time();
}
