#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <double_reset.h>
#include <button_gpio.h>
#include <nvs_flash.h>

static const char TAG[] = "app_main";

_Noreturn void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    bool reconfigure = false;
    ESP_ERROR_CHECK(double_reset_start(&reconfigure, 5000));

    ESP_LOGI(TAG, "started");

    while (true)
    {
        vTaskDelay(1);
    }
}
