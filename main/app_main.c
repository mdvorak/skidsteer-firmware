#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <button_gpio.h>
#include <nvs_flash.h>
#include <driver/ledc.h>

static const char TAG[] = "app_main";

ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num  = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
};

ledc_channel_config_t ledc_channel[3];


_Noreturn void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 1));

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel[0].channel = LEDC_CHANNEL_0;
    ledc_channel[0].gpio_num = 24;

    ledc_channel[1].channel = LEDC_CHANNEL_1;
    ledc_channel[1].gpio_num = 25;

    ledc_channel[2].channel = LEDC_CHANNEL_2;
    ledc_channel[2].gpio_num = 26;

    for (int i = 0; i < 3; i++) {
        ledc_channel[i].speed_mode = LEDC_LOW_SPEED_MODE;
        ledc_channel[i].timer_sel = LEDC_TIMER_0;
        ledc_channel[i].intr_type = LEDC_INTR_DISABLE;
        ledc_channel[i].duty = 0;
        ledc_channel[i].hpoint = 0;

        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[i]));
    }

    ESP_LOGI(TAG, "started");

    while (true) {
        vTaskDelay(1);
    }
}
