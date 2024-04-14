#include "skid.h"
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

static const char *TAG = "skid_power";

// Config
static const gpio_num_t SKID_POWER_PIN = CONFIG_SKID_POWER_PIN;

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
